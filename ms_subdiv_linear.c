// init accel structure
static struct ms_accel
init_hashtable(struct ms_mesh mesh)
{
    TracyCZone(__FUNC__, true);
    
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
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}

static int
edge_adjacent_face(struct ms_accel *accel, int me, int start, int end)
{
    //TracyCZone(__FUNC__, true);
    
    int start_faces_from = accel->faces_starts[start];
    int end_faces_from = accel->faces_starts[end];
    
    int nfaces_start = accel->faces_count[start];
    int nfaces_end = accel->faces_count[end];
    
    int *faces = accel->faces_matrix;
    
    if (nfaces_start > 0 && nfaces_end > 0) {
        for (int f1 = start_faces_from; f1 < start_faces_from + nfaces_start; ++f1) {
            int face = faces[f1];
            if (face == me) {
                continue;
            }
            
            for (int f2 = end_faces_from; f2 < end_faces_from + nfaces_end; ++f2) {
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

static void
free_accel(struct ms_accel *accel)
{
    free(accel->faces_starts);
    free(accel->verts_starts);
    
    free(accel->faces_count);
    free(accel->verts_count);
    
    free(accel->faces_matrix);
    free(accel->verts_matrix);
}