#define OLD_INIT 0

// NOTE: csr.

struct ms_accel {
    int *faces_starts;
    int *verts_starts;
    
    int *faces_count;
    int *verts_count;
    
    int *faces_matrix;
    int *verts_matrix;
};

// TODO: if we converge on this implementation as the fastest, we shouldn't
// return ms_vec's?

// init accel structure
static struct ms_accel
init_hashtable(struct ms_mesh mesh)
{
    // TODO: is there a faster way of detecting duplicates?
    
    TracyCZone(__FUNC__, true);
    
#if OLD_INIT
    TracyCZoneN(count_unique, "count unique neighbours", true);
    struct ms_vec *verts = calloc(1, mesh.nverts * sizeof(struct ms_vec));
    struct ms_vec *faces = calloc(1, mesh.nverts * sizeof(struct ms_vec));
    
    
    // NOTE: count everything to allocate memory
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int next = (vert + 1) % mesh.degree;
            
            int start = mesh.faces[face * mesh.degree + vert];
            int end = mesh.faces[face * mesh.degree + next];
            
            ms_vec_unique_push(verts + start, end);
            ms_vec_unique_push(verts + end, start);
            
            ms_vec_unique_push(faces + start, face);
            ms_vec_unique_push(faces + end, face);
        }
    }
    TracyCZoneEnd(count_unique);
    
    
    TracyCZoneN(fill_starts, "create first CSR array", true);
    int total_faces = 0;
    int total_verts = 0;
    
    int *verts_starts = calloc(1, (mesh.nverts + 1) * sizeof(int));
    int *faces_starts = calloc(1, (mesh.nverts + 1) * sizeof(int));
    
    for (int v = 0; v < mesh.nverts; ++v) {
        total_verts += verts[v].len;
        total_faces += faces[v].len;
        verts_starts[v + 1] = total_verts;
        faces_starts[v + 1] = total_faces;
    }
    TracyCZoneEnd(fill_starts);
    
    TracyCZoneN(fill_matrix, "create second CSR array", true);
    int *verts_matrix = malloc(total_verts * sizeof(int));
    int *faces_matrix = malloc(total_faces * sizeof(int));
    
    for (int v = 0; v < mesh.nverts; ++v) {
        int verts_from = verts_starts[v];
        int faces_from = faces_starts[v];
        
        for (int i = 0; i < faces[v].len; ++i) {
            faces_matrix[faces_from + i] = faces[v].data[i];
        }
        
        for (int i = 0; i < verts[v].len; ++i) {
            verts_matrix[verts_from + i] = verts[v].data[i];
        }
    }
    TracyCZoneEnd(fill_matrix);
    
    struct ms_accel result = { 0 };
    
    result.faces_starts = faces_starts;
    result.verts_starts = verts_starts;
    result.faces_matrix = faces_matrix;
    result.verts_matrix = verts_matrix;
#else
    /*
Attempt at a more cache friendly CSR constriction routine.
 First count upper limit on array lengths. Then collapse dublicates (leaves holes)
*/
    int *edges_from = calloc(1, (mesh.nverts + 1) * sizeof(int));
    int *faces_from = calloc(1, (mesh.nverts + 1) * sizeof(int));
    
    int nedges = 0;
    int nfaces = 0;
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int next = (vert + 1) % mesh.degree;
            
            int start = mesh.faces[face * mesh.degree + vert];
            int end = mesh.faces[face * mesh.degree + next];
            
            edges_from[start + 1]++;
            edges_from[end + 1]++;
            
            faces_from[start + 1]++;
            faces_from[end + 1]++;
            
            nedges += 2;
            nfaces += 2;
        }
    }
    
    for (int v = 1; v < mesh.nverts + 1; ++v) {
        edges_from[v] += edges_from[v - 1];
        faces_from[v] += faces_from[v - 1];
    }
    
    int *edges = malloc(nedges * sizeof(int));
    int *faces = malloc(nfaces * sizeof(int));
    
    int *edges_accum = calloc(1, mesh.nverts * sizeof(int));
    int *faces_accum = calloc(1, mesh.nverts * sizeof(int));
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int next = (vert + 1) % mesh.degree;
            
            int start = mesh.faces[face * mesh.degree + vert];
            int end = mesh.faces[face * mesh.degree + next];
            
            /* edge start */
            int edge_base = edges_from[start];
            int edge_count = edges_accum[start];
            
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
    
    struct ms_accel result = { 0 };
    
    result.faces_starts = faces_from;
    result.verts_starts = edges_from;
    result.faces_count = faces_accum;
    result.verts_count = edges_accum;
    result.faces_matrix = faces;
    result.verts_matrix = edges;
#endif
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}

// find all faces of vert
static struct ms_vec
vert_adjacent_faces(struct ms_accel *accel, struct ms_mesh mesh, int vertex)
{
    //TracyCZone(__FUNC__, true);
    (void) mesh;
    
    struct ms_vec result = { 0 };
    
    int from = accel->faces_starts[vertex];
#if OLD_INIT
    int to = accel->faces_starts[vertex + 1];
#else
    int to = accel->faces_starts[vertex] + accel->faces_count[vertex];
#endif
    
    result.len = to - from;
    result.cap = to - from;
    result.data = accel->faces_matrix + from;
    
    //TracyCZoneEnd(__FUNC__);
    
    return(result);
}


// find all verts of vert
static struct ms_vec
vert_adjacent_vertices(struct ms_accel *accel, struct ms_mesh mesh, int vertex)
{
    //TracyCZone(__FUNC__, true);
    
    (void) mesh;
    
    struct ms_vec result = { 0 };
    
    int from = accel->verts_starts[vertex];
    
#if OLD_INIT
    int to = accel->verts_starts[vertex + 1];
#else
    int to = accel->verts_starts[vertex] + accel->verts_count[vertex];
#endif
    
    result.len = to - from;
    result.cap = to - from;
    result.data = accel->verts_matrix + from;
    
    //TracyCZoneEnd(__FUNC__);
    
    return(result);
}

static int
edge_adjacent_face(struct ms_accel *accel, struct ms_mesh mesh, int me, int start, int end)
{
    //TracyCZone(__FUNC__, true);
    
    (void) mesh;
    
#if OLD_INIT
    int start_faces_from = accel->faces_starts[start];
    int start_faces_to = accel->faces_starts[start + 1];
    
    int end_faces_from = accel->faces_starts[end];
    int end_faces_to = accel->faces_starts[end + 1];
    
    int nfaces_start = start_faces_to - start_faces_from;
    int nfaces_end = end_faces_to - end_faces_from;
#else
    int start_faces_from = accel->faces_starts[start];
    int end_faces_from = accel->faces_starts[end];
    
    int nfaces_start = accel->faces_count[start];
    int nfaces_end = accel->faces_count[end];
    
    int start_faces_to = start_faces_from + nfaces_start;
    int end_faces_to = end_faces_from + nfaces_end;
#endif
    
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
                    //TracyCZoneEnd(__FUNC__);
                    return(face);
                }
            }
        }
    }
    
    //TracyCZoneEnd(__FUNC__);
    
    return(me);
}

// free
static void
free_results_vec(struct ms_vec *vec)
{
    (void) vec;
}

static void
free_hashtable(struct ms_accel *ht)
{
    (void) ht;
}