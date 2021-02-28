/* 
* For now:
*     - quad faces only
*     - assume the mesh (x4) fits within memory of one node
*     - assume more or less even distrubution of vertices in space (no super-dense regions)
*     - no vertices lie exactly on the cut
*/
static void
coordinate_cut(int size, struct ms_mesh *mesh, int *map, int *pp_faces, int *pp_nfaces, int *pp_displs,
               f32 *pp_verts_x, f32 *pp_verts_y, f32 *pp_verts_z, int *pp_nverts, int *pp_vdispls, int *pp_map)
{
    f32 min_x = mesh->vertices_x[0];
    f32 max_x = min_x;
    
    for (int v = 0; v < mesh->nverts; ++v) {
        f32 x = mesh->vertices_x[v];
        if (x < min_x) { min_x = x; }
        if (x > max_x) { max_x = x; }
    }
    
    f32 range = max_x - min_x;
    f32 pp_range = range / size;
    
    for (int v = 0; v < mesh->nverts; ++v) {
        f32 x = mesh->vertices_x[v];
        int proc = (x - min_x) / pp_range; /* note that casting to int rounds towards zero, saving us from min_x being assigned to node -1 */
        map[v] = proc;
    }
    
    for (int face = 0; face < mesh->nfaces; ++face) {
        int v1 = mesh->faces[face * 4 + 0];
        int v2 = mesh->faces[face * 4 + 1];
        int v3 = mesh->faces[face * 4 + 2];
        int v4 = mesh->faces[face * 4 + 3];
        
        int p1 = map[v1];
        int p2 = map[v2];
        int p3 = map[v3];
        int p4 = map[v4];
        
        /* If at least one vertex of face belongs to process, then all
vertices of face go to that process */
        for (int p = 0; p < size; ++p) {
            int pp_offset = p * mesh->nverts * 4;
            if (p == p1 || p == p2 || p == p3 || p == p4) {
                int nfaces = pp_nfaces[p];
                
                pp_faces[pp_offset + nfaces + 0] = v1;
                pp_faces[pp_offset + nfaces + 1] = v2;
                pp_faces[pp_offset + nfaces + 2] = v3;
                pp_faces[pp_offset + nfaces + 3] = v4;
                
                pp_nfaces[p] += 4;
            }
        }
    }
    
    for (int p = 0; p < size; ++p) {
        pp_displs[p] = p * mesh->nverts * 4;
    }
    
    /* 2-in-1 special! Get actual vertex coordinates, and also re-number the vertices */
    for (int p = 0; p < size; ++p) {
        int pp_offset = p * mesh->nverts * 4;
        int pp_voffset = p * mesh->nverts;
        int nfaces = pp_nfaces[p];
        
        for (int i = 0; i < nfaces; ++i) {
            int vertex = pp_faces[pp_offset + i];
            /* We haven't got this vertex' coordinates yet */
            if (pp_map[pp_voffset + vertex] == -1) {
                int nverts = pp_nverts[p];
                
                f32 x = mesh->vertices_x[vertex];
                f32 y = mesh->vertices_y[vertex];
                f32 z = mesh->vertices_z[vertex];
                
                pp_map[pp_voffset + vertex] = nverts;
                
                pp_verts_x[pp_voffset + nverts] = x;
                pp_verts_y[pp_voffset + nverts] = y;
                pp_verts_z[pp_voffset + nverts] = z;
                
                ++pp_nverts[p];
            }
        }
    }
    
    for (int p = 0; p < size; ++p) {
        pp_vdispls[p] = p * mesh->nverts;
    }
}

