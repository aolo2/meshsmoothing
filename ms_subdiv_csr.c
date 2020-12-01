static inline void
add_edge_1(int start, int end, int findex, int face,
           struct ms_edge *edges, int *edges_accum, int *offsets)
{
    struct ms_edge edge;
    
    edge.start = start;
    edge.end = end;
    
    edge.findex_1 = findex;
    edge.face_1 = face;
    
    /* If edge touches a hole, this will not be overwritten */
    edge.face_2 = edge.face_1;
    edge.findex_2 = edge.findex_1;
    
    edges[offsets[start] + edges_accum[start]++] = edge;
}

static inline void
add_edge_2(int start, int end, int findex, int face,
           struct ms_edge *edges, int *edges_accum, int *offsets)
{
    int base = offsets[end];
    for (int i = 0; i < edges_accum[end]; ++i) {
        struct ms_edge *edge = edges + base + i;
        if (edge->end == start) {
            /*
This is guaranteed to happen, because:
1. Edges are traversed in the order of their starts
2. If (end > start) then start has already been traversed, and edges[start] already has end
*/
            edge->findex_2 = findex;
            edge->face_2 = face;
            
            break;
        }
    }
}

static struct ms_edges
init_acceleration_struct(struct ms_mesh mesh)
{
    TracyCZone(__FUNC__, true);
    
    int *offsets = calloc64((mesh.nverts + 1) * sizeof(int));
    int *offsets_both = calloc64((mesh.nverts + 1) * sizeof(int));
    
    TracyCZoneN(count, "count edges", true);
    
    /* Count edges */
    for (int face = 0; face < mesh.nfaces * 4; face += 4) {
        int v1 = mesh.faces[face + 0];
        int v2 = mesh.faces[face + 1];
        int v3 = mesh.faces[face + 2];
        int v4 = mesh.faces[face + 3];
        
        if (v1 < v2) offsets[v1 + 1]++;
        if (v2 < v3) offsets[v2 + 1]++;
        if (v3 < v4) offsets[v3 + 1]++;
        if (v4 < v1) offsets[v4 + 1]++;
        
        offsets_both[v1 + 1]++;
        offsets_both[v2 + 1]++;
        offsets_both[v3 + 1]++;
        offsets_both[v4 + 1]++;
    }
    
    TracyCZoneEnd(count);
    
    TracyCZoneN(propogate, "propogate offsets", true);
    
    /* Propogate offsets */
    for (int v = 1; v < mesh.nverts + 1; ++v) {
        offsets[v] += offsets[v - 1];
        offsets_both[v] += offsets_both[v - 1];
    }
    
    TracyCZoneEnd(propogate);
    
    
    TracyCZoneN(add_starts, "add edges first pass", true);
    
    int nedges = offsets[mesh.nverts];
    int nedges_both = offsets_both[mesh.nverts];
    struct ms_edge *edges = malloc64(nedges * sizeof(struct ms_edge));
    int *edges_accum = calloc64(nedges * sizeof(int));
    
    int *edges_simple = malloc64(nedges_both * sizeof(int));
    int *faces_simple = malloc64(nedges_both * sizeof(int));
    int *edges_accum_simple = calloc64(nedges_both * sizeof(int));
    
    for (int face = 0; face < mesh.nfaces * 4; face += 4) {
        int v1 = mesh.faces[face + 0];
        int v2 = mesh.faces[face + 1];
        int v3 = mesh.faces[face + 2];
        int v4 = mesh.faces[face + 3];
        
        int actual_face = face / 4;
        
        /*
v1 ----- v2 
           |    0    | 
         |3        | 
         |        1| 
         |    2    | 
         v4 ----- v3
*/
        if (v1 < v2) add_edge_1(v1, v2, 0, actual_face, edges, edges_accum, offsets);
        if (v2 < v3) add_edge_1(v2, v3, 1, actual_face, edges, edges_accum, offsets);
        if (v3 < v4) add_edge_1(v3, v4, 2, actual_face, edges, edges_accum, offsets);
        if (v4 < v1) add_edge_1(v4, v1, 3, actual_face, edges, edges_accum, offsets);
        
        int i1 = offsets_both[v1] + edges_accum_simple[v1]++;
        int i2 = offsets_both[v2] + edges_accum_simple[v2]++;
        int i3 = offsets_both[v3] + edges_accum_simple[v3]++;
        int i4 = offsets_both[v4] + edges_accum_simple[v4]++;
        
        edges_simple[i1] = v2;
        edges_simple[i2] = v3;
        edges_simple[i3] = v4;
        edges_simple[i4] = v1;
        
        faces_simple[i1] = actual_face;
        faces_simple[i2] = actual_face;
        faces_simple[i3] = actual_face;
        faces_simple[i4] = actual_face;
    }
    
    TracyCZoneEnd(add_starts);
    
    TracyCZoneN(add_ends, "add edges second pass", true);
    
    for (int face = 0; face < mesh.nfaces * 4; face += 4) {
        int v1 = mesh.faces[face + 0];
        int v2 = mesh.faces[face + 1];
        int v3 = mesh.faces[face + 2];
        int v4 = mesh.faces[face + 3];
        
        int actual_face = face / 4;
        
        /*
v1 ----- v2 
           |    0    | 
         |3        | 
         |        1| 
         |    2    | 
         v4 ----- v3
*/
        if (v1 > v2) add_edge_2(v1, v2, 0, actual_face, edges, edges_accum, offsets);
        if (v2 > v3) add_edge_2(v2, v3, 1, actual_face, edges, edges_accum, offsets);
        if (v3 > v4) add_edge_2(v3, v4, 2, actual_face, edges, edges_accum, offsets);
        if (v4 > v1) add_edge_2(v4, v1, 3, actual_face, edges, edges_accum, offsets);
    }
    
    TracyCZoneEnd(add_ends);
    
    free(offsets);
    free(edges_accum);
    free(edges_accum_simple);
    
    struct ms_edges result;
    
    result.count = nedges;
    result.edges = edges;
    
    result.verts_starts = offsets_both;
    result.verts_matrix = edges_simple;
    result.faces_matrix = faces_simple;
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}