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
ms_hashtable_insert_(struct ms_hashsc *ht, int index, int from, int to, int face)
{
    struct ht_entry *bucket = ht->buckets[index];
    
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
            ht->buckets[index] = entry;
        }
    } else {
        struct ht_entry *entry = entry_from(from, to, face);
        ht->buckets[index] = entry;
    }
}

static struct ht_entry *
ms_hashtable_find_(struct ms_hashsc *ht, int index, int key)
{
    struct ht_entry *bucket = ht->buckets[index];
    
    for (struct ht_entry *entry = bucket; entry; entry = entry->next) {
        if (entry->key == key) {
            return(entry);
        }
    }
    
    return(NULL);
}

#if HASH_PLAIN
static void
ms_hashtable_insert(struct ms_hashsc *ht, int from, int to, int face)
{
    u32 hashcode = from % ht->width; // TODO
    ms_hashtable_insert_(ht, hashcode, from, to, face);
}

static struct ht_entry *
ms_hashtable_find(struct ms_hashsc *ht, int key)
{
    u32 hashcode = key % ht->width; // TODO
    struct ht_entry *result = ms_hashtable_find_(ht, hashcode, key);
    return(result);
}
#elif HASH_LOCAL
static const int LSH_SCALE = 100000;
static const int LSH_L = 100;
static const int LSH_P1 = 73856093;
static const int LSH_P2 = 19349663;
static const int LSH_P3 = 83492791;

static void
ms_hashtable_insert_lsh(struct ms_hashsc *ht, struct ms_v3 vertex, int from, int to, int face)
{
    int x = vertex.x * LSH_SCALE / LSH_L;
    int y = vertex.y * LSH_SCALE / LSH_L;
    int z = vertex.z * LSH_SCALE / LSH_L;
    u32 hashcode = (x * LSH_P1) ^ (y * LSH_P2) ^ (z * LSH_P3);
    hashcode %= ht->width;
    
    ms_hashtable_insert_(ht, hashcode, from, to, face);
}

static struct ht_entry *
ms_hashtable_find_lsh(struct ms_hashsc *ht, struct ms_v3 vertex, int key)
{
    int x = vertex.x * LSH_SCALE / LSH_L;
    int y = vertex.y * LSH_SCALE / LSH_L;
    int z = vertex.z * LSH_SCALE / LSH_L;
    u32 hashcode = (x * LSH_P1) ^ (y * LSH_P2) ^ (z * LSH_P3);
    hashcode %= ht->width;
    struct ht_entry *result = ms_hashtable_find_(ht, hashcode, key);
    return(result);
}
#endif