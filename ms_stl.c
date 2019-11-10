/* Only triangle meshes! */
static struct ms_mesh
ms_stl_read_file(char *filename)
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
    
    printf("[INFO] Loaded %s\n", filename);
    
    return(mesh);
}

/* Only quad meshes! */
static void
ms_stl_write_file(struct ms_mesh mesh, char *filename)
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