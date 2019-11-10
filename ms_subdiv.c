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
    for (u32 face = 0; face < mesh.primitives; ++face) {
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
    
    for (u32 face = 0; face < mesh.primitives; ++face) {
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
    
    for (u32 face = 0; face < mesh.primitives; ++face) {
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
    /* Face points */
    struct ms_v3 *face_points = malloc(mesh.primitives * sizeof(struct ms_v3));
    
    for (u32 face = 0; face < mesh.primitives; ++face) {
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
    /* TODO: quads! */
    struct ms_v3 *edge_points = malloc(mesh.primitives * 3 * sizeof(struct ms_v3));
    
    for (u32 face = 0; face < mesh.primitives; ++face) {
        for (u32 vert = 0; vert < 3; ++vert) {
            u32 next_vert = (vert + 1) % 3;
            struct ms_v3 start = mesh.vertices[face * 3 + vert];
            struct ms_v3 end = mesh.vertices[face * 3 + next_vert];
            
            u32 adj = _edge_adjacent_triangle(mesh, face, start, end);
            
            struct ms_v3 adj_center = ms_math_avg(face_points[face], face_points[adj]);
            struct ms_v3 edge_center = ms_math_avg(start, end);
            struct ms_v3 face_center = ms_math_avg(adj_center, edge_center);
            
            edge_points[face * 3 + vert] = ms_math_avg(edge_center, face_center);
        }
    }
    
    /* Update points */
    struct ms_v3 *new_verts = malloc(mesh.primitives * 3 * sizeof(struct ms_v3));
    
    for (u32 face = 0; face < mesh.primitives; ++face) {
        for (u32 vert = 0; vert < 3; ++vert) {
            struct ms_v3 old_vert = mesh.vertices[face * 3 + vert];
            
            /* Average of face points of all the faces this vertex is adjacent to */
            u32 nadj_faces = _vert_adjacent_triangles(mesh, old_vert, NULL);
            u32 *adj_faces = malloc(nadj_faces * sizeof(u32));
            _vert_adjacent_triangles(mesh, old_vert, adj_faces);
            
            struct ms_v3 avg_face_point = { 0 };
            for (u32 i = 0; i < nadj_faces; ++i) {
                avg_face_point.x += face_points[adj_faces[i]].x;
                avg_face_point.y += face_points[adj_faces[i]].y;
                avg_face_point.z += face_points[adj_faces[i]].z;
            }
            avg_face_point.x /= (f32) nadj_faces;
            avg_face_point.y /= (f32) nadj_faces;
            avg_face_point.z /= (f32) nadj_faces;
            
            /* Average of edge points of all the edges this vertex is adjacent to */
            u32 nadj_edges = _vert_adjacent_edges(mesh, old_vert, NULL);
            u32 *adj_edges = malloc(nadj_edges * sizeof(u32));
            _vert_adjacent_edges(mesh, old_vert, adj_edges);
            
            struct ms_v3 avg_edge_point = { 0 };
            for (u32 i = 0; i < nadj_edges; ++i) {
                avg_edge_point.x += edge_points[adj_edges[i]].x;
                avg_edge_point.y += edge_points[adj_edges[i]].y;
                avg_edge_point.z += edge_points[adj_edges[i]].z;
            }
            avg_edge_point.x /= (f32) nadj_edges;
            avg_edge_point.y /= (f32) nadj_edges;
            avg_edge_point.z /= (f32) nadj_edges;
            
            /* Weights */
            f32 w1 = (f32) (nadj_faces - 3) / (f32) nadj_faces;
            f32 w2 = 1.0f / (f32) nadj_faces;
            f32 w3 = 2.0f * w2;
            
            /* Weighted average to obtain a new vertex */
            struct ms_v3 new_vert;
            new_vert.x = w1 * old_vert.x + w2 * avg_face_point.x + w3 * avg_edge_point.x;
            new_vert.y = w1 * old_vert.y + w2 * avg_face_point.y + w3 * avg_edge_point.y;
            new_vert.z = w1 * old_vert.z + w2 * avg_face_point.z + w3 * avg_edge_point.z;
            
            new_verts[face * 3 + vert] = new_vert;
        }
    }
    
    /* Subdivide */
    struct ms_mesh new_mesh;
    new_mesh.primitives = mesh.primitives * 3;
    new_mesh.vertices = malloc(mesh.primitives * 3 * 4 * sizeof(struct ms_v3));
    
    for (u32 face = 0; face < mesh.primitives; ++face) {
        struct ms_v3 edge_point_ab = edge_points[face * 3 + 0];
        struct ms_v3 edge_point_bc = edge_points[face * 3 + 1];
        struct ms_v3 edge_point_ca = edge_points[face * 3 + 2];
        struct ms_v3 face_point_abc = face_points[face];
        
        struct ms_v3 a = new_verts[face * 3 + 0];
        struct ms_v3 b = new_verts[face * 3 + 1];
        struct ms_v3 c = new_verts[face * 3 + 2];
        
        /* face 0 */
        new_mesh.vertices[face * 12 + 0] = a;
        new_mesh.vertices[face * 12 + 1] = edge_point_ab;
        new_mesh.vertices[face * 12 + 2] = face_point_abc;
        new_mesh.vertices[face * 12 + 3] = edge_point_ca;
        
        /* face 1 */
        new_mesh.vertices[face * 12 + 4] = b;
        new_mesh.vertices[face * 12 + 5] = edge_point_bc;
        new_mesh.vertices[face * 12 + 6] = face_point_abc;
        new_mesh.vertices[face * 12 + 7] = edge_point_ab;
        
        /* face 2 */
        new_mesh.vertices[face * 12 + 8] = c;
        new_mesh.vertices[face * 12 + 9] = edge_point_ca;
        new_mesh.vertices[face * 12 + 10] = face_point_abc;
        new_mesh.vertices[face * 12 + 11] = edge_point_bc;
    }
    
    free(new_verts);
    free(face_points);
    free(edge_points);
    
    new_mesh.vert_per_face = 4;
    
    printf("[INFO] Finished Catmull-Clark step\n");
    
    return(new_mesh);
}