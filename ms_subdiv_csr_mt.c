static inline void
add_face(int *faces_from, int *faces_accum, int *faces,
         int face, int vert)
{
    int face_base = faces_from[vert];
    int face_count = faces_accum[vert];
    
    bool found = false;
    for (int f = face_base; f < face_base + face_count; ++f) {
        if (faces[f] == face) {
            found = true;
            break;
        }
    }
    
    if (!found) {
        faces[face_base + face_count] = face;
        faces_accum[vert] += 1;
    }
}

static inline void
add_edge(int *edges_from, int *edges_accum, int *edges, int *edge_indices,
         int edge_index, int start, int end)
{
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
        edge_indices[(edge_base + edge_count) * 2 + 0] = edge_index;
        edge_indices[(edge_base + edge_count) * 2 + 1] = edge_index;
    } else {
        edge_indices[found * 2 + 1] = edge_index;
    }
}

static void
insert_edge_and_face(int *edges_from, int *edges_accum, int *edges,
                     int *edge_indices, int edge_index,
                     int *faces_from, int *faces_accum, int *faces,
                     int start, int end, int face,
                     int this_tid_process_from, int this_tid_process_to)
{
    if (this_tid_process_from <= start && start < this_tid_process_to) {
        add_edge(edges_from, edges_accum, edges, edge_indices, edge_index, start, end);
        add_face(faces_from, faces_accum, faces, face, start);
    }
    
    if (this_tid_process_from <= end && end < this_tid_process_to) {
        add_edge(edges_from, edges_accum, edges, edge_indices, edge_index, end, start);
        add_face(faces_from, faces_accum, faces, face, end);
    }
}

// init accel structure (two CSRs)
static struct ms_accel
init_acceleration_struct_mt(struct ms_mesh mesh)
{
    TracyCZone(__FUNC__, true);
    
    int nthreads = NTHREADS;
    
    int nedges = mesh.nfaces * mesh.degree * 2;
    int nfaces = nedges;
    
    int *edges_from = calloc(1, (mesh.nverts + 1) * sizeof(int));
    TracyCAlloc(edges_from, (mesh.nverts + 1) * sizeof(int));
    
    int **edges_from_locals = malloc(nthreads * sizeof(int *));
    TracyCAlloc(edges_from_locals, nthreads * sizeof(int *));
    
#pragma omp parallel
    {
        TracyCZoneN(count_initial_offsets, "count all edges", true);
        
        int tid = omp_get_thread_num();
        
        edges_from_locals[tid] = calloc(1, (mesh.nverts + 1) * sizeof(int));
        TracyCAlloc(edges_from_locals[tid], (mesh.nverts + 1) * sizeof(int));
        
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
    }
    
    TracyCZoneN(reduce_offsets, "reduce offsets", true);
    for (int t = 0; t < nthreads; ++t) {
        for (int v = 1; v < mesh.nverts + 1; ++v) {
            edges_from[v] += edges_from_locals[t][v];
        }
    }
    TracyCZoneEnd(reduce_offsets);
    
    TracyCZoneN(propogate_offsets, "propogate offsets", true);
    for (int v = 1; v < mesh.nverts + 1; ++v) {
        edges_from[v] += edges_from[v - 1];
    }
    TracyCZoneEnd(propogate_offsets);
    
    int *faces_from = edges_from;
    
    
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
    
#pragma omp parallel
    {
        TracyCZoneN(count_unique_edges_count, "count", true);
        
        int tid = omp_get_thread_num();
        int block_size = mesh.nverts / nthreads;
        
        int this_tid_process_from = tid * block_size;
        int this_tid_process_to = this_tid_process_from + block_size;
        
        if (tid == nthreads - 1) {
            this_tid_process_to = mesh.nverts;
        }
        
        for (int face = 0; face < mesh.nfaces; ++face) {
            for (int vert = 0; vert < mesh.degree; ++vert) {
                int next = (vert + 1) % mesh.degree;
                
                int start_edge_index = face * mesh.degree + vert;
                int end_edge_index = face * mesh.degree + next;
                
                int start = mesh.faces[start_edge_index];
                int end = mesh.faces[end_edge_index];
                
                insert_edge_and_face(edges_from, edges_accum, edges,
                                     edge_indices, start_edge_index,
                                     faces_from, faces_accum, faces,
                                     start, end, face,
                                     this_tid_process_from, this_tid_process_to);
            }
        }
        
        TracyCZoneEnd(count_unique_edges_count);
    }
    
    faces_from = malloc((mesh.nverts + 1) * sizeof(int));
    
    TracyCAlloc(faces_from, (mesh.nverts + 1) * sizeof(int));
    
    memcpy(faces_from, edges_from, (mesh.nverts + 1) * sizeof(int));
    
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
    
    for (int t = 0; t < nthreads; ++t) {
        free(edges_from_locals[t]);
        TracyCFree(edges_from_locals[t]);
    }
    
    free(edges_from_locals);
    TracyCFree(edges_from_locals);
    
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