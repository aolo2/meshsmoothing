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
            edge->findex_2 = findex;
            edge->face_2 = face;
            return;
        }
    }
    
    /* If we are here, this edge touches a hole (add_edge_1 was not called for it) */
    struct ms_edge *edge = edges + base + edges_accum[end];
    
    edge->start = start;
    edge->end = end;
    edge->findex_1 = edge->findex_2 = findex;
    edge->face_1 = edge->face_2 = face;
    
    ++edges_accum[end];
}

static struct ms_edges
init_acceleration_struct(struct ms_mesh *mesh)
{
    /* 
This function HEAVILY relies on the fact that all faces are oriented either CW or CCW 
In particular, it is used to ensure that if we have an edge AB, where A < B, then this
edge either lies on a cut, or we have an edge BA.
*/
    
    TracyCZone(__FUNC__, true);
    
    int *offsets = calloc64((mesh->nverts + 1) * sizeof(int));
    
    TracyCZoneN(count, "count edges", true);
    
    /* Count edges */
    for (int face = 0; face < mesh->nfaces * 4; face += 4) {
        int v1 = mesh->faces[face + 0];
        int v2 = mesh->faces[face + 1];
        int v3 = mesh->faces[face + 2];
        int v4 = mesh->faces[face + 3];
        
        /* 
start < end guarantees that an edge will be added
start > end means that an edge COULD be added
 */
        
        offsets[MIN(v1, v2) + 1]++; 
        offsets[MIN(v2, v3) + 1]++; 
        offsets[MIN(v3, v4) + 1]++; 
        offsets[MIN(v4, v1) + 1]++; 
    }
    
    TracyCZoneEnd(count);
    
    TracyCZoneN(propogate, "propogate offsets", true);
    
    /* Propogate offsets */
    for (int v = 1; v < mesh->nverts + 1; ++v) {
        offsets[v] += offsets[v - 1];
    }
    
    TracyCZoneEnd(propogate);
    
    
    TracyCZoneN(add_starts, "add edges first pass", true);
    
    int nedges = offsets[mesh->nverts];
    struct ms_edge *edges = calloc64(nedges * sizeof(struct ms_edge));
    int *edges_accum = calloc64(nedges * sizeof(int));
    
    for (int face = 0; face < mesh->nfaces * 4; face += 4) {
        int v1 = mesh->faces[face + 0];
        int v2 = mesh->faces[face + 1];
        int v3 = mesh->faces[face + 2];
        int v4 = mesh->faces[face + 3];
        
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
    }
    
    TracyCZoneEnd(add_starts);
    
    TracyCZoneN(add_ends, "add edges second pass", true);
    
    for (int face = 0; face < mesh->nfaces * 4; face += 4) {
        int v1 = mesh->faces[face + 0];
        int v2 = mesh->faces[face + 1];
        int v3 = mesh->faces[face + 2];
        int v4 = mesh->faces[face + 3];
        
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
    
    TracyCZoneN(compact, "remove empty edges", true);
    
    TracyCZoneN(compact_count, "count non-empty edges", true);
    int actual_nedges = 0;
    for (int e = 0; e < nedges; ++e) {
        struct ms_edge *edge = edges + e;
        if (edge->start != edge->end) {
            ++actual_nedges;
        }
    }
    TracyCZoneEnd(compact_count);
    
    TracyCZoneN(compact_write, "write non-empty edges", true);
    struct ms_edge *actual_edges = malloc64(actual_nedges * sizeof(struct ms_edge));
    int head = 0;
    
    for (int e = 0; e < nedges; ++e) {
        struct ms_edge edge = edges[e];
        if (edge.start != edge.end) {
            actual_edges[head++] = edge;
        }
    }
    TracyCZoneEnd(compact_write);
    
    TracyCZoneEnd(compact);
    
    free(offsets);
    free(edges);
    free(edges_accum);
    
    struct ms_edges result;
    
    result.count = actual_nedges;
    result.edges = actual_edges;
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}

static void
repack_mesh(struct ms_mesh *mesh)
{
    TracyCZone(__FUNC__, true);
    
    int counter = 0;
    int *aliases = malloc(mesh->nverts * sizeof(int));
    f32 *new_vertices_x = malloc(mesh->nverts * sizeof(f32));
    f32 *new_vertices_y = malloc(mesh->nverts * sizeof(f32));
    f32 *new_vertices_z = malloc(mesh->nverts * sizeof(f32));
    
    TracyCZoneN(memset_alias, "memset aliases to -1", true);
    memset(aliases, -1, mesh->nverts * sizeof(int)); /* software prefetch :P */
    TracyCZoneEnd(memset_alias);
    
    TracyCZoneN(initial_rename, "initial rename", true);
    for (int v = 0; v < mesh->nfaces * 4; ++v) {
        int vertex = mesh->faces[v];
        if (aliases[vertex] == -1) {
            aliases[vertex] = counter++;
        }
    }
    TracyCZoneEnd(initial_rename);
    
    TracyCZoneN(replace_faces, "replace faces with alias", true);
    for (int v = 0; v < mesh->nfaces * 4; ++v) {
        int alias = aliases[mesh->faces[v]];
        mesh->faces[v] = alias;
    }
    TracyCZoneEnd(replace_faces);
    
    TracyCZoneN(replace_coordinates, "replace coordinate data", true);
    for (int v = 0; v < mesh->nverts; ++v) {
        int alias = aliases[v];
        new_vertices_x[alias] = mesh->vertices_x[v];
        new_vertices_y[alias] = mesh->vertices_y[v];
        new_vertices_z[alias] = mesh->vertices_z[v];
    }
    TracyCZoneEnd(replace_coordinates);
    
    free(aliases);
    free(mesh->vertices_x);
    free(mesh->vertices_y);
    free(mesh->vertices_z);
    
    mesh->vertices_x = new_vertices_x;
    mesh->vertices_y = new_vertices_y;
    mesh->vertices_z = new_vertices_z;
    
    TracyCZoneEnd(__FUNC__);
}