static void
distribute_mesh_with_overlap(MPI_Comm comm, int rank, int size, struct ms_mesh *mesh)
{
    /* 
* Basic outline of the algorithm:
    * 1. Partition _vertices_ based on their coordinate(s)
* 2. Each node gets all _faces_ containing these vertices (this creates 1-wide overlap)
* 3. Faces that end up on the edge of the cut are not subdivided (they are only needed to
  *    properly subdivide inner faces)
*/
    int *map = NULL;
    
    int *pp_faces = NULL;
    int *pp_nfaces = NULL;
    int *pp_displs = NULL;
    
    int *pp_nverts = NULL;
    f32 *pp_verts_x = NULL;
    f32 *pp_verts_y = NULL;
    f32 *pp_verts_z = NULL;
    int *pp_vdispls = NULL;
    int *pp_map = NULL;
    
    if (rank == MASTER) {
        /* Worst case allocation: each vertex is in a separate face. All faces in one array for
 simplicity of MPI_Send and malloc/free */
        pp_faces = malloc(mesh->nverts * 4 * size * sizeof(int));
        pp_nfaces = malloc(size * sizeof(int));
        pp_displs = malloc(size * sizeof(int));
        
        pp_verts_x = malloc(mesh->nverts * size * sizeof(f32));
        pp_verts_y = malloc(mesh->nverts * size * sizeof(f32));
        pp_verts_z = malloc(mesh->nverts * size * sizeof(f32));
        pp_nverts = malloc(size * sizeof(int));
        pp_vdispls = malloc(size * sizeof(int));
        pp_map = malloc(mesh->nverts * size * sizeof(int));
        
        map = malloc(mesh->nverts * sizeof(int));
        memset(pp_nfaces, 0, size * sizeof(int));
        memset(pp_nverts, 0, size * sizeof(int));
        memset(pp_map, -1, mesh->nverts * size * sizeof(int));
        
        coordinate_cut(size, mesh, map, pp_faces, pp_nfaces, pp_displs,
                       pp_verts_x, pp_verts_y, pp_verts_z, pp_nverts, pp_vdispls, pp_map);
    }
    
    /* Scatter vertices */
    int vertices_count = 0;
    f32 *vertices_x  = NULL;
    f32 *vertices_y  = NULL;
    f32 *vertices_z  = NULL;
    
    MPI_Scatter(pp_nverts, 1, MPI_INT, &vertices_count, 1, MPI_INT, MASTER, comm);
    vertices_x = malloc(vertices_count * sizeof(f32));
    vertices_y = malloc(vertices_count * sizeof(f32));
    vertices_z = malloc(vertices_count * sizeof(f32));
    MPI_Scatterv(pp_verts_x, pp_nverts, pp_vdispls, MPI_FLOAT, vertices_x, vertices_count, MPI_FLOAT, MASTER, comm);
    MPI_Scatterv(pp_verts_y, pp_nverts, pp_vdispls, MPI_FLOAT, vertices_y, vertices_count, MPI_FLOAT, MASTER, comm);
    MPI_Scatterv(pp_verts_z, pp_nverts, pp_vdispls, MPI_FLOAT, vertices_z, vertices_count, MPI_FLOAT, MASTER, comm);
    
    /* Scatter faces */
    int faces_count = 0;
    int *faces  = NULL;
    
    MPI_Scatter(pp_nfaces, 1, MPI_INT, &faces_count, 1, MPI_INT, MASTER, comm);
    faces = malloc(faces_count * sizeof(int));
    MPI_Scatterv(pp_faces, pp_nfaces, pp_displs, MPI_INT, faces, faces_count, MPI_INT, MASTER, comm);
    
    mesh->nfaces = faces_count / 4;
    mesh->faces = faces;
    mesh->nverts = vertices_count;
    mesh->vertices_x = vertices_x;
    mesh->vertices_y = vertices_y;
    mesh->vertices_z = vertices_z;
    
    if (rank == MASTER) {
        free(pp_nfaces);
        free(pp_faces);
        free(pp_displs);
        free(pp_nverts);
        free(pp_verts_x);
        free(pp_verts_y);
        free(pp_verts_z);
        free(pp_vdispls);
        free(pp_map);
        free(map);
    }
}

static void
stitch_back_mesh(MPI_Comm comm, int rank, int size, struct ms_mesh *mesh)
{
    /* TODO */
}