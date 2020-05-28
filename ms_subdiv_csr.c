static inline void
add_edge_and_face(struct ms_v4i *offsets, int *edges, int *faces, int *edge_indices,
                  int end, int face, int edge_index)
{
    /* edge */
    int edge_base = offsets->a;
    int edge_count = offsets->b;
    
    int face_count = offsets->d;
    int face_base = edge_base;
    
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

static struct ms_accel
init_acceleration_struct(struct ms_mesh mesh, struct ms_v3 *face_points)
{
    TracyCZone(__FUNC__, true);
    
    TracyCZoneN(alloc_initial_offsets, "alloc offset arrays", true);
    
    struct ms_v4i *offsets = NULL;
    posix_memalign((void **) &offsets, 64, (mesh.nverts + 1) * sizeof(struct ms_v4i));
    
    assert(offsets);
    
    memset(offsets, 0x00, (mesh.nverts + 1) * sizeof(struct ms_v4i));
    
    TracyCAlloc(offsets, (mesh.nverts + 1) * sizeof(struct ms_v4i));
    
    TracyCZoneEnd(alloc_initial_offsets);
    
    TracyCZoneN(count_initial_offsets, "count all edges", true);
    
    int nedges = mesh.nfaces * mesh.degree * 2;
    int nfaces = mesh.nfaces * mesh.degree * 2;
    int nfaces4 = mesh.nfaces * 4;
    
    for (int face = 0; face < nfaces4; face += 4) {
        int start = mesh.faces[face + 0];
        int end = mesh.faces[face + 1];
        
        offsets[start + 1].a += 1;
        offsets[end + 1].a += 1;
        
        start = end;
        end = mesh.faces[face + 2];
        
        offsets[start + 1].a += 1;
        offsets[end + 1].a += 1;
        
        start = end;
        end = mesh.faces[face + 3];
        
        offsets[start + 1].a += 1;
        offsets[end + 1].a += 1;
        
        start = end;
        end = mesh.faces[face + 0];
        
        offsets[start + 1].a += 1;
        offsets[end + 1].a += 1;
    }
    
    for (int v = 1; v < mesh.nverts + 1; ++v) {
        offsets[v].a += offsets[v - 1].a;
    }
    
    TracyCZoneEnd(count_initial_offsets);
    
    
    
    TracyCZoneN(count_unique_edges, "alloc and count unique edges", true);
    
    TracyCZoneN(alloc_unique_edges, "alloc", true);
    
    int *edges = NULL;
    int *faces = NULL;
    
    int *edge_indices = NULL;
    int *edge_indices_accum = NULL;
    
    int *face_counts = NULL;
    
    struct ms_v3 *fp_averages = NULL;
    
    posix_memalign((void **) &edges, 64, nedges * sizeof(int));
    posix_memalign((void **) &faces, 64, nfaces * sizeof(int));
    
    posix_memalign((void **) &edge_indices, 64, 2 * nedges * sizeof(int));
    posix_memalign((void **) &edge_indices_accum, 64, mesh.nverts * sizeof(int));
    
    posix_memalign((void **) &fp_averages, 64, mesh.nverts * sizeof(struct ms_v3));
    posix_memalign((void **) &face_counts, 64, mesh.nverts * sizeof(int));
    
    assert(edges);
    assert(faces);
    assert(edge_indices_accum);
    assert(edge_indices);
    assert(fp_averages);
    assert(face_counts);
    
    memset(edge_indices_accum, 0x00, mesh.nverts * sizeof(int));
    
    TracyCAlloc(edges, nedges * sizeof(int));
    TracyCAlloc(faces, nedges * sizeof(int));
    
    TracyCAlloc(edge_indices, 2 * nedges * sizeof(int));
    TracyCAlloc(edge_indices_accum, mesh.nverts * sizeof(int));
    
    TracyCZoneEnd(alloc_unique_edges);
    
    TracyCZoneN(count_unique_edges_count, "count", true);
    
    for (int face = 0; face < nfaces4; face += 4) {
        int start = mesh.faces[face + 0];
        int end = mesh.faces[face + 1];
        int actual_face = face >> 2;
        
        add_edge_and_face(offsets + start, edges, faces, edge_indices,
                          end, actual_face, face);
        
        add_edge_and_face(offsets + end, edges, faces, edge_indices,
                          start, actual_face, face);
        
        start = end;
        end = mesh.faces[face + 2];
        
        add_edge_and_face(offsets + start, edges, faces, edge_indices,
                          end, actual_face, face + 1);
        
        add_edge_and_face(offsets + end, edges, faces, edge_indices,
                          start, actual_face, face + 1);
        
        start = end;
        end = mesh.faces[face + 3];
        
        add_edge_and_face(offsets + start, edges, faces, edge_indices,
                          end, actual_face, face + 2);
        
        add_edge_and_face(offsets + end, edges, faces, edge_indices,
                          start, actual_face, face + 2);
        
        start = end;
        end = mesh.faces[face + 0];
        
        add_edge_and_face(offsets + start, edges, faces, edge_indices,
                          end, actual_face, face + 3);
        
        add_edge_and_face(offsets + end, edges, faces, edge_indices,
                          start, actual_face, face + 3);
    }
    
    for (int i = 0; i < mesh.nverts + 1; ++i) {
        offsets[i].c = offsets[i].a;
    }
    
    TracyCZoneEnd(count_unique_edges_count);
    TracyCZoneEnd(count_unique_edges);
    
    struct ms_edge *packed_edges = NULL;
    posix_memalign((void **) &packed_edges, 64, nedges * sizeof(struct ms_edge));
    
    assert(packed_edges);
    
    TracyCZoneN(tight_pack, "pack unique edges", true);
    /* Tighter! */
    int edges_head = 0;
    int faces_head = 0;
    
    for (int v = 0; v < mesh.nverts; ++v) {
        int e_from = offsets[v].a;
        int e_count = offsets[v].b;
        int f_from = offsets[v].c;
        int f_count = offsets[v].d;
        
        struct ms_v3 startv = mesh.vertices[v];
        
        struct ms_v3 mid_point = { 0 };
        struct ms_v3 face_point = { 0 };
        
        offsets[v].a = edges_head;
        offsets[v].c = faces_head;
        
        for (int i = 0; i < e_count; ++i) {
            int end = edges[e_from + i];
            struct ms_v3 endv = mesh.vertices[end];
            
            int edge_index_1 = edge_indices[(e_from + i) * 2 + 0];
            int edge_index_2 = edge_indices[(e_from + i) * 2 + 1];
            
            edges[edges_head] = end;
            
            struct ms_edge *edge = packed_edges + edges_head;
            
            edge->endv = mesh.vertices[end];
            edge->edge_index_1 = edge_index_1;
            edge->edge_index_2 = edge_index_2;
            edge->end = end;
            
            ++edges_head;
            
            mid_point.x += (startv.x + endv.x) * 0.5f;
            mid_point.y += (startv.y + endv.y) * 0.5f;
            mid_point.z += (startv.z + endv.z) * 0.5f;
        }
        
        for (int i = 0; i < f_count; ++i) {
            int face = faces[f_from + i];
            
            faces[faces_head++] = face;
            
            face_point.x += face_points[face].x;
            face_point.y += face_points[face].y;
            face_point.z += face_points[face].z;
        }
        
        if (f_count > 0) {
            face_point.x /= f_count;
            face_point.y /= f_count;
            face_point.z /= f_count;
        }
        
        if (e_count > 0) {
            mid_point.x /= e_count; 
            mid_point.y /= e_count; 
            mid_point.z /= e_count; 
        }
        
        /* Weights */
        f32 w1 = (f32) (f_count - 3) / f_count;
        f32 w2 = 1.0f / f_count;
        f32 w3 = 2.0f * w2;
        
        fp_averages[v].x = w1 * startv.x + w2 * face_point.x + w3 * mid_point.x;
        fp_averages[v].y = w1 * startv.y + w2 * face_point.y + w3 * mid_point.y;
        fp_averages[v].z = w1 * startv.z + w2 * face_point.z + w3 * mid_point.z;
        
        face_counts[v] = f_count;
    }
    
    offsets[mesh.nverts].a = edges_head;
    offsets[mesh.nverts].c = faces_head;
    
    TracyCZoneEnd(tight_pack);
    
    TracyCZoneN(repack_offsets, "repack offset arrays", true);
    int *edges_from = malloc((mesh.nverts + 1) * sizeof(int));
    
    TracyCAlloc(edges_from, (mesh.nverts + 1) * sizeof(int));
    
    for (int i = 0; i < mesh.nverts + 1; ++i) {
        edges_from[i] = offsets[i].a;
    }
    
    TracyCZoneEnd(repack_offsets);
    
    TracyCZoneN(return_and_free, "free memory", true);
    
    struct ms_accel result = { 0 };
    
    result.verts_starts = edges_from;
    result.verts_matrix = edges;
    result.face_counts = face_counts;
    
    result.fp_averages = fp_averages;
    result.pack = packed_edges;
    
    free(offsets);
    free(edge_indices_accum);
    
    TracyCFree(offsets);
    TracyCFree(edge_indices_accum);
    
    TracyCZoneEnd(return_and_free);
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}

static int
edge_accel_index(struct ms_accel *accel, int start, int end)
{
    int start_from = accel->verts_starts[start];
    int start_to = accel->verts_starts[start + 1];
    
    for (int e = start_from; e < start_to; ++e) {
        int some_end = accel->verts_matrix[e];
        
        if (some_end == end) {
            return(e);
        }
    }
    
    return(-1);
}

static struct ms_v2i
both_edge_indices(struct ms_accel *accel, int start, int end)
{
    int start_from = accel->verts_starts[start];
    int start_to = accel->verts_starts[start + 1];
    
    struct ms_v2i result = { 0 };
    
    for (int e = start_from; e < start_to; ++e) {
        int some_end = accel->pack[e].end;
        
        if (some_end == end) {
            int index_1 = accel->pack[e].edge_index_1;
            int index_2 = accel->pack[e].edge_index_2;
            
            result.a = index_1;
            result.b = index_2;
            
            break;
        }
    }
    
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
            int index_1 = accel->pack[e].edge_index_1;
            int index_2 = accel->pack[e].edge_index_2;
            
            result.a = index_1 >> 2;
            result.b = index_2 >> 2;
            
            break;
        }
    }
    
    return(result);
}

static void
free_acceleration_struct(struct ms_accel *accel)
{
    free(accel->verts_starts);
    free(accel->verts_matrix);
    
    TracyCFree(accel->verts_starts);
    TracyCFree(accel->verts_matrix);
}