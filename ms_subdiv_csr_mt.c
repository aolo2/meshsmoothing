// init accel structure (two CSRs)
static struct ms_accel
init_acceleration_struct_mt(struct ms_mesh mesh)
{
    TracyCZone(__FUNC__, true);
    
    TracyCZoneN(alloc_initial_offsets, "alloc offset arrays", true);
    int *edges_from = calloc(1, (mesh.nverts + 1) * sizeof(int));
    
    TracyCAlloc(edges_from, (mesh.nverts + 1) * sizeof(int));
    TracyCZoneEnd(alloc_initial_offsets);
    
    TracyCZoneN(count_initial_offsets, "count all edges", true);
    int nedges = 0;
    int nfaces = 0;
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int next = (vert + 1) % mesh.degree;
            
            int start = mesh.faces[face * mesh.degree + vert];
            int end = mesh.faces[face * mesh.degree + next];
            
            edges_from[start + 1]++;
            edges_from[end + 1]++;
            
            nedges += 2;
            nfaces += 2;
        }
    }
    
    for (int v = 1; v < mesh.nverts + 1; ++v) {
        edges_from[v] += edges_from[v - 1];
    }
    
    int *faces_from = edges_from;
    int *edges_from_repeats = edges_from;
    TracyCZoneEnd(count_initial_offsets);
    
    TracyCZoneN(count_unique_edges, "alloc and count unique edges", true);
    
    TracyCZoneN(alloc_unique_edges, "alloc", true);
    int *edges = malloc(nedges * sizeof(int));
    int *faces = malloc(nfaces * sizeof(int));
    
    int *edges_accum = calloc(1, mesh.nverts * sizeof(int));
    int *faces_accum = calloc(1, mesh.nverts * sizeof(int));
    
    int *edges_repeats = malloc(nedges * sizeof(int));
    int *edges_accum_repeats = calloc(1, mesh.nverts * sizeof(int));
    
    int *edge_indices = malloc(nedges * sizeof(int));
    int *edge_faces = malloc(nedges * sizeof(int));
    int *edge_indices_accum = calloc(1, mesh.nverts * sizeof(int));
    
    TracyCAlloc(edges, nedges * sizeof(int));
    TracyCAlloc(faces, nedges * sizeof(int));
    
    TracyCAlloc(edges_accum, mesh.nverts * sizeof(int));
    TracyCAlloc(faces_accum, mesh.nverts * sizeof(int));
    
    TracyCAlloc(edges_accum_repeats, mesh.nverts * sizeof(int));
    TracyCAlloc(edges_repeats, mesh.nverts * sizeof(int));
    
    TracyCAlloc(edge_indices, nedges * sizeof(int));
    TracyCAlloc(edge_faces, nedges * sizeof(int));
    TracyCAlloc(edge_indices_accum, mesh.nverts * sizeof(int));
    
    TracyCZoneEnd(alloc_unique_edges);
    
    TracyCZoneN(count_unique_edges_count, "count", true);
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int next = (vert + 1) % mesh.degree;
            
            int start_edge_index = face * mesh.degree + vert;
            int end_edge_index = face * mesh.degree + next;
            
            int start = mesh.faces[start_edge_index];
            int end = mesh.faces[end_edge_index];
            
            /* edge start */
            int edge_base = edges_from[start];
            int edge_count = edges_accum[start];
            
            int edge_base_repeats = edges_from_repeats[start];
            int edge_count_repeats = edges_accum_repeats[start];
            
            edges_repeats[edge_base_repeats + edge_count_repeats] = end;
            ++edges_accum_repeats[start];
            
            edge_indices[edge_base_repeats + edge_count_repeats] = start_edge_index;
            edge_faces[edge_base_repeats + edge_count_repeats] = face;
            ++edge_indices_accum[start];
            
            edge_base_repeats = edges_from_repeats[end];
            edge_count_repeats = edges_accum_repeats[end];
            
            edges_repeats[edge_base_repeats + edge_count_repeats] = start;
            ++edges_accum_repeats[end];
            
            edge_indices[edge_base_repeats + edge_count_repeats] = start_edge_index;
            edge_faces[edge_base_repeats + edge_count_repeats] = face;
            ++edge_indices_accum[end];
            
            bool found = false;
            for (int e = edge_base; e < edge_base + edge_count; ++e) {
                if (edges[e] == end) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                edges[edge_base + edge_count] = end;
                edges_accum[start] += 1;
            }
            
            /* edge end */
            edge_base = edges_from[end];
            edge_count = edges_accum[end];
            
            found = false;
            for (int e = edge_base; e < edge_base + edge_count; ++e) {
                if (edges[e] == start) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                edges[edge_base + edge_count] = start;
                edges_accum[end] += 1;
            }
            
            /* start face */
            int face_base = faces_from[start];
            int face_count = faces_accum[start];
            
            found = false;
            for (int f = face_base; f < face_base + face_count; ++f) {
                if (faces[f] == face) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                faces[face_base + face_count] = face;
                faces_accum[start] += 1;
            }
            
            /* end face */
            face_base = faces_from[end];
            face_count = faces_accum[end];
            
            found = false;
            for (int f = face_base; f < face_base + face_count; ++f) {
                if (faces[f] == face) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                faces[face_base + face_count] = face;
                faces_accum[end] += 1;
            }
        }
    }
    
    faces_from = malloc((mesh.nverts + 1) * sizeof(int));
    edges_from_repeats = malloc((mesh.nverts + 1) * sizeof(int));
    
    TracyCAlloc(faces_from, (mesh.nverts + 1) * sizeof(int));
    TracyCAlloc(edges_from_repeats, (mesh.nverts + 1) * sizeof(int));
    
    memcpy(faces_from, edges_from, (mesh.nverts + 1) * sizeof(int));
    memcpy(edges_from_repeats, edges_from, (mesh.nverts + 1) * sizeof(int));
    
    TracyCZoneEnd(count_unique_edges_count);
    TracyCZoneEnd(count_unique_edges);
    
    TracyCZoneN(tight_pack, "pack unique edges", true);
    /* Tighter! */
    int edges_head = 0;
    int faces_head = 0;
    
    for (int v = 0; v < mesh.nverts; ++v) {
        int e_from = edges_from[v];
        int e_count = edges_accum[v];
        
        edges_from[v] = edges_head;
        
        for (int i = 0; i < e_count; ++i) {
            edges[edges_head] = edges[e_from + i];
            ++edges_head;
        }
    }
    
    edges_from[mesh.nverts] = edges_head;
    
    for (int v = 0; v < mesh.nverts; ++v) {
        int f_from = faces_from[v];
        int f_count = faces_accum[v];
        
        faces_from[v] = faces_head;
        
        for (int i = 0; i < f_count; ++i) {
            faces[faces_head++] = faces[f_from + i];
        }
    }
    
    faces_from[mesh.nverts] = faces_head;
    
    TracyCZoneEnd(tight_pack);
    
    TracyCZoneN(return_and_free, "free memory", true);
    struct ms_accel result = { 0 };
    
    result.faces_starts = faces_from;
    result.verts_starts = edges_from;
    result.faces_matrix = faces;
    result.verts_matrix = edges;
    
    result.edge_indices = edge_indices;
    result.edge_faces = edge_faces;
    
    result.verts_starts_repeats = edges_from_repeats;
    result.verts_matrix_repeats = edges_repeats;
    
    free(faces_accum);
    free(edges_accum);
    free(edges_accum_repeats);
    free(edge_indices_accum);
    
    TracyCFree(faces_accum);
    TracyCFree(edges_accum);
    TracyCFree(edges_accum_repeats);
    TracyCFree(edge_indices_accum);
    
    TracyCZoneEnd(return_and_free);
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}