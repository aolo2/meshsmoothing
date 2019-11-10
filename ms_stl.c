static struct ms_mesh
ms_stl_read_file(char *filename)
{
    FILE *file = fopen(filename, "rb");
    struct ms_mesh mesh;
    
    assert(file);
    
    /* Header. No data significance */
    fseek(file, 80, SEEK_SET);
    fread(&mesh.triangles, 4, 1, file);
    
    /* Only triangle meshes! */
    mesh.vertices = malloc(mesh.triangles * 9 * sizeof(f32));
    mesh.normals = malloc(mesh.triangles * 3 * sizeof(f32));
    
    assert(mesh.vertices);
    assert(mesh.normals);
    
    for (u32 i = 0; i < mesh.triangles; ++i) {
        fread(mesh.normals + i * 3, 3 * sizeof(f32), 1, file);
        fread(mesh.vertices + i * 9, 9 * sizeof(f32), 1, file);
        fseek(file, 2, SEEK_CUR);
    }
    
    fclose(file);
    
    printf("[INFO] Loaded %s\n", filename);
    
    return(mesh);
}