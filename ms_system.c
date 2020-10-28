static u64
usec_now(void)
{
    u64 result;
    struct timespec ts;
    
    clock_gettime(CLOCK_MONOTONIC, &ts);
    result = ts.tv_sec * 1000000UL + ts.tv_nsec / 1000UL;
    
    return(result);
}

static u64
cycles_now(void)
{
    u32 hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return((u64) lo) | (((u64) hi) << 32);
}

static void *
cacheline_alloc(u64 size)
{
    if (size % CACHELINE) {
        size = ((size + CACHELINE - 1) / CACHELINE) * CACHELINE;
    }
    
    void *result = aligned_alloc(CACHELINE, size);
    return(result);
}

static struct ms_buffer
ms_file_read(char *filename)
{
    FILE *file;
    u64 size;
    void *memory;
    
    file = fopen(filename, "rb");
    assert(file);
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
    memory = malloc(size);
    assert(memory);
    fread(memory, size, 1, file);
    
    struct ms_buffer result;
    result.size = size;
    result.data = memory;
    
    fclose(file);
    
    return(result);
}

static f32
read_float(char *buffer, u32 at, u32 size, u64 *new_at)
{
    while (at < size && isspace(buffer[at])) { 
        ++at; 
    }
    
    f32 whole_part = 0.0f;
    f32 fractional_part = 0.0f;
    
    bool negative = false;
    
    // sign
    if (buffer[at] == '-') {
        negative = true;
        ++at;
    }
    
    // before dot
    while (at < size && isdigit(buffer[at])) {
        whole_part *= 10;
        whole_part += buffer[at] - '0';
        ++at;
    }
    
    // after dot
    if (buffer[at] == '.') {
        ++at;
        
        u32 fraction_degree = 1;
        while (at < size && isdigit(buffer[at])) {
            fractional_part *= 10;
            fractional_part += buffer[at] - '0';
            fraction_degree *= 10;
            ++at;
        }
        
        if (fraction_degree != 1) {
            fractional_part /= fraction_degree;
        } else {
            assert(!"Parsing error: floating point number has nothing after '.'");
        }
    }
    
    *new_at = at;
    
    f32 result = whole_part + fractional_part;
    if (negative) {
        result *= -1.0f;
    }
    
    return(result);
}

static s32
read_int(char *buffer, u32 at, u32 size, u64 *new_at)
{
    while (at < size && isspace(buffer[at])) { 
        ++at; 
    }
    
    bool negative = false;
    s32 result = 0;
    
    // sign
    if (buffer[at] == '-') {
        negative = true;
        ++at;
    }
    
    while (at < size && isdigit(buffer[at])) {
        result *= 10;
        result += buffer[at] - '0';
        ++at;
    }
    
    *new_at = at;
    
    if (negative) {
        result *= -1;
    }
    
    return(result);
}


static struct ms_mesh
ms_file_obj_read_fast(char *filename)
{
    TracyCZone(__FUNC__, true);
    
    printf("[INFO] Loading OBJ file: %s...\n", filename);
    
    struct ms_buffer file = ms_file_read(filename);
    
    u64 size = file.size;
    char *buffer = file.data;
    
    struct ms_v3 *verts = NULL;
    int *faces = NULL;
    int *faces_from = NULL;
    
    /* TODO(aolo2): prealloc 3 * linecount ? */
    
    for (u64 i = 0; i < size - 1; ++i) {
        if (buffer[i] == 'v' && isspace(buffer[i + 1])) {
            struct ms_v3 vertex = { 0 };
            ++i;
            
            vertex.x = read_float(buffer, i, size, &i);
            vertex.y = read_float(buffer, i, size, &i);
            vertex.z = read_float(buffer, i, size, &i);
            
            sb_push(verts, vertex);
        } else if (buffer[i] == 'f' && isspace(buffer[i + 1])) {
            ++i;
            
            while (buffer[i] != '\n') {
                int index = read_int(buffer, i, size, &i);
                
                sb_push(faces, index);
                
                /* Skip other attributes */
                while (!isspace(buffer[i])) {
                    ++i;
                }
            }
            
            sb_push(faces_from, sb_count(faces));
        }
    }
    
    struct ms_mesh mesh = { 0 };
    
    mesh.vertices = verts;
    mesh.faces = faces;
    mesh.faces_from = faces_from;
    mesh.nverts = sb_count(verts);
    mesh.nfaces = sb_count(faces_from) - 1;
    
    for (int i = 0; i < sb_count(faces); ++i) {
        /* Negative indices mean addressing from the end */
        if (mesh.faces[i] < 0) {
            mesh.faces[i] += mesh.nfaces;
        }
        
        /* Indices are 1-based, NOT zero based! */
        --mesh.faces[i];
    }
    
    free(buffer);
    
    TracyCZoneEnd(__FUNC__);
    
    printf("[INFO] Loading complete\n");
    
    return(mesh);
}

static void
ms_file_dump_quads(char *filename, struct ms_v3 *vertices)
{
    FILE *file = fopen(filename, "wb");
    assert(file);
    
    for (int v = 0; v < sb_count(vertices); ++v) {
        struct ms_v3 vertex = vertices[v];
        fprintf(file, "v %f %f %f\n", vertex.x, vertex.y, vertex.z);
    }
    
    for (int f = 0; f < sb_count(vertices); f += 4) {
        fprintf(file, "f %d %d %d %d\n", f + 1, f + 2, f + 3, f + 4);
    }
    
    fclose(file);
}

static void
ms_file_obj_write_file(char *filename, struct ms_mesh mesh)
{
    FILE *file = fopen(filename, "wb");
    assert(file);
    
    for (int v = 0; v < mesh.nverts; ++v) {
        struct ms_v3 vertex = mesh.vertices[v];
        fprintf(file, "v %f %f %f\n", vertex.x, vertex.y, vertex.z);
    }
    
    for (int f = 0; f < mesh.nfaces; ++f) {
        fprintf(file, "f %d %d %d %d\n",
                mesh.faces[f * 4 + 0] + 1, mesh.faces[f * 4 + 1] + 1,
                mesh.faces[f * 4 + 2] + 1, mesh.faces[f * 4 + 3] + 1);
    }
    
    fclose(file);
}