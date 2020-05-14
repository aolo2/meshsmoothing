static struct ms_mesh
ms_subdiv_catmull_clark_new(struct ms_mesh mesh)
{
    TracyCZoneS(__FUNC__, true, CALLSTACK_DEPTH);
    
    int nthreads = NTHREADS;
    omp_set_num_threads(NTHREADS);
    
    struct ms_v3 *face_points = malloc(mesh.nfaces * sizeof(struct ms_v3));
    
    assert(face_points);
    
    /* Construct acceleration structure */
    struct ms_accel accel = init_acceleration_struct_mt(mesh);
    
    TracyCZoneNS(alloc_everything, "allocate", true, CALLSTACK_DEPTH);
    
    struct ms_mesh new_mesh = { 0 };
    new_mesh.nfaces = mesh.nfaces * 4;
    new_mesh.faces = malloc(new_mesh.nfaces * 4 * sizeof(int));
    new_mesh.degree = 4;
    
    assert(new_mesh.faces);
    
    int vert_base = 0;
    int nedge_pointsv = 0;
    struct ms_v3 *new_verts = malloc(mesh.nverts * sizeof(struct ms_v3));
    int nedges = accel.verts_starts[mesh.nverts];
    int *edge_points = malloc(mesh.nfaces * mesh.degree * sizeof(int));
    int *nedges_per_thread = malloc((nthreads + 1) * sizeof(int));
    struct ms_v3 *edge_pointsv = malloc(nedges * 2 * sizeof(struct ms_v3));
    
    assert(new_verts);
    assert(edge_points);
    assert(nedges_per_thread);
    assert(edge_pointsv);
    
    memset(edge_points, -1, mesh.nfaces * mesh.degree);
    
    TracyCAllocS(edge_points, mesh.nfaces * mesh.degree * sizeof(struct ms_v3), CALLSTACK_DEPTH);
    TracyCAllocS(edge_pointsv, nedges * 2 * sizeof(struct ms_v3), CALLSTACK_DEPTH);
    TracyCAllocS(new_verts, mesh.nverts * sizeof(struct ms_v3), CALLSTACK_DEPTH);
    TracyCAllocS(face_points, mesh.nfaces * sizeof(struct ms_v3), CALLSTACK_DEPTH);
    TracyCAllocS(new_mesh.faces, new_mesh.nfaces * 4 * sizeof(int), CALLSTACK_DEPTH);
    
    TracyCZoneEnd(alloc_everything);
    
#pragma omp barrier
    
    /* Face points */
    f32 one_over_mesh_degree = 1.0f / mesh.degree;
    
#pragma omp parallel
    {
        TracyCZoneNS(compute_face_points, "face points", true, CALLSTACK_DEPTH);
        
#pragma omp for
        for (int face = 0; face < mesh.nfaces; ++face) {
            struct ms_v3 fp = { 0 };
            for (int vert = 0; vert < mesh.degree; ++vert) {
                struct ms_v3 vertex = mesh.vertices[mesh.faces[face * mesh.degree + vert]];
                fp.x += vertex.x;
                fp.y += vertex.y;
                fp.z += vertex.z;
            }
            
            fp.x *= one_over_mesh_degree;
            fp.y *= one_over_mesh_degree;
            fp.z *= one_over_mesh_degree;
            
            face_points[face] = fp;
        }
        
        TracyCZoneEnd(compute_face_points);
        
        /* Edge points */
        TracyCZoneNS(count_ep_work, "count edge point work", true, CALLSTACK_DEPTH);
        int tid = omp_get_thread_num();
        int block_size = mesh.nverts / nthreads;
        
        int this_tid_process_from = tid * block_size;
        int this_tid_process_to = this_tid_process_from + block_size;
        
        if (tid == nthreads - 1) {
            this_tid_process_to = mesh.nverts;
        }
        
        int this_tid_nedges = 0;
        
        for (int start = this_tid_process_from; start < this_tid_process_to; ++start) {
            int from = accel.verts_starts[start];
            int to = accel.verts_starts[start + 1];
            this_tid_nedges += (to - from);
        }
        
        
        nedges_per_thread[tid + 1] = this_tid_nedges;
        TracyCZoneEnd(count_ep_work);
        
#pragma omp barrier
        
#pragma omp single
        {
            nedges_per_thread[0] = 0;
            for (int t = 1; t < nthreads + 1; ++t) {
                nedges_per_thread[t] += nedges_per_thread[t - 1];
            }
            
            nedge_pointsv = nedges_per_thread[nthreads];
        }
        
        
#pragma omp barrier
        TracyCZoneNS(compute_edge_points, "edge_points", true, CALLSTACK_DEPTH);
        int edge_pointsv_offset = nedges_per_thread[tid];
        
        for (int start = this_tid_process_from; start < this_tid_process_to; ++start) {
            struct ms_v3 startv = mesh.vertices[start];
            
            int from = accel.verts_starts[start];
            int to = accel.verts_starts[start + 1];
            
            for (int e = from; e < to; ++e) {
                int end = accel.verts_matrix[e];
                struct ms_v3 endv = mesh.vertices[end];
                
                /* edge_index_2 might be equal to edge_index_1 if the edge is unique */
                int edge_index_1 = accel.edge_indices[2 * e + 0];
                int edge_index_2 = accel.edge_indices[2 * e + 1];
                
                int face = edge_index_1 >> 2;
                int adj  = edge_index_2 >> 2;
                
                struct ms_v3 edge_point;
                
                if (adj != face) {
                    struct ms_v3 face_point_me = face_points[face];
                    struct ms_v3 face_point_adj = face_points[adj];
                    
                    edge_point.x = (face_point_me.x + face_point_adj.x + startv.x + endv.x) * 0.25f;
                    edge_point.y = (face_point_me.y + face_point_adj.y + startv.y + endv.y) * 0.25f;
                    edge_point.z = (face_point_me.z + face_point_adj.z + startv.z + endv.z) * 0.25f;
                } else {
                    /* This is an edge of a hole */
                    edge_point.x = (startv.x + endv.x) * 0.5f;
                    edge_point.y = (startv.y + endv.y) * 0.5f;
                    edge_point.z = (startv.z + endv.z) * 0.5f;
                }
                
                edge_points[edge_index_1] = edge_pointsv_offset;
                edge_points[edge_index_2] = edge_pointsv_offset; /* REMINDER: edge_index_2 might be equal to edge_index_1 if the edge is unique */
                
                edge_pointsv[edge_pointsv_offset] = edge_point;
                
                ++edge_pointsv_offset;
            }
        }
        
        
        TracyCZoneEnd(compute_edge_points);
        
        TracyCZoneNS(update_positions, "update old points", true, CALLSTACK_DEPTH);
        //int DBG_count = 0;
        
#pragma omp for
        for (int v = 0; v < mesh.nverts; ++v) {
            //++DBG_count;
            struct ms_v3 vertex = mesh.vertices[v];
            struct ms_v3 new_vert;
            
            int adj_verts_base = accel.verts_starts[v];
            int adj_faces_base = accel.faces_starts[v];
            
            int adj_verts_count = accel.verts_starts[v + 1] - adj_verts_base;
            int adj_faces_count = accel.faces_starts[v + 1] - adj_faces_base;
            
            f32 one_over_adj_faces_count = 1.0f / adj_faces_count;
            
            if (adj_faces_count != adj_verts_count) {
                /* This vertex is on an edge of a hole */
                int nedges_adj_to_hole = 0;
                struct ms_v3 avg_mid_edge_point = { 0 };
                
                for (int i = 0; i < adj_verts_count; ++i) {
                    int start = v;
                    int end = accel.verts_matrix[adj_verts_base + i];
                    
                    struct ms_v3 startv = mesh.vertices[start];
                    struct ms_v3 endv = mesh.vertices[end];
                    
                    /* Only take into account edges that are also on the edge of a hole */
                    struct ms_v2i adj_faces = edge_adjacent_faces(&accel, v, end);
                    int adj_face = adj_faces.a;
                    int another_adj_face = adj_faces.b;
                    
                    if (adj_face == another_adj_face) {
                        ++nedges_adj_to_hole;
                        avg_mid_edge_point.x += (startv.x + endv.x);
                        avg_mid_edge_point.y += (startv.y + endv.y);
                        avg_mid_edge_point.z += (startv.z + endv.z);
                    }
                }
                
                f32 one_over_adj_to_hole = 1.0f / (nedges_adj_to_hole + 1);
                
                avg_mid_edge_point.x *= 0.5f;
                avg_mid_edge_point.y *= 0.5f;
                avg_mid_edge_point.z *= 0.5f;
                
                new_vert.x = (avg_mid_edge_point.x + vertex.x) * one_over_adj_to_hole;
                new_vert.y = (avg_mid_edge_point.y + vertex.y) * one_over_adj_to_hole;
                new_vert.z = (avg_mid_edge_point.z + vertex.z) * one_over_adj_to_hole;
            } else {
                /* Average of face points of all the faces this vertex is adjacent to */
                struct ms_v3 avg_face_point = { 0 };
                for (int i = 0; i < adj_faces_count; ++i) {
                    struct ms_v3 fp = face_points[accel.faces_matrix[adj_faces_base + i]];
                    avg_face_point.x += fp.x;
                    avg_face_point.y += fp.y;
                    avg_face_point.z += fp.z;
                }
                avg_face_point.x *= one_over_adj_faces_count;
                avg_face_point.y *= one_over_adj_faces_count;
                avg_face_point.z *= one_over_adj_faces_count;
                
                /* Average of mid points of all the edges this vertex is adjacent to */
                struct ms_v3 avg_mid_edge_point = { 0 };
                for (int i = 0; i < adj_verts_count; ++i) {
                    int start = v;
                    int end = accel.verts_matrix[adj_verts_base + i];
                    
                    struct ms_v3 startv = mesh.vertices[start];
                    struct ms_v3 endv = mesh.vertices[end];
                    
                    avg_mid_edge_point.x += (startv.x + endv.x) * 0.5f;
                    avg_mid_edge_point.y += (startv.y + endv.y) * 0.5f;
                    avg_mid_edge_point.z += (startv.z + endv.z) * 0.5f;
                }
                
                f32 norm_coeff = 1.0f / adj_verts_count;
                avg_mid_edge_point.x *= norm_coeff;
                avg_mid_edge_point.y *= norm_coeff;
                avg_mid_edge_point.z *= norm_coeff;
                
                /* Weights */
                f32 w1 = (f32) (adj_faces_count - 3) * one_over_adj_faces_count;
                f32 w2 = one_over_adj_faces_count;
                f32 w3 = 2.0f * w2;
                
                /* Weighted average to obtain a new vertex */
                new_vert.x = w1 * vertex.x + w2 * avg_face_point.x + w3 * avg_mid_edge_point.x;
                new_vert.y = w1 * vertex.y + w2 * avg_face_point.y + w3 * avg_mid_edge_point.y;
                new_vert.z = w1 * vertex.z + w2 * avg_face_point.z + w3 * avg_mid_edge_point.z;
            }
            
            new_verts[v] = new_vert;
        }
        
        //printf("%d] %d\n", tid, DBG_count);
        TracyCZoneEnd(update_positions);
        
        
#pragma omp single
        {
            TracyCZoneNS(alloc_new_mesh, "allocate new mesh", true, CALLSTACK_DEPTH);
            
            /* Updated vertices + edge points + 1 face point per face */
            new_mesh.nverts = mesh.nverts + nedge_pointsv + mesh.nfaces;
            new_mesh.vertices = malloc(new_mesh.nverts * sizeof(struct ms_v3));
            
            assert(new_mesh.vertices);
            
            TracyCAllocS(new_mesh.vertices, new_mesh.nverts * sizeof(struct ms_v3), CALLSTACK_DEPTH);
            
            TracyCZoneEnd(alloc_new_mesh);
            
            
            
            vert_base = mesh.nverts + nedge_pointsv;
        }
        
        TracyCZoneNS(copy_data, "copy unique points", true, CALLSTACK_DEPTH);
        
        int copy_nv_block = mesh.nverts / nthreads;
        int copy_ep_block = nedge_pointsv / nthreads;
        int copy_fp_block = mesh.nfaces / nthreads;
        
        int my_nv = copy_nv_block;
        int my_ep = copy_ep_block;
        int my_fp = copy_fp_block;
        
        if (tid == nthreads - 1) {
            my_nv += mesh.nverts % nthreads;
            my_ep += nedge_pointsv % nthreads;
            my_fp += mesh.nfaces % nthreads;
        }
        
        memcpy(new_mesh.vertices + tid * copy_nv_block,
               new_verts + tid * copy_nv_block,
               my_nv * sizeof(struct ms_v3));
        
        memcpy(new_mesh.vertices + mesh.nverts + tid * copy_ep_block,
               edge_pointsv + tid * copy_ep_block,
               my_ep * sizeof(struct ms_v3));
        
        memcpy(new_mesh.vertices + vert_base + tid * copy_fp_block,
               face_points + tid * copy_fp_block,
               my_fp * sizeof(struct ms_v3));
        
        TracyCZoneEnd(copy_data);
        
        
        int edgep_base = mesh.nverts;
        
        /* Subdivide */
        TracyCZoneNS(subdivide, "do subdivision", true, CALLSTACK_DEPTH);
#pragma omp for schedule(guided)
        for (int face = 0; face < mesh.nfaces * 4; face += 4) {
            int face_base = face << 2;
            int facep_index = vert_base + (face >> 2);
            
            int edge_point_ab = edgep_base + edge_points[face + 0];
            int edge_point_bc = edgep_base + edge_points[face + 1];
            int edge_point_cd = edgep_base + edge_points[face + 2];
            int edge_point_da = edgep_base + edge_points[face + 3];
            
            int a = mesh.faces[face + 0];
            int b = mesh.faces[face + 1];
            int c = mesh.faces[face + 2];
            int d = mesh.faces[face + 3];
            
            /* Add faces */
            {
                /* face 0 */
                new_mesh.faces[face_base + 0] = a;
                new_mesh.faces[face_base + 1] = edge_point_ab;
                new_mesh.faces[face_base + 2] = facep_index;
                new_mesh.faces[face_base + 3] = edge_point_da;
                
                /* face 1 */
                new_mesh.faces[face_base + 4] = b;
                new_mesh.faces[face_base + 5] = edge_point_bc;
                new_mesh.faces[face_base + 6] = facep_index;
                new_mesh.faces[face_base + 7] = edge_point_ab;
                
                /* face 2 */
                new_mesh.faces[face_base + 8] = c;
                new_mesh.faces[face_base + 9] = edge_point_cd;
                new_mesh.faces[face_base + 10] = facep_index;
                new_mesh.faces[face_base + 11] = edge_point_bc;
                
                /* face 3 */
                new_mesh.faces[face_base + 12] = d;
                new_mesh.faces[face_base + 13] = edge_point_da;
                new_mesh.faces[face_base + 14] = facep_index;
                new_mesh.faces[face_base + 15] = edge_point_cd;
            }
        }
        
        TracyCZoneEnd(subdivide);
    }
    
    free_acceleration_struct(&accel);
    
    free(edge_pointsv);
    free(new_verts);
    free(face_points);
    free(edge_points);
    free(nedges_per_thread);
    
    TracyCFree(edge_pointsv);
    TracyCFree(new_verts);
    TracyCFree(face_points);
    TracyCFree(edge_points);
    
    TracyCZoneEnd(__FUNC__);
    
    return(new_mesh);
}