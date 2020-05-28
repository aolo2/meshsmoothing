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
    
    int nverts = 0;
    int nfaces = 0;
    
    for (u64 i = 0; i < size - 2; ++i) {
        if (i == 0 && buffer[i + 1] == ' ') {
            if (buffer[i] == 'v') {
                ++nverts;
            } else if (buffer[i] == 'f') {
                ++nfaces;
            }
        }
        
        if (buffer[i] == '\n' && buffer[i + 2] == ' ') {
            if (buffer[i + 1] == 'v') {
                ++nverts;
            } else if (buffer[i + 1] == 'f') {
                ++nfaces;
            }
        }
    }
    
    struct ms_v3 *verts = malloc(nverts * sizeof(struct ms_v3));
    int *faces = malloc(nfaces * 4 * sizeof(int));
    
    assert(verts);
    assert(faces);
    
    u32 verts_read = 0;
    u32 faces_read = 0;
    
    bool nl = true;
    for (u64 i = 0; i < size - 1; ++i) {
        if (nl) {
            if (buffer[i] == 'v' && isspace(buffer[i + 1])) {
                struct ms_v3 vertex = { 0 };
                
                ++i;
                
                vertex.x = read_float(buffer, i, size, &i);
                vertex.y = read_float(buffer, i, size, &i);
                vertex.z = read_float(buffer, i, size, &i);
                
                verts[verts_read++] = vertex;
            } else if (buffer[i] == 'f' && isspace(buffer[i + 1])) {
                
                ++i;
                
                int a = read_int(buffer, i, size, &i);
                while (buffer[i] == '/' || isdigit(buffer[i])) { ++i; }
                
                int b = read_int(buffer, i, size, &i);
                while (buffer[i] == '/' || isdigit(buffer[i])) { ++i; }
                
                int c = read_int(buffer, i, size, &i);
                while (buffer[i] == '/' || isdigit(buffer[i])) { ++i; }
                
                int d = read_int(buffer, i, size, &i);
                
                /* Negative indices mean addressing from the end */
                if (a < 0) { a = nverts + a; }
                if (b < 0) { b = nverts + b; }
                if (c < 0) { c = nverts + c; }
                if (d < 0) { d = nverts + d; }
                
                /* Indices are 1-based, NOT zero based! */
                faces[faces_read++] = a - 1;
                faces[faces_read++] = b - 1;
                faces[faces_read++] = c - 1;
                faces[faces_read++] = d - 1;
            }
            
            nl = false;
        }
        
        if (buffer[i] == '\n') {
            nl = true;
        }
    }
    
    free(buffer);
    
    struct ms_mesh mesh = { 0 };
    
    mesh.degree = 4;
    mesh.nverts = nverts;
    mesh.nfaces = nfaces;
    mesh.vertices = verts;
    mesh.faces = faces;
    
    TracyCZoneEnd(__FUNC__);
    
    return(mesh);
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