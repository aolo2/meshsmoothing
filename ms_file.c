/* Only triangle meshes! */
static struct ms_mesh
ms_file_stl_read_file(char *filename)
{
    FILE *file = fopen(filename, "rb");
    assert(file);
    
    struct ms_mesh mesh;
    mesh.degree = 3;
    
    /* Header. No data significance */
    fseek(file, 80, SEEK_SET);
    fread(&mesh.primitives, 4, 1, file);
    
    mesh.vertices = malloc(mesh.primitives * 3 * sizeof(struct ms_v3));
    mesh.normals = malloc(mesh.primitives * sizeof(struct ms_v3));
    
    assert(mesh.vertices);
    assert(mesh.normals);
    
    for (u32 i = 0; i < mesh.primitives; ++i) {
        fread(mesh.normals + i, sizeof(struct ms_v3), 1, file);
        fread(mesh.vertices + i * 3, 3 * sizeof(struct ms_v3), 1, file);
        fseek(file, 2, SEEK_CUR);
    }
    
    fclose(file);
    
    printf("[INFO] Loaded STL file: %s\n", filename);
    
    return(mesh);
}

static void
_get_vert_count_with_repeats(FILE *file, u32 *wo_repeats, u32 *with_repeats)
{
    s64 read;
    u64 len = 0;
    char *line = NULL;
    
    *wo_repeats = 0;
    *with_repeats = 0;
    
    while ((read = getline(&line, &len, file)) != -1) {
        if (read <= 1) {
            continue;
        }
        
        if (line[0] == 'v' && isspace(line[1])) {
            *wo_repeats += 1;
        }
        
        if (line[0] == 'f') {
            char *str = line + 1;
            while (*str != '\n' && *str != '\r') {
                while (*str == ' ' || *str == '\t') { ++str; }
                
                if (isdigit(*str)) {
                    *with_repeats += 1;
                }
                
                /* Skip all the other attributes */
                while (isdigit(*str) || *str == '/') { ++str; }
            }
        }
    }
}

/* All faces are expected to have the same number of vertices */
static struct ms_mesh
ms_file_obj_read_file(char *filename)
{
    FILE *file = fopen(filename, "rb");
    assert(file);
    
    char *line = NULL;
    u64 len = 0;
    s64 read;
    
    u32 vt_worpts;
    u32 vt_wrpts;
    _get_vert_count_with_repeats(file, &vt_worpts, &vt_wrpts);
    
    struct ms_v3 *vertices_without_repeats = malloc(vt_worpts * sizeof(struct ms_v3));
    struct ms_v3 *vertices_with_repeats = malloc(vt_wrpts * sizeof(struct ms_v3));
    
    assert(vertices_without_repeats);
    assert(vertices_with_repeats);
    
    fseek(file, 0, SEEK_SET);
    
    /* Read all the vertices (to accept forward declarations) */
    u32 verts_read = 0;
    while ((read = getline(&line, &len, file)) != -1) {
        if (read > 1 && line[0] == 'v') {
            if (!isspace(line[1])) { break; }
            
            char *str = line + 1;
            struct ms_v3 vertex;
            
            vertex.x = strtof(str, &str);
            vertex.y = strtof(str, &str);
            vertex.z = strtof(str, &str);
            
            vertices_without_repeats[verts_read] = vertex;
            ++verts_read;
        }
    }
    
    fseek(file, 0, SEEK_SET);
    
    /* 
        * Face definition
        * Read the first number in each slash-separated block
        */
    u32 verts_per_face = 0;
    verts_read = 0;
    
    while ((read = getline(&line, &len, file)) != -1) {
        if (read > 1 && line[0] == 'f') {
            if (!isspace(line[1])) { break; }
            
            char *str = line + 1;
            u32 face_vertices = 0;
            
            while (*str != '\n' && *str != '\r') {
                while (*str == ' ' || *str == '\t') { ++str; }
                
                s32 index = strtod(str, &str);
                
                /* Indices are 1-based, NOT zero based! */
                if (index < 0) {
                    index = vt_worpts + index;
                }
                
                vertices_with_repeats[verts_read] = vertices_without_repeats[index - 1];
                ++verts_read;
                ++face_vertices;
                
                /* Skip all the other attributes */
                while (isdigit(*str) || *str == '/') { ++str; }
            }
            
            if (verts_per_face == 0) {
                verts_per_face = face_vertices;
            } else {
                assert(face_vertices == verts_per_face);
            }
        }
    }
    
    
    struct ms_mesh mesh;
    
    assert(verts_read % verts_per_face == 0);
    
    mesh.degree = verts_per_face;
    mesh.primitives = verts_read / verts_per_face;
    mesh.vertices = vertices_with_repeats;
    mesh.normals = NULL;
    
    free(vertices_without_repeats);
    printf("[INFO] Loaded OBJ file: %s\n", filename);
    fclose(file);
    
    return(mesh);
}

/* Only quad meshes! */
static void
ms_file_stl_write_file(struct ms_mesh mesh, char *filename)
{
    FILE *file = fopen(filename, "wb");
    assert(file);
    
    fseek(file, 80, SEEK_SET);
    
    u32 primitives = mesh.primitives * 2;
    fwrite(&primitives, 4, 1, file);
    
    s16 attrib_count = 0;
    struct ms_v3 normal = { 0 };
    
    for (u32 i = 0; i < mesh.primitives; ++i) {
        /* ABCD quad -> two triangles ABC and CDA */
        
        /* ABC */
        fwrite(&normal, sizeof(struct ms_v3), 1, file);
        fwrite(mesh.vertices + i * 4, 3 * sizeof(struct ms_v3), 1, file);
        fwrite(&attrib_count, 2, 1, file);
        
        /* CDA */
        fwrite(&normal, sizeof(struct ms_v3), 1, file);
        fwrite(mesh.vertices + i * 4 + 2, 2 * sizeof(struct ms_v3), 1, file);
        fwrite(mesh.vertices + i * 4, sizeof(struct ms_v3), 1, file);
        fwrite(&attrib_count, 2, 1, file);
    }
    
    fclose(file);
    
    printf("[INFO] Saved %s\n", filename);
}

/* TODO: export to obj */