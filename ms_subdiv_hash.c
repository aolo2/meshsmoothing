static void
free_hashtable(struct ms_hashsc *ht)
{
    ms_hashtable_free(ht);
}

static struct ms_hashsc
init_hashtable(struct ms_mesh mesh)
{
    struct ms_hashsc hashtable = { 0 };
    TracyCZone(__FUNC__, true);
    
#if ADJACENCY_ACCEL == 1
    hashtable = ms_hashtable_init(mesh.nfaces);
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int next = (vert + 1) % mesh.degree;
            
            int start = mesh.faces[face * mesh.degree + vert];
            int end = mesh.faces[face * mesh.degree + next];
            
            ms_hashtable_insert(&hashtable, start, end, face);
            ms_hashtable_insert(&hashtable, end, start, face);
        }
    }
#elif ADJACENCY_ACCEL == 2
    hashtable = ms_hashtable_init(mesh.nfaces);
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int next = (vert + 1) % mesh.degree;
            
            int start = mesh.faces[face * mesh.degree + vert];
            int end = mesh.faces[face * mesh.degree + next];
            
            struct ms_v3 startv = mesh.vertices[start];
            struct ms_v3 endv = mesh.vertices[end];
            
            ms_hashtable_insert(&hashtable, startv, start, end, face);
            ms_hashtable_insert(&hashtable, endv, end, start, face);
        }
    }
#endif
    
    TracyCZoneEnd(__FUNC__);
    
    return (hashtable);
}

static inline struct ht_entry *
_find(struct ms_hashsc *ht, struct ms_mesh mesh, int vertex)
{
    struct ht_entry *entry;
    
#if ADJACENCY_ACCEL == 1
    (void) mesh;
    entry = ms_hashtable_find(ht, vertex);
#elif ADJACENCY_ACCEL == 2
    entry = ms_hashtable_find(ht, mesh.vertices[vertex], vertex);
#endif
    
    return(entry);
}

static struct ms_vec
vert_adjacent_faces(struct ms_hashsc *ht, struct ms_mesh mesh, int vertex)
{
    TracyCZone(__FUNC__, true);
    
    struct ht_entry *entry = _find(ht, mesh, vertex);
    struct ms_vec result = ms_vec_init(0);
    
    if (entry) {
        result = entry->faces;
    }
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}


static struct ms_vec
vert_adjacent_vertices(struct ms_hashsc *ht, struct ms_mesh mesh, int vertex)
{
    TracyCZone(__FUNC__, true);
    
    struct ht_entry *entry = _find(ht, mesh, vertex);
    struct ms_vec result = ms_vec_init(0);
    
    if (entry) {
        result = entry->vertices;
    }
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}


static int
edge_adjacent_face(struct ms_hashsc *ht, struct ms_mesh mesh, int me, int start, int end)
{
    TracyCZone(__FUNC__, true);
    
    struct ht_entry *entry_start = _find(ht, mesh, start);
    struct ht_entry *entry_end = _find(ht, mesh, end);
    
    if (entry_start && entry_end) {
        for (int f1 = 0; f1 < entry_start->faces.len; ++f1) {
            int face = entry_start->faces.data[f1];
            if (face == me) {
                continue;
            }
            
            for (int f2 = 0; f2 < entry_end->faces.len; ++f2) {
                int other_face = entry_end->faces.data[f2];
                if (other_face == face) {
                    TracyCZoneEnd(__FUNC__);
                    return(face);
                }
            }
        }
    }
    
    TracyCZoneEnd(__FUNC__);
    
    return(me);
}