static inline void
add_edges_and_face(struct ms_v3i *offsets, int *edges, int *faces, int *edge_indices,
                   int end_1, int end_2, int face, int edge_index_1, int edge_index_2)
{
    /* edge */
    int edge_base = offsets->a;
    int edge_count = offsets->b;
    
    int face_count = offsets->c;
    int face_base = edge_base;
    
    int found_1 = -1;
    int found_2 = -1;
    
    for (int e = edge_base; e < edge_base + edge_count; ++e) {
        if (edges[e] == end_1) {
            found_1 = e;
        } else if (edges[e] == end_2) {
            found_2 = e;
        }
        
        //if (found_1 != -1 && found_2 != -1) { break; }
    }
    
    if (found_1 == -1) {
        offsets->b += 1;
        edges[edge_base + edge_count] = end_1;
        edge_indices[(edge_base + edge_count) * 2 + 0] = edge_index_1;
        edge_indices[(edge_base + edge_count) * 2 + 1] = edge_index_1;
        ++edge_count;
    } else {
        edge_indices[found_1 * 2 + 1] = edge_index_1;
    }
    
    if (found_2 == -1) {
        offsets->b += 1;
        edges[edge_base + edge_count] = end_2;
        edge_indices[(edge_base + edge_count) * 2 + 0] = edge_index_2;
        edge_indices[(edge_base + edge_count) * 2 + 1] = edge_index_2;
    } else {
        edge_indices[found_2 * 2 + 1] = edge_index_2;
    }
    
    /* face */
    bool found = false;
    for (int f = face_base; f < face_base + face_count + 1; ++f) {
        if (faces[f] == face) {
            found = true;
            break;
        }
    }
    
    if (!found) {
        faces[face_base + face_count] = face;
        offsets->c += 1;
    }
}

