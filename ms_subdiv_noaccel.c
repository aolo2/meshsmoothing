static struct ms_vec
vert_adjacent_faces_noaccel(struct ms_mesh mesh, int vertex)
{
    TracyCZone(__FUNC__, true);
    
    struct ms_vec result = ms_vec_init(4);
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            if (vertex == mesh.faces[face * mesh.degree + vert]) {
                ms_vec_push(&result, face);
            }
        }
    }
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}

static struct ms_vec
vert_adjacent_edges_noaccel(struct ms_mesh mesh, int vertex)
{
    TracyCZone(__FUNC__, true);
    
    struct ms_vec result = ms_vec_init(4);
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int next = (vert + 1) % mesh.degree;
            
            int start = mesh.faces[face * mesh.degree + vert];
            int end = mesh.faces[face * mesh.degree + next];
            
            if (start > end) { SWAP(start, end); }
            
            if (vertex == start || vertex == end) {
                bool found = false;
                for (int i = 0; i < result.len / 2; ++i) {
                    int other_start = result.data[2 * i + 0];
                    int other_end = result.data[2 * i + 1];
                    
                    if (other_start == start && other_end == end) {
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    ms_vec_push(&result, start);
                    ms_vec_push(&result, end);
                }
            }
        }
    }
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}


static int
edge_adjacent_face_noaccel(struct ms_mesh mesh, int me, int start, int end)
{
    TracyCZone(__FUNC__, true);
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        if (face == me) {
            continue;
        }
        
        bool found_start = false;
        bool found_end = false;
        
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int vertex = mesh.faces[face * mesh.degree + vert];
            if (vertex == start) { found_start = true; }
            if (vertex == end)   { found_end = true; }
        }
        
        if (found_start && found_end) {
            TracyCZoneEnd(__FUNC__);
            return(face);
        }
    }
    
    TracyCZoneEnd(__FUNC__);
    
    return(me);
}