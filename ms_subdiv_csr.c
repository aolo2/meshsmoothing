static struct ms_accel
init_acceleration_struct2(struct ms_mesh mesh)
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
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int next = vert + 1;
            if (next == mesh.degree) {
                next = 0;
            }
            
            int start = mesh.faces[face * mesh.degree + vert];
            int end = mesh.faces[face * mesh.degree + next];
            
            offsets[start + 1].a += 1;
            offsets[end + 1].a += 1;
        }
    }
    
    for (int v = 1; v < mesh.nverts + 1; ++v) {
        offsets[v].a += offsets[v - 1].a;
    }
    
    TracyCZoneEnd(count_initial_offsets);
    
    
    
    TracyCZoneN(count_unique_edges, "alloc and count unique edges", true);
    
    TracyCZoneN(alloc_unique_edges, "alloc", true);
    
    int *edges = malloc(nedges * sizeof(int));
    int *faces = malloc(nfaces * sizeof(int));
    
    int *edge_indices = malloc(2 * nedges * sizeof(int));
    int *edge_indices_accum = calloc(1, mesh.nverts * sizeof(int));
    
    assert(edges);
    assert(faces);
    assert(edge_indices_accum);
    assert(edge_indices);
    
    TracyCAlloc(edges, nedges * sizeof(int));
    TracyCAlloc(faces, nedges * sizeof(int));
    
    TracyCAlloc(edge_indices, 2 * nedges * sizeof(int));
    TracyCAlloc(edge_indices_accum, mesh.nverts * sizeof(int));
    
    TracyCZoneEnd(alloc_unique_edges);
    
    
    
    
    TracyCZoneN(count_unique_edges_count, "count", true);
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int next = vert + 1;
            if (next == mesh.degree) {
                next = 0;
            }
            
            int start_edge_index = face * mesh.degree + vert;
            int end_edge_index = face * mesh.degree + next;
            
            int start = mesh.faces[start_edge_index];
            int end = mesh.faces[end_edge_index];
            
            struct ms_v4i *offsets_start = offsets + start;
            struct ms_v4i *offsets_end = offsets + end;
            
            /* edge start */
            int edge_base = offsets_start->a;
            int edge_count = offsets_start->b;
            
            int found = -1;
            for (int e = edge_base; e < edge_base + edge_count; ++e) {
                if (edges[e] == end) {
                    found = e;
                    break;
                }
            }
            
            if (found == -1) {
                edges[edge_base + edge_count] = end;
                offsets_start->b += 1;
                edge_indices[(edge_base + edge_count) * 2 + 0] = start_edge_index;
                edge_indices[(edge_base + edge_count) * 2 + 1] = start_edge_index;
            } else {
                edge_indices[found * 2 + 1] = start_edge_index;
            }
            
            /* start face */
            int face_base = offsets_start->a;
            int face_count = offsets_start->d;
            
            found = 0;
            for (int f = face_base; f < face_base + face_count; ++f) {
                if (faces[f] == face) {
                    found = 1;
                    break;
                }
            }
            
            if (!found) {
                faces[face_base + face_count] = face;
                offsets_start->d += 1;
            }
            
            
            /* edge end */
            edge_base = offsets_end->a;
            edge_count = offsets_end->b;
            
            found = -1;
            for (int e = edge_base; e < edge_base + edge_count; ++e) {
                if (edges[e] == start) {
                    found = e;
                    break;
                }
            }
            
            if (found == -1) {
                edges[edge_base + edge_count] = start;
                offsets_end->b += 1;
                edge_indices[(edge_base + edge_count) * 2 + 0] = start_edge_index;
                edge_indices[(edge_base + edge_count) * 2 + 1] = start_edge_index;
            } else {
                edge_indices[found * 2 + 1] = start_edge_index;
            }
            
            /* end face */
            face_base = offsets_end->a;
            face_count = offsets_end->d;
            
            found = 0;
            for (int f = face_base; f < face_base + face_count; ++f) {
                if (faces[f] == face) {
                    found = 1;
                    break;
                }
            }
            
            if (!found) {
                faces[face_base + face_count] = face;
                offsets_end->d += 1;
            }
        }
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
    TracyCFree(offsets);
    
    TracyCZoneEnd(return_and_free);
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}

// init accel structure (two CSRs)
static struct ms_accel
init_acceleration_struct(struct ms_mesh mesh)
{
    TracyCZone(__FUNC__, true);
    
    TracyCZoneN(alloc_initial_offsets, "alloc offset arrays", true);
    int *edges_from = calloc(1, (mesh.nverts + 1) * sizeof(int));
    
    TracyCAlloc(edges_from, (mesh.nverts + 1) * sizeof(int));
    TracyCZoneEnd(alloc_initial_offsets);
    
    assert(edges_from);
    
    TracyCZoneN(count_initial_offsets, "count all edges", true);
    int nedges = 0;
    int nfaces = 0;
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int next = vert + 1;
            if (next == mesh.degree) {
                next = 0;
            }
            
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
    int *edge_indices_accum = calloc(1, mesh.nverts * sizeof(int));
    
    assert(edges);
    assert(faces);
    assert(edge_indices_accum);
    assert(edge_indices);
    
    TracyCAlloc(edges, nedges * sizeof(int));
    TracyCAlloc(faces, nedges * sizeof(int));
    
    TracyCAlloc(edges_accum, mesh.nverts * sizeof(int));
    TracyCAlloc(faces_accum, mesh.nverts * sizeof(int));
    
    TracyCAlloc(edge_indices, 2 * nedges * sizeof(int));
    TracyCAlloc(edge_indices_accum, mesh.nverts * sizeof(int));
    
    TracyCZoneEnd(alloc_unique_edges);
    
    TracyCZoneN(count_unique_edges_count, "count", true);
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int next = vert + 1;
            if (next == mesh.degree) {
                next = 0;
            }
            
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
    
    assert(faces_from);
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