static struct ms_accel
init_acceleration_struct(struct ms_mesh mesh, struct ms_v3 *face_points)
{
    TracyCZone(__FUNC__, true);
    
    TracyCZoneN(alloc_initial_offsets, "alloc offset arrays", true);
    
    struct ms_v3i *offsets = NULL;
    int *offsets_a = NULL;
    
    posix_memalign((void **) &offsets, 64, (mesh.nverts + 1) * sizeof(struct ms_v3i));
    posix_memalign((void **) &offsets_a, 64, (mesh.nverts + 1) * sizeof(int));
    
    assert(offsets);
    assert(offsets_a);
    
    memset(offsets, 0x00, (mesh.nverts + 1) * sizeof(struct ms_v3i));
    memset(offsets_a, 0x00, (mesh.nverts + 1) * sizeof(int));
    
    TracyCAlloc(offsets, (mesh.nverts + 1) * sizeof(struct ms_v3i));
    
    TracyCZoneEnd(alloc_initial_offsets);
    
    TracyCZoneN(count_initial_offsets, "count all edges", true);
    
    int nedges = mesh.nfaces * mesh.degree * 2;
    int nfaces = mesh.nfaces * mesh.degree * 2;
    int nfaces4 = mesh.nfaces * 4;
    
    for (int face = 0; face < nfaces4; face += 4) {
        int v1 = mesh.faces[face + 0];
        int v2 = mesh.faces[face + 1];
        int v3 = mesh.faces[face + 2];
        int v4 = mesh.faces[face + 3];
        offsets_a[v1 + 1] += 2;
        offsets_a[v2 + 1] += 2;
        offsets_a[v3 + 1] += 2;
        offsets_a[v4 + 1] += 2;
    }
    
    for (int v = 1; v < mesh.nverts + 1; ++v) {
        offsets_a[v] += offsets_a[v - 1];
        offsets[v].a = offsets_a[v];
    }
    
    free(offsets_a);
    
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
    
    TracyCAlloc(fp_averages, mesh.nverts * sizeof(struct ms_v3));
    TracyCAlloc(face_counts, mesh.nverts * sizeof(int));
    
    TracyCZoneEnd(alloc_unique_edges);
    
    TracyCZoneN(count_unique_edges_count, "count", true);
    
    for (int face = 0; face < nfaces4; face += 4) {
        int v1 = mesh.faces[face + 0];
        int v2 = mesh.faces[face + 1];
        int v3 = mesh.faces[face + 2];
        int v4 = mesh.faces[face + 3];
        int actual_face = face >> 2;
        
        /*

v1 ----- v2 
           |    0    | 
         |3        | 
         |        1| 
         |    2    | 
         v4 ----- v3

*/
        add_edges_and_face(offsets + v1, edges, faces, edge_indices, v2, v4, actual_face, face + 0, face + 3);
        add_edges_and_face(offsets + v2, edges, faces, edge_indices, v3, v1, actual_face, face + 1, face + 0);
        add_edges_and_face(offsets + v3, edges, faces, edge_indices, v4, v2, actual_face, face + 2, face + 1);
        add_edges_and_face(offsets + v4, edges, faces, edge_indices, v1, v3, actual_face, face + 3, face + 2);
    }
    
    TracyCZoneEnd(count_unique_edges_count);
    TracyCZoneEnd(count_unique_edges);
    
    struct ms_edge *packed_edges = NULL;
    posix_memalign((void **) &packed_edges, 64, nedges * sizeof(struct ms_edge));
    TracyCAlloc(packed_edges, nedges * sizeof(struct ms_edge));
    
    int *edges_from = malloc((mesh.nverts + 1) * sizeof(int));
    TracyCAlloc(edges_from, (mesh.nverts + 1) * sizeof(int));
    
    
    assert(packed_edges);
    assert(edges_from);
    
    TracyCZoneN(tight_pack, "pack unique edges", true);
    /* Tighter! */
    int edges_head = 0;
    
    for (int v = 0; v < mesh.nverts; ++v) {
        int from  = offsets[v].a;
        int e_count = offsets[v].b;
        int f_count = offsets[v].c;
        
        face_counts[v] = f_count;
        
        struct ms_v3 startv = mesh.vertices[v];
        
        struct ms_v3 mid_point = { 0 };
        struct ms_v3 face_point = { 0 };
        
        edges_from[v] = edges_head;
        
        for (int e = from; e < from + e_count; ++e) {
            int end = edges[e];
            struct ms_v3 endv = mesh.vertices[end];
            int edge_index_1 = edge_indices[e * 2 + 0];
            int edge_index_2 = edge_indices[e * 2 + 1];
            struct ms_edge *edge = packed_edges + edges_head;
            
            edges[edges_head] = end;
            
            edge->endv = mesh.vertices[end];
            edge->edge_index_1 = edge_index_1;
            edge->edge_index_2 = edge_index_2;
            edge->end = end;
            edge->face_point_1 = face_points[edge_index_1 >> 2];
            edge->face_point_2 = face_points[edge_index_2 >> 2];
            
            ++edges_head;
            
            mid_point.x += (startv.x + endv.x) * 0.5f;
            mid_point.y += (startv.y + endv.y) * 0.5f;
            mid_point.z += (startv.z + endv.z) * 0.5f;
        }
        
        for (int f = from; f < from + f_count; ++f) {
            int face = faces[f];
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
    }
    
    edges_from[mesh.nverts] = edges_head;
    
    TracyCZoneEnd(tight_pack);
    
    TracyCZoneN(repack_offsets, "repack offset arrays", true);
    
    TracyCZoneEnd(repack_offsets);
    
    TracyCZoneN(return_and_free, "free memory", true);
    
    struct ms_accel result = { 0 };
    
    result.verts_starts = edges_from;
    result.verts_matrix = edges;
    result.face_counts = face_counts;
    
    result.fp_averages = fp_averages;
    result.pack = packed_edges;
    
    free(offsets);
    free(faces);
    free(edge_indices);
    free(edge_indices_accum);
    
    TracyCFree(offsets);
    TracyCFree(edge_indices);
    TracyCFree(faces);
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
    free(accel->face_counts);
    
    free(accel->pack);
    
    TracyCFree(accel->verts_starts);
    TracyCFree(accel->verts_matrix);
    TracyCFree(accel->face_counts);
    
    TracyCFree(accel->pack);
}