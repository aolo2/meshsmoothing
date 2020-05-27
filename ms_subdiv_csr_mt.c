static inline void
add_edge_and_face(struct ms_v4i *offsets, int *edges, int *faces, int *edge_indices,
                  int end, int face, int edge_index)
{
    /* edge */
    int edge_base = offsets->a;
    int edge_count = offsets->b;
    
    int face_base = offsets->c;
    int face_count = offsets->d;
    
    bool found = false;
    for (int e = edge_base; e < edge_base + edge_count; ++e) {
        if (edges[e] == end) {
            found = true;
            edge_indices[e * 2 + 1] = edge_index;
            break;
        }
    }
    
    if (!found) {
        edges[edge_base + edge_count] = end;
        offsets->b += 1;
        edge_indices[(edge_base + edge_count) * 2 + 0] = edge_index;
        edge_indices[(edge_base + edge_count) * 2 + 1] = edge_index;
    }
    
    /* face */
    found = false;
    for (int f = face_base; f < face_base + face_count + 1; ++f) {
        if (faces[f] == face) {
            found = true;
            break;
        }
    }
    
    if (!found) {
        faces[face_base + face_count] = face;
        offsets->d += 1;
    }
}

// init accel structure (two CSRs)
static struct ms_accel
init_acceleration_struct_mt(struct ms_mesh mesh, struct ms_v3 *face_points)
{
    (void) face_points;
    
    TracyCZoneS(__FUNC__, true, CALLSTACK_DEPTH);
    
    int nthreads = NTHREADS;
    
    int nedges = mesh.nfaces * mesh.degree * 2;
    int nfaces = nedges;
    
    struct ms_v4i *offsets = NULL;
    posix_memalign((void **) &offsets, 64, (mesh.nverts + 1) * sizeof(struct ms_v4i));
    assert(offsets);
    memset(offsets, 0x00, (mesh.nverts + 1) * sizeof(struct ms_v4i));
    TracyCAlloc(offsets, (mesh.nverts + 1) * sizeof(struct ms_v4i));
    
    int **edges_from_locals = malloc(nthreads * sizeof(int *));
    TracyCAllocS(edges_from_locals, nthreads * sizeof(int *), CALLSTACK_DEPTH);
    
    int *edges_from = NULL;
    int *faces_from = NULL;
    
    posix_memalign((void **) &edges_from, 64, ((mesh.nverts + 1) * sizeof(int)));
    posix_memalign((void **) &faces_from, 64, ((mesh.nverts + 1) * sizeof(int)));
    
    TracyCAlloc(edges_from, (mesh.nverts + 1) * sizeof(int));
    TracyCAlloc(faces_from, (mesh.nverts + 1) * sizeof(int));
    
    assert(edges_from_locals);
    
#pragma omp parallel
    {
        TracyCZoneNS(count_initial_offsets, "count offsets (local)", true, CALLSTACK_DEPTH);
        
        int tid = omp_get_thread_num();
        
        edges_from_locals[tid] = calloc(1, (mesh.nverts + 1) * sizeof(int));
        TracyCAllocS(edges_from_locals[tid], (mesh.nverts + 1) * sizeof(int), CALLSTACK_DEPTH);
        
#pragma omp for
        for (int face = 0; face < mesh.nfaces; ++face) {
            for (int vert = 0; vert < mesh.degree; ++vert) {
                int next = (vert + 1) % mesh.degree;
                
                int start = mesh.faces[face * mesh.degree + vert];
                int end = mesh.faces[face * mesh.degree + next];
                
                edges_from_locals[tid][start + 1]++;
                edges_from_locals[tid][end + 1]++;
            }
        }
        TracyCZoneEnd(count_initial_offsets);
        
        TracyCZoneNS(propogate_local_offsets, "propogate offsets (local)", true, CALLSTACK_DEPTH);
        for (int v = 1; v < mesh.nverts + 1; ++v) {
            edges_from_locals[tid][v] += edges_from_locals[tid][v - 1];
        }
        TracyCZoneEnd(propogate_local_offsets);
    }
    
    for (int t = 0; t < nthreads; ++t) {
#pragma omp parallel
        {
            TracyCZoneNS(reduce_offsets, "reduce offsets", true, CALLSTACK_DEPTH);
            
#pragma omp for
            for (int v = 1; v < mesh.nverts + 1; ++v) {
                offsets[v].a += edges_from_locals[t][v];
                offsets[v].c += edges_from_locals[t][v];
            }
            
            TracyCZoneEnd(reduce_offsets);
        }
    }
    
    TracyCZoneNS(alloc_unique_edges, "alloc unique edges", true, CALLSTACK_DEPTH);
    int *edges = NULL;
    int *faces = NULL;
    
    int *edge_indices = NULL;
    int *edge_indices_accum = NULL;
    
    posix_memalign((void **) &edges, 64, nedges * sizeof(int));
    posix_memalign((void **) &faces, 64, nfaces * sizeof(int));
    
    posix_memalign((void **) &edge_indices, 64, 2 * nedges * sizeof(int));
    posix_memalign((void **) &edge_indices_accum, 64, mesh.nverts * sizeof(int));
    
    assert(edges);
    assert(faces);
    assert(edge_indices);
    assert(edge_indices_accum);
    
    memset(edge_indices_accum, 0x00, mesh.nverts * sizeof(int));
    
    TracyCAllocS(edges, nedges * sizeof(int), CALLSTACK_DEPTH);
    TracyCAllocS(faces, nedges * sizeof(int), CALLSTACK_DEPTH);
    
    TracyCAllocS(edge_indices, 2 * nedges * sizeof(int), CALLSTACK_DEPTH);
    TracyCAllocS(edge_indices_accum, mesh.nverts * sizeof(int), CALLSTACK_DEPTH);
    
    TracyCZoneEnd(alloc_unique_edges);
    
#pragma omp parallel
    {
        TracyCZoneNS(count_unique_edges_count, "count unique edges", true, CALLSTACK_DEPTH);
        
        int tid = omp_get_thread_num();
        int block_size = mesh.nverts / nthreads;
        
        int this_tid_process_from = tid * block_size;
        int this_tid_process_to = this_tid_process_from + block_size;
        
        if (tid == nthreads - 1) {
            this_tid_process_to = mesh.nverts;
        }
        
        for (int face = 0; face < mesh.nfaces; ++face) {
            for (int vert = 0; vert < mesh.degree; ++vert) {
                int next = (vert + 1) & 0x3;
                
                int start_edge_index = face * mesh.degree + vert;
                int end_edge_index = face * mesh.degree + next;
                
                int end = mesh.faces[end_edge_index];
                int start = mesh.faces[start_edge_index];
                
                if (this_tid_process_from <= start && start < this_tid_process_to) {
                    add_edge_and_face(offsets + start, edges, faces, edge_indices,
                                      end, face, start_edge_index);
                }
                
                if (this_tid_process_from <= end && end < this_tid_process_to) {
                    add_edge_and_face(offsets + end, edges, faces, edge_indices,
                                      start, face, start_edge_index);
                }
            }
        }
        
        TracyCZoneEnd(count_unique_edges_count);
        
        TracyCZoneNS(compute_face_points, "face points", true, CALLSTACK_DEPTH);
        
        f32 one_over_mesh_degree = 1.0f / mesh.degree;
        
#pragma omp for schedule(guided) // barrier!
        for (int f = 0; f < mesh.nfaces * 4; f += 4) {
            
            struct ms_v3 v1 = mesh.vertices[mesh.faces[f + 0]];
            struct ms_v3 v2 = mesh.vertices[mesh.faces[f + 1]];
            struct ms_v3 v3 = mesh.vertices[mesh.faces[f + 2]];
            struct ms_v3 v4 = mesh.vertices[mesh.faces[f + 3]];
            
            struct ms_v3 fp;
            fp.x = (v1.x + v2.x + v3.x + v4.x) * one_over_mesh_degree;
            fp.y = (v1.y + v2.y + v3.y + v4.y) * one_over_mesh_degree;
            fp.z = (v1.z + v2.z + v3.z + v4.z) * one_over_mesh_degree;
            
            face_points[f >> 2] = fp;
        }
        
        TracyCZoneEnd(compute_face_points);
        
#pragma omp sections
        {
#pragma omp section
            {
                TracyCZoneN(pack_edges, "pack edges", true);
                int edges_head = 0;
                
                for (int v = 0; v < mesh.nverts; ++v) {
                    int e_from = offsets[v].a;
                    int e_count = offsets[v].b;
                    
                    edges_from[v] = edges_head;
                    
                    for (int i = 0; i < e_count; ++i) {
                        edges[edges_head] = edges[e_from + i];
                        edge_indices[2 * edges_head + 0] = edge_indices[(e_from + i) * 2 + 0];
                        edge_indices[2 * edges_head + 1] = edge_indices[(e_from + i) * 2 + 1];
                        ++edges_head;
                    }
                }
                
                edges_from[mesh.nverts] = edges_head;
                
                TracyCZoneEnd(pack_edges);
            }
            
#pragma omp section
            {
                TracyCZoneN(pack_faces, "pack faces", true);
                int faces_head = 0;
                
                for (int v = 0; v < mesh.nverts; ++v) {
                    int f_from = offsets[v].c;
                    int f_count = offsets[v].d;
                    
                    faces_from[v] = faces_head;
                    
                    for (int i = 0; i < f_count; ++i) {
                        faces[faces_head++] = faces[f_from + i];
                    }
                }
                
                faces_from[mesh.nverts] = faces_head;
                
                TracyCZoneEnd(pack_faces);
            }
        }
    }
    
    
    TracyCZoneNS(return_and_free, "free memory", true, CALLSTACK_DEPTH);
    struct ms_accel result = { 0 };
    
    result.faces_starts = faces_from;
    result.verts_starts = edges_from;
    result.faces_matrix = faces;
    result.verts_matrix = edges;
    
    result.edge_indices = edge_indices;
    
    for (int t = 0; t < nthreads; ++t) {
        free(edges_from_locals[t]);
        TracyCFree(edges_from_locals[t]);
    }
    
    free(edges_from_locals);
    TracyCFree(edges_from_locals);
    
    free(edge_indices_accum);
    
    TracyCFree(edge_indices_accum);
    TracyCZoneEnd(return_and_free);
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}

static struct ms_v2i
edge_adjacent_faces(struct ms_accel *accel, int start, int end)
{
    int start_from = accel->verts_starts[start];
    int start_to = accel->verts_starts[start + 1];
    
    struct ms_v2i result = { 0 };
    
    for (int e = start_from; e < start_to; ++e) {
        int some_end = accel->verts_matrix[e];
        
        if (some_end == end) {
            int face_1 = accel->edge_indices[2 * e + 0] >> 2;
            int face_2 = accel->edge_indices[2 * e + 1] >> 2;
            
            result.a = face_1;
            result.b = face_2;
            
            break;
        }
    }
    
    return(result);
}

static void
free_acceleration_struct(struct ms_accel *accel)
{
    free(accel->faces_starts);
    free(accel->verts_starts);
    
    free(accel->edge_indices);
    
    free(accel->faces_matrix);
    free(accel->verts_matrix);
    
    TracyCFree(accel->faces_starts);
    TracyCFree(accel->verts_starts);
    
    TracyCFree(accel->edge_indices);
    
    TracyCFree(accel->faces_matrix);
    TracyCFree(accel->verts_matrix);
}