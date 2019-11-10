static bool
_verts_equal(struct ms_v3 a, struct ms_v3 b)
{
    if (fabs(a.x - b.x) > ERR) {
        return(false);
    }
    
    if (fabs(a.y - b.y) > ERR) {
        return(false);
    }
    
    if (fabs(a.z - b.z) > ERR) {
        return(false);
    }
    
    return(true);
}

static u32
_edge_adjacent_triangle(struct ms_mesh mesh, u32 me, struct ms_v3 start, struct ms_v3 end)
{
    for (u32 face = 0; face < mesh.triangles; ++face) {
        if (face == me) {
            continue;
        }
        
        bool found_start = false;
        bool found_end = false;
        
        for (u32 vert = 0; vert < 3; ++vert) {
            struct ms_v3 vertex = mesh.vertices[face * 3 + vert];
            
            if (_verts_equal(vertex, start)) {
                found_start = true;
            }
            
            if (_verts_equal(vertex, end)) {
                found_end = true;
            }
        }
        
        if (found_start && found_end) {
            return(face);
        }
    }
    
    /* Should not happen */
    assert(false);
}

static u32
_vert_adjacent_triangles(struct ms_mesh mesh, struct ms_v3 point, u32 *dest)
{
    u32 tr_count = 0;
    
    for (u32 face = 0; face < mesh.triangles; ++face) {
        for (u32 vert = 0; vert < 3; ++vert) {
            struct ms_v3 vertex = mesh.vertices[face * 3 + vert];
            
            if (_verts_equal(point, vertex)) {
                if (dest != NULL) {
                    dest[tr_count] = face;
                }
                ++tr_count;
            }
        }
    }
    
    return(tr_count);
}

static u32
_vert_adjacent_edges(struct ms_mesh mesh, struct ms_v3 point, u32 *dest)
{
    u32 edge_count = 0;
    
    for (u32 face = 0; face < mesh.triangles; ++face) {
        for (u32 vert = 0; vert < 3; ++vert) {
            u32 next_vert = (vert + 1) % 3;
            struct ms_v3 start = mesh.vertices[face * 3 + vert];
            struct ms_v3 end = mesh.vertices[face * 3 + next_vert];
            
            if (_verts_equal(point, start) || _verts_equal(point, end)) {
                if (dest != NULL) {
                    dest[edge_count] = face * 3 + vert;
                }
                ++edge_count;
            }
        }
    }
    
    return(edge_count);
}

static struct ms_mesh
ms_subdiv_catmull_clark(struct ms_mesh mesh)
{
    /* TODO: mesh.vertices should be ms_v3 * */
    struct ms_v3 *face_points = malloc(mesh.triangles * sizeof(struct ms_v3));
    
    /* Face points */
    for (u32 face = 0; face < mesh.triangles; ++face) {
        struct ms_v3 fp = { 0 };
        
        for (u32 vert = 0; vert < 3; ++vert) {
            fp.x += mesh.vertices[face * 3 + vert].x;
            fp.y += mesh.vertices[face * 3 + vert].y;
            fp.z += mesh.vertices[face * 3 + vert].z;
        }
        
        fp.x /= 3.0f;
        fp.y /= 3.0f;
        fp.z /= 3.0f;
        
        face_points[face] = fp;
    }
    
    /* Edge points */
    struct ms_v3 *edge_points = malloc(mesh.triangles * 3 * 2 * sizeof(struct ms_v3));
    for (u32 face = 0; face < mesh.triangles; ++face) {
        for (u32 vert = 0; vert < 3; ++vert) {
            u32 next_vert = (vert + 1) % 3;
            struct ms_v3 start = mesh.vertices[face * 3 + vert];
            struct ms_v3 end = mesh.vertices[face * 3 + next_vert];
            
            u32 adj = _edge_adjacent_triangle(mesh, face, start, end);
            
            struct ms_v3 adj_center = ms_math_avg(face_points[face], face_points[adj]);
            struct ms_v3 edge_center = ms_math_avg(start, end);
            struct ms_v3 face_center = ms_math_avg(adj_center, face_center);
            
            edge_points[face * 3 + vert] = ms_math_avg(edge_center, face_center);
        }
    }
    
    
    /* Subdivide */
    for (u32 face = 0; face < mesh.triangles; ++face) {
        for (u32 vert = 0; vert < 3; ++vert) {
            /* 
    * New points need:
    *     - Old coordinates
            *     - Number of faces this vertex belongs to
            *     - Edge points of edges this vertex belongs to
            *     - Face points of faces this vertex belongs to
    */
            struct ms_v3 old = mesh.vertices[face * 3 + vert];
            
            u32 nadj_faces = _vert_adjacent_triangles(mesh, old, NULL);
            u32 *adj_faces = malloc(nadj_faces * sizeof(u32));
            _vert_adjacent_triangles(mesh, old, adj_faces);
            
            u32 nadj_edges = _vert_adjacent_edges(mesh, old, NULL);
            u32 *adj_edges = malloc(nadj_edges * sizeof(u32));
            _vert_adjacent_edges(mesh, old, adj_edges);
        }
    }
    
    free(face_points);
    
    printf("[INFO] Finished Catmull-Clark step\n");
    
    return(mesh);
}