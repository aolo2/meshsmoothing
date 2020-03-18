struct ms_accel { void *dummy; };

static inline void
free_results_vec(struct ms_vec *vec)
{
    ms_vec_free(vec);
}

static void
free_hashtable(struct ms_accel *ht)
{
    (void) ht;
}

static struct ms_accel
init_hashtable(struct ms_mesh mesh)
{
    (void) mesh;
    
    struct ms_accel result = { 0 };
    
    return (result);
}

static struct ms_vec
vert_adjacent_faces(struct ms_accel *ht, struct ms_mesh mesh, int vertex)
{
    TracyCZone(__FUNC__, true);
    
    (void) ht;
    
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
vert_adjacent_vertices(struct ms_accel *ht, struct ms_mesh mesh, int vertex)
{
    TracyCZone(__FUNC__, true);
    
    (void) ht;
    
    struct ms_vec result = ms_vec_init(4);
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int next = (vert + 1) % mesh.degree;
            
            int start = mesh.faces[face * mesh.degree + vert];
            int end = mesh.faces[face * mesh.degree + next];
            int neighbour = -1;
            
            if (vertex == start) {
                neighbour = end;
            } else if (vertex == end) {
                neighbour = start;
            }
            
            if (neighbour != -1) {
                ms_vec_unique_push(&result, neighbour);
            }
        }
    }
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}


static int
edge_adjacent_face(struct ms_accel *ht, struct ms_mesh mesh, int me, int start, int end)
{
    TracyCZone(__FUNC__, true);
    
    (void) ht;
    
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