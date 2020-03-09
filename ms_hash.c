struct ht_entry {
    int key;
    struct ht_entry *next;
    struct ms_vec vertices;
    struct ms_vec faces;
};

struct ms_hashsc {
    struct ht_entry **buckets;
    int width;
};

static struct ms_hashsc
ms_hashtable_init(int size)
{
    struct ms_hashsc result = { 0 };
    
    result.buckets = calloc(1, size * sizeof(struct ht_entry *));
    result.width = size;
    
    return(result);
}

static struct ht_entry *
entry_from(int from, int to, int face)
{
    struct ht_entry *entry = malloc(sizeof(struct ht_entry));
    
    entry->next = NULL;
    entry->key = from;
    entry->vertices = ms_vec_init(4);
    entry->faces = ms_vec_init(4);
    
    ms_vec_push(&entry->vertices, to);
    ms_vec_push(&entry->faces, face);
    
    return(entry);
}

static void
ms_hashtable_insert(struct ms_hashsc *ht, int from, int to, int face)
{
    u32 hashcode = from; // TODO
    struct ht_entry *bucket = ht->buckets[hashcode % ht->width];
    
    if (bucket) {
        bool found_start = false;
        struct ht_entry *entry;
        for (entry = bucket; entry; entry = entry->next) {
            if (entry->key == from) {
                found_start = true;
                ms_vec_unique_push(&entry->vertices, to);
                ms_vec_unique_push(&entry->faces, face);
            }
        }
        
        if (!found_start) {
            struct ht_entry *entry = entry_from(from, to, face);
            entry->next = bucket;
            ht->buckets[hashcode % ht->width] = entry;
        }
    } else {
        struct ht_entry *entry = entry_from(from, to, face);
        ht->buckets[hashcode % ht->width] = entry;
    }
}

static struct ht_entry *
ms_hashtable_find(struct ms_hashsc *ht, int key)
{
    u32 hashcode = key; // TODO
    struct ht_entry *bucket = ht->buckets[hashcode % ht->width];
    
    for (struct ht_entry *entry = bucket; entry; entry = entry->next) {
        if (entry->key == key) {
            return(entry);
        }
    }
    
    return(NULL);
}