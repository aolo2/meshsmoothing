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
    TracyCZoneEnd(count_initial_offsets);
    
    TracyCZoneN(count_unique_edges, "alloc and count unique edges", true);
    
    TracyCZoneN(alloc_unique_edges, "alloc", true);
    int *edges = malloc(nedges * sizeof(int));
    int *faces = malloc(nfaces * sizeof(int));
    
    int *edges_accum = calloc(1, mesh.nverts * sizeof(int));
    int *faces_accum = calloc(1, mesh.nverts * sizeof(int));
    
    int *edge_indices = malloc(2 * nedges * sizeof(int));
    int *edge_faces = malloc(nedges * sizeof(int));
    int *edge_indices_accum = calloc(1, mesh.nverts * sizeof(int));
    
    TracyCAlloc(edges, nedges * sizeof(int));
    TracyCAlloc(faces, nedges * sizeof(int));
    
    TracyCAlloc(edges_accum, mesh.nverts * sizeof(int));
    TracyCAlloc(faces_accum, mesh.nverts * sizeof(int));
    
    TracyCAlloc(edge_indices, 2 * nedges * sizeof(int));
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
            
            int found = -1;
            for (int e = edge_base; e < edge_base + edge_count; ++e) {
                if (edges[e] == end) {
                    found = e;
                    break;
                }
            }
            
            if (found == -1) {
                edges[edge_base + edge_count] = end;
                edges_accum[start] += 1;
                edge_indices[(edge_base + edge_count) * 2 + 0] = start_edge_index;
                edge_indices[(edge_base + edge_count) * 2 + 1] = start_edge_index;
            } else {
                edge_indices[found * 2 + 1] = start_edge_index;
            }
            
            /* edge end */
            edge_base = edges_from[end];
            edge_count = edges_accum[end];
            
            found = -1;
            for (int e = edge_base; e < edge_base + edge_count; ++e) {
                if (edges[e] == start) {
                    found = e;
                    break;
                }
            }
            
            if (found == -1) {
                edges[edge_base + edge_count] = start;
                edges_accum[end] += 1;
                edge_indices[(edge_base + edge_count) * 2 + 0] = start_edge_index;
                edge_indices[(edge_base + edge_count) * 2 + 1] = start_edge_index;
            } else {
                edge_indices[found * 2 + 1] = start_edge_index;
            }
            
            
            /* start face */
            int face_base = faces_from[start];
            int face_count = faces_accum[start];
            
            found = 0;
            for (int f = face_base; f < face_base + face_count; ++f) {
                if (faces[f] == face) {
                    found = 1;
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
            
            found = 0;
            for (int f = face_base; f < face_base + face_count; ++f) {
                if (faces[f] == face) {
                    found = 1;
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
    
    TracyCAlloc(faces_from, (mesh.nverts + 1) * sizeof(int));
    
    memcpy(faces_from, edges_from, (mesh.nverts + 1) * sizeof(int));
    
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
            edge_indices[2 * edges_head + 0] = edge_indices[(e_from + i) * 2 + 0];
            edge_indices[2 * edges_head + 1] = edge_indices[(e_from + i) * 2 + 1];
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
    
    free(faces_accum);
    free(edges_accum);
    free(edge_indices_accum);
    
    TracyCFree(faces_accum);
    TracyCFree(edges_accum);
    TracyCFree(edge_indices_accum);
    TracyCZoneEnd(return_and_free);
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}

static int
edge_adjacent_face(struct ms_accel *accel, int me, int start, int end)
{
    int start_faces_from = accel->faces_starts[start];
    int end_faces_from = accel->faces_starts[end];
    
    int start_faces_to = accel->faces_starts[start + 1];
    int end_faces_to = accel->faces_starts[end + 1];
    
    int nfaces_start = start_faces_to - start_faces_from;
    int nfaces_end = end_faces_to - end_faces_from;
    
    int *faces = accel->faces_matrix;
    
    if (nfaces_start > 0 && nfaces_end > 0) {
        for (int f1 = start_faces_from; f1 < start_faces_to; ++f1) {
            int face = faces[f1];
            if (face == me) {
                continue;
            }
            
            for (int f2 = end_faces_from; f2 < end_faces_to; ++f2) {
                int other_face = faces[f2];
                if (other_face == face) {
                    return(face);
                }
            }
        }
    }
    
    return(me);
}

static void
free_acceleration_struct(struct ms_accel *accel)
{
    free(accel->faces_starts);
    free(accel->verts_starts);
    
    free(accel->edge_indices);
    free(accel->edge_faces);
    
    free(accel->faces_matrix);
    free(accel->verts_matrix);
    
    TracyCFree(accel->faces_starts);
    TracyCFree(accel->verts_starts);
    
    TracyCFree(accel->edge_indices);
    TracyCFree(accel->edge_faces);
    
    TracyCFree(accel->faces_matrix);
    TracyCFree(accel->verts_matrix);
}