static struct ms_vec
vert_adjacent_faces_hash(struct ms_hashsc *ht, int vertex)
{
    TracyCZone(__FUNC__, true);
    
    struct ht_entry *entry = ms_hashtable_find(ht, vertex);
    struct ms_vec result = ms_vec_init(0);
    
    if (entry) {
        result = entry->faces;
    }
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}


static struct ms_vec
vert_adjacent_edges_hash(struct ms_hashsc *ht, int vertex)
{
    TracyCZone(__FUNC__, true);
    
    struct ht_entry *entry = ms_hashtable_find(ht, vertex);
    struct ms_vec result = ms_vec_init(4);
    
    if (entry) {
        for (int v = 0; v < entry->vertices.len; ++v) {
            int start = vertex;
            int end = entry->vertices.data[v];
            
            if (start > end) { SWAP(start, end); }
            
            ms_vec_push(&result, start);
            ms_vec_push(&result, end);
        }
    }
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}


static int
edge_adjacent_face_hash(struct ms_hashsc *ht, int me, int start, int end)
{
    TracyCZone(__FUNC__, true);
    
    struct ht_entry *entry_start = ms_hashtable_find(ht, start);
    struct ht_entry *entry_end = ms_hashtable_find(ht, end);
    
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