static inline void
add_edge_1(int start, int end, int findex, int face, struct ms_mesh mesh, struct ms_v3 face_point,
           struct ms_edge *edges, int *edges_accum, int *offsets)
{
    struct ms_edge edge;
    
    edge.end = end;
    edge.startv = mesh.vertices[start];
    edge.endv = mesh.vertices[end];
    
    edge.facepoint_1 = face_point;
    edge.findex_1 = findex;
    edge.face_1 = face;
    
    /* If edge touches a hole, this will not be overwritten */
    edge.face_2 = edge.face_1;
    edge.findex_2 = edge.findex_1;
    edge.facepoint_2 = edge.facepoint_1;
    
    edges[offsets[start] + edges_accum[start]++] = edge;
}

static inline void
add_edge_2(int start, int end, int findex, int face, struct ms_v3 face_point,
           struct ms_edge *edges, int *edges_accum, int *offsets)
{
    for (int i = 0; i < edges_accum[end]; ++i) {
        struct ms_edge *edge = edges + (offsets[end] + i);
        if (edge->end == start) {
            /*
This is guaranteed to happen, because:
1. Edges are traversed in the order of their starts
2. If (end > start) then start has already been traversed, and edges[start] already has end
*/
            edge->facepoint_2 = face_point;
            edge->findex_2 = findex;
            edge->face_2 = face;
            
            break;
        }
    }
}

static struct ms_edges
init_acceleration_struct(struct ms_mesh mesh, struct ms_v3 *face_points)
{
    TracyCZone(__FUNC__, true);
    
    int *offsets = calloc64((mesh.nverts + 1) * sizeof(int));
    
    /* Count edges */
    for (int face = 0; face < mesh.nfaces * 4; face += 4) {
        int start = mesh.faces[face + 0];
        int end = mesh.faces[face + 1];
        
        if (start < end) {
            offsets[start + 1]++;
        }
        
        start = end;
        end = mesh.faces[face + 2];
        
        if (start < end) {
            offsets[start + 1]++;
        }
        
        start = end;
        end = mesh.faces[face + 3];
        
        if (start < end) {
            offsets[start + 1]++;
        }
        
        start = end;
        end = mesh.faces[face + 0];
        
        if (start < end) {
            offsets[start + 1]++;
        }
    }
    
    /* Propogate offsets */
    for (int v = 1; v < mesh.nverts + 1; ++v) {
        offsets[v] += offsets[v - 1];
    }
    
    int nedges = offsets[mesh.nverts];
    struct ms_edge *edges = malloc64(nedges * sizeof(struct ms_edge));
    int *edges_accum = calloc64(nedges * sizeof(int));
    
    for (int face = 0; face < mesh.nfaces * 4; face += 4) {
        int v1 = mesh.faces[face + 0];
        int v2 = mesh.faces[face + 1];
        int v3 = mesh.faces[face + 2];
        int v4 = mesh.faces[face + 3];
        
        int actual_face = face / 4;
        struct ms_v3 face_point = face_points[actual_face];
        
        /*
v1 ----- v2 
           |    0    | 
         |3        | 
         |        1| 
         |    2    | 
         v4 ----- v3
*/
        if (v1 < v2) add_edge_1(v1, v2, 0, actual_face, mesh, face_point, edges, edges_accum, offsets);
        if (v2 < v3) add_edge_1(v2, v3, 1, actual_face, mesh, face_point, edges, edges_accum, offsets);
        if (v3 < v4) add_edge_1(v3, v4, 2, actual_face, mesh, face_point, edges, edges_accum, offsets);
        if (v4 < v1) add_edge_1(v4, v1, 3, actual_face, mesh, face_point, edges, edges_accum, offsets);
    }
    
    for (int face = 0; face < mesh.nfaces * 4; face += 4) {
        int v1 = mesh.faces[face + 0];
        int v2 = mesh.faces[face + 1];
        int v3 = mesh.faces[face + 2];
        int v4 = mesh.faces[face + 3];
        
        int actual_face = face / 4;
        struct ms_v3 face_point = face_points[actual_face];
        
        /*
v1 ----- v2 
           |    0    | 
         |3        | 
         |        1| 
         |    2    | 
         v4 ----- v3
*/
        if (v1 > v2) add_edge_2(v1, v2, 0, actual_face, face_point, edges, edges_accum, offsets);
        if (v2 > v3) add_edge_2(v2, v3, 1, actual_face, face_point, edges, edges_accum, offsets);
        if (v3 > v4) add_edge_2(v3, v4, 2, actual_face, face_point, edges, edges_accum, offsets);
        if (v4 > v1) add_edge_2(v4, v1, 3, actual_face, face_point, edges, edges_accum, offsets);
    }
    
    TracyCZoneEnd(__FUNC__);
    
    free(edges_accum);
    
    struct ms_edges result;
    
    result.count = nedges;
    result.edges = edges;
    result.offsets = offsets;
    
    return(result);
}