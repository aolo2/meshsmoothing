static struct ms_mesh
ms_stl_read_file(char *filename)
{
    FILE *file = fopen(filename, "rb");
    
    struct ms_mesh mesh;
    mesh.vert_per_face = 3;
    
    assert(file);
    
    /* Header. No data significance */
    fseek(file, 80, SEEK_SET);
    fread(&mesh.primitives, 4, 1, file);
    
    /* Only triangle meshes! */
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