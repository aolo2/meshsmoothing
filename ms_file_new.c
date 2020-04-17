static struct ms_mesh
ms_file_obj_read_file_new(char *filename)
{
    FILE *file = fopen(filename, "rb");
    assert(file);
    
    printf("[INFO] Loading OBJ file: %s...\n", filename);
    
    int nverts = 0;
    int nfaces = 0;
    char *line = NULL;
    int expected_face_vertices = 4; /* TODO: csr for irregular meshes? */
    int read = 0;
    size_t len = 0;
    
    /* Count faces and vertices */
    while ((read = getline(&line, &len, file)) != -1) {
        if (read <= 1) {
            continue;
        }
        
        if (line[0] == 'v' && isspace(line[1])) {
            ++nverts;
        }
        
        if (line[0] == 'f' && isspace(line[1])) {
            ++nfaces;
        }
    }
    
    struct ms_v3 *vertices = malloc(nverts * sizeof(struct ms_v3));
    int *faces = malloc(nfaces * expected_face_vertices * sizeof(int));
    
    TracyCAlloc(vertices, nverts * sizeof(struct ms_v3));
    TracyCAlloc(faces, nfaces * expected_face_vertices * sizeof(int));
    
    fseek(file, 0, SEEK_SET);
    
    /* Read vertices into an array. Faces only use indices into this array */
    int verts_read = 0;
    while ((read = getline(&line, &len, file)) != -1) {
        if (read > 1 && line[0] == 'v') {
            if (!isspace(line[1])) { break; }
            
            char *str = line + 1;
            struct ms_v3 vertex;
            
            vertex.x = strtof(str, &str);
            vertex.y = strtof(str, &str);
            vertex.z = strtof(str, &str);
            
            vertices[verts_read] = vertex;
            ++verts_read;
            
            if (verts_read == nverts) {
                break;
            }
        }
    }
    
    /*
    * Face definition
    * Read the first number in each slash-separated block
    */
    int indices_read = 0;
    while ((read = getline(&line, &len, file)) != -1) {
        if (read > 1 && line[0] == 'f') {
            if (!isspace(line[1])) { break; }
            
            char *str = line + 1;
            int face_vertices = 0;
            
            while (*str != '\n' && *str != '\r') {
                while (*str == ' ' || *str == '\t') { ++str; }
                
                int index = strtod(str, &str);
                
                /* Negative indices mean addressing from the end */
                if (index < 0) {
                    index = nverts + index;
                }
                
                /* Indices are 1-based, NOT zero based! */
                faces[indices_read] = index - 1;
                ++face_vertices;
                ++indices_read;
                
                /* Skip all the other attributes */
                while (isdigit(*str) || *str == '/') { ++str; }
            }
            
            assert(face_vertices == expected_face_vertices);
        }
    }
    
    free(line);
    
    fclose(file);
    
    struct ms_mesh mesh = { 0 };
    
    mesh.vertices = vertices;
    mesh.faces = faces;
    mesh.nfaces = nfaces;
    mesh.nverts = nverts;
    mesh.degree = 4;
    
    printf("[INFO] Loaded OBJ file: %s.\n", filename);
    
    return(mesh);
}

static void
ms_file_obj_write_file_new(char *filename, struct ms_mesh mesh)
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