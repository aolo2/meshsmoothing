static inline void
add_edges_and_face(struct ms_v4i *offsets, int *edges, int *faces, int *edge_indices,
                   int end_1, int end_2, int face, int edge_index_1, int edge_index_2)
{
    /* edge */
    int edge_base = offsets->a;
    int edge_count = offsets->b;
    
    int face_count = offsets->d;
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
        offsets->d += 1;
    }
}

static struct ms_accel
init_acceleration_struct(struct ms_mesh mesh)
{
    TracyCZone(__FUNC__, true);
    
    TracyCZoneN(alloc_initial_offsets, "alloc offset arrays", true);
    
    struct ms_v4i *offsets = cacheline_alloc((mesh.nverts + 1) * sizeof(struct ms_v4i));
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
    
    int *edges = cacheline_alloc(nedges * sizeof(int));
    int *faces = cacheline_alloc(nfaces * sizeof(int));
    
    int *edge_indices = cacheline_alloc(2 * nedges * sizeof(int));
    int *edge_indices_accum = cacheline_alloc(mesh.nverts * sizeof(int));
    
    assert(edges);
    assert(faces);
    assert(edge_indices_accum);
    assert(edge_indices);
    
    memset(edge_indices_accum, 0x00, mesh.nverts * sizeof(int));
    
    TracyCAlloc(edges, nedges * sizeof(int));
    TracyCAlloc(faces, nedges * sizeof(int));
    
    TracyCAlloc(edge_indices, 2 * nedges * sizeof(int));
    TracyCAlloc(edge_indices_accum, mesh.nverts * sizeof(int));
    
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
    
    for (int i = 0; i < mesh.nverts + 1; ++i) {
        offsets[i].c = offsets[i].a;
    }
    
    TracyCZoneEnd(count_unique_edges_count);
    TracyCZoneEnd(count_unique_edges);
    
    TracyCZoneN(tight_pack, "pack unique edges", true);
    /* Tighter! */
    int edges_head = 0;
    int faces_head = 0;
    
    for (int v = 0; v < mesh.nverts; ++v) {
        int e_from = offsets[v].a;
        int e_count = offsets[v].b;
        
        offsets[v].a = edges_head;
        
        for (int i = 0; i < e_count; ++i) {
            edges[edges_head] = edges[e_from + i];
            edge_indices[2 * edges_head + 0] = edge_indices[(e_from + i) * 2 + 0];
            edge_indices[2 * edges_head + 1] = edge_indices[(e_from + i) * 2 + 1];
            ++edges_head;
        }
    }
    
    offsets[mesh.nverts].a = edges_head;
    
    for (int v = 0; v < mesh.nverts; ++v) {
        int f_from = offsets[v].c;
        int f_count = offsets[v].d;
        
        offsets[v].c = faces_head;
        
        for (int i = 0; i < f_count; ++i) {
            faces[faces_head++] = faces[f_from + i];
        }
    }
    
    offsets[mesh.nverts].c = faces_head;
    
    TracyCZoneEnd(tight_pack);
    
    TracyCZoneN(repack_offsets, "repack offset arrays", true);
    int *edges_from = malloc((mesh.nverts + 1) * sizeof(int));
    int *faces_from = malloc((mesh.nverts + 1) * sizeof(int));
    
    TracyCAlloc(edges_from, (mesh.nverts + 1) * sizeof(int));
    TracyCAlloc(faces_from, (mesh.nverts + 1) * sizeof(int));
    
    for (int i = 0; i < mesh.nverts + 1; ++i) {
        edges_from[i] = offsets[i].a;
        faces_from[i] = offsets[i].c;
    }
    
    TracyCZoneEnd(repack_offsets);
    
    
    TracyCZoneN(return_and_free, "free memory", true);
    
    struct ms_accel result = { 0 };
    
    result.verts_starts = edges_from;
    result.faces_starts = faces_from;
    
    result.faces_matrix = faces;
    result.verts_matrix = edges;
    
    result.edge_indices = edge_indices;
    
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
        int some_end = accel->verts_matrix[e];
        
        if (some_end == end) {
            int index_1 = accel->edge_indices[2 * e + 0];
            int index_2 = accel->edge_indices[2 * e + 1];
            
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