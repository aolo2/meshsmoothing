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
_edge_adjacent_face(struct ms_mesh mesh, u32 me, struct ms_v3 start, struct ms_v3 end)
{
    for (u32 face = 0; face < mesh.primitives; ++face) {
        if (face == me) {
            continue;
        }
        
        bool found_start = false;
        bool found_end = false;
        
        for (u32 vert = 0; vert < mesh.degree; ++vert) {
            struct ms_v3 vertex = mesh.vertices[face * mesh.degree + vert];
            
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
    
    return(me);
}

static u32
_vert_adjacent_faces_repeats(struct ms_mesh mesh, struct ms_v3 point, u32 *dest)
{
    u32 tr_count = 0;
    
    for (u32 face = 0; face < mesh.primitives; ++face) {
        for (u32 vert = 0; vert < mesh.degree; ++vert) {
            struct ms_v3 vertex = mesh.vertices[face * mesh.degree + vert];
            
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
_vert_adjacent_edges_repeats(struct ms_mesh mesh, struct ms_v3 point, struct ms_v3 *dest)
{
    u32 edge_count = 0;
    
    for (u32 face = 0; face < mesh.primitives; ++face) {
        for (u32 vert = 0; vert < mesh.degree; ++vert) {
            u32 next_vert = (vert + 1) % mesh.degree;
            struct ms_v3 start = mesh.vertices[face * mesh.degree + vert];
            struct ms_v3 end = mesh.vertices[face * mesh.degree + next_vert];
            
            if (_verts_equal(point, start) || _verts_equal(point, end)) {
                if (dest != NULL) {
                    dest[2 * edge_count + 0] = start;
                    dest[2 * edge_count + 1] = end;
                }
                ++edge_count;
            }
        }
    }
    
    return(edge_count);
}

static u32
_find_edge(struct ms_v3 *edges, u32 me)
{
    struct ms_v3 me_start = edges[me * 2 + 0];
    struct ms_v3 me_end = edges[me * 2 + 1];
    
    for (u32 edge = 0; edge < me; ++edge) {
        struct ms_v3 start = edges[edge * 2 + 0];
        struct ms_v3 end = edges[edge * 2 + 1];
        
        if (_verts_equal(me_start, start) && _verts_equal(me_end, end)) {
            if (edge != me) {
                return(edge);
            }
        }
        
        if (_verts_equal(me_start, end) && _verts_equal(me_end, start)) {
            if (edge != me) {
                return(edge);
            }
        }
    }
    
    return(me);
}

static u32
_vert_adjacent_edges(struct ms_mesh mesh, struct ms_v3 point, struct ms_v3 **dest)
{
    /* 
* Edges can be repeated between faces, but not all of them do (e.g. 
 * edges near a hole only occur once). Here we detect all the duplicates
 * and correct the return values
*/
    u32 nedges = _vert_adjacent_edges_repeats(mesh, point, NULL);
    u32 nedges_no_repeats = 0;
    struct ms_v3 *edges = malloc(nedges * 2 * sizeof(struct ms_v3));
    struct ms_v3 *edges_no_repeats = malloc(nedges * 2 * sizeof(struct ms_v3));
    
    _vert_adjacent_edges_repeats(mesh, point, edges);
    
    for (u32 edge = 0; edge < nedges; ++edge) {
        struct ms_v3 start = edges[edge * 2 + 0];
        struct ms_v3 end = edges[edge * 2 + 1];
        
        u32 index = _find_edge(edges, edge);
        if (index == edge) {
            edges_no_repeats[nedges_no_repeats * 2 + 0] = start;
            edges_no_repeats[nedges_no_repeats * 2 + 1] = end;
            ++nedges_no_repeats;
        }
    }
    
    free(edges);
    *dest = edges_no_repeats;
    
    return(nedges_no_repeats);
}

static u32
_vert_adjacent_faces(struct ms_mesh mesh, struct ms_v3 point, u32 **dest)
{
    /* There are no repeated faces, but just for consistency of the API */
    u32 nfaces = _vert_adjacent_faces_repeats(mesh, point, NULL);
    u32 *faces = malloc(nfaces * sizeof(u32));
    
    _vert_adjacent_faces_repeats(mesh, point, faces);
    *dest = faces;
    
    return(nfaces);
}

static struct ms_mesh
ms_subdiv_catmull_clark(struct ms_mesh mesh)
{
    /* Face points */
    struct ms_v3 *face_points = malloc(mesh.primitives * sizeof(struct ms_v3));
    
    for (u32 face = 0; face < mesh.primitives; ++face) {
        struct ms_v3 fp = { 0 };
        
        for (u32 vert = 0; vert < mesh.degree; ++vert) {
            fp.x += mesh.vertices[face * mesh.degree + vert].x;
            fp.y += mesh.vertices[face * mesh.degree + vert].y;
            fp.z += mesh.vertices[face * mesh.degree + vert].z;
        }
        
        fp.x /= (f32) mesh.degree;
        fp.y /= (f32) mesh.degree;
        fp.z /= (f32) mesh.degree;
        
        face_points[face] = fp;
    }
    
    /* Edge points */
    struct ms_v3 *edge_points = malloc(mesh.primitives * mesh.degree * sizeof(struct ms_v3));
    
    for (u32 face = 0; face < mesh.primitives; ++face) {
        for (u32 vert = 0; vert < mesh.degree; ++vert) {
            u32 next_vert = (vert + 1) % mesh.degree;
            struct ms_v3 start = mesh.vertices[face * mesh.degree + vert];
            struct ms_v3 end = mesh.vertices[face * mesh.degree + next_vert];
            
            u32 adj = _edge_adjacent_face(mesh, face, start, end);
            if (adj != face) {
                struct ms_v3 face_avg = ms_math_avg(face_points[face], face_points[adj]);
                struct ms_v3 edge_avg = ms_math_avg(start, end);
                edge_points[face * mesh.degree + vert] = ms_math_avg(face_avg, edge_avg);
            } else {
                /* This is an edge of a hole */
                edge_points[face * mesh.degree + vert] = ms_math_avg(start, end);
            }
        }
    }
    
    /* Update points */
    struct ms_v3 *new_verts = malloc(mesh.primitives * mesh.degree * sizeof(struct ms_v3));
    
    for (u32 face = 0; face < mesh.primitives; ++face) {
        for (u32 vert = 0; vert < mesh.degree; ++vert) {
            struct ms_v3 old_vert = mesh.vertices[face * mesh.degree + vert];
            struct ms_v3 new_vert;
            
            u32 *adj_faces;
            struct ms_v3 *adj_edges;
            s32 nadj_faces = _vert_adjacent_faces(mesh, old_vert, &adj_faces);
            s32 nadj_edges = _vert_adjacent_edges(mesh, old_vert, &adj_edges);
            
            if (nadj_faces != nadj_edges) {
                //new_vert = old_vert;
#if 1
                
                /* This vertex is on an edge of a hole */
                u32 nedges_adj_to_hole = 0;
                struct ms_v3 avg_mid_edge_point = { 0 };
                
                for (s32 i = 0; i < nadj_edges; ++i) {
                    struct ms_v3 start = adj_edges[2 * i + 0];
                    struct ms_v3 end = adj_edges[2 * i + 1];
                    struct ms_v3 mid = ms_math_avg(start, end);
                    
                    /* Only take into account edges that are also on the edge of a hole */
                    u32 adj_face = _edge_adjacent_face(mesh, 0, start, end);
                    u32 another_adj_face = _edge_adjacent_face(mesh, adj_face, start, end);
                    if (adj_face == another_adj_face) {
                        ++nedges_adj_to_hole;
                        avg_mid_edge_point.x += mid.x;
                        avg_mid_edge_point.y += mid.y;
                        avg_mid_edge_point.z += mid.z;
                    }
                }
                
                new_vert.x = (avg_mid_edge_point.x + old_vert.x) / (nedges_adj_to_hole + 1);
                new_vert.y = (avg_mid_edge_point.y + old_vert.y) / (nedges_adj_to_hole + 1);
                new_vert.z = (avg_mid_edge_point.z + old_vert.z) / (nedges_adj_to_hole + 1);
#endif
            } else {
                
                /* Average of face points of all the faces this vertex is adjacent to */
                struct ms_v3 avg_face_point = { 0 };
                for (s32 i = 0; i < nadj_faces; ++i) {
                    avg_face_point.x += face_points[adj_faces[i]].x;
                    avg_face_point.y += face_points[adj_faces[i]].y;
                    avg_face_point.z += face_points[adj_faces[i]].z;
                }
                avg_face_point.x /= (f32) nadj_faces;
                avg_face_point.y /= (f32) nadj_faces;
                avg_face_point.z /= (f32) nadj_faces;
                
                /* Average of mid points of all the edges this vertex is adjacent to */
                struct ms_v3 avg_mid_edge_point = { 0 };
                for (s32 i = 0; i < nadj_edges; ++i) {
                    struct ms_v3 start = adj_edges[2 * i + 0];
                    struct ms_v3 end = adj_edges[2 * i + 1];
                    struct ms_v3 mid = ms_math_avg(start, end);
                    avg_mid_edge_point.x += mid.x;
                    avg_mid_edge_point.y += mid.y;
                    avg_mid_edge_point.z += mid.z;
                }
                avg_mid_edge_point.x /= (f32) nadj_edges;
                avg_mid_edge_point.y /= (f32) nadj_edges;
                avg_mid_edge_point.z /= (f32) nadj_edges;
                
                free(adj_faces);
                free(adj_edges);
                
                /* Weights */
                f32 w1 = (f32) (nadj_faces - 3) / (f32) nadj_faces;
                f32 w2 = 1.0f / (f32) nadj_faces;
                f32 w3 = 2.0f * w2;
                
                /* Weighted average to obtain a new vertex */
                new_vert.x = w1 * old_vert.x + w2 * avg_face_point.x + w3 * avg_mid_edge_point.x;
                new_vert.y = w1 * old_vert.y + w2 * avg_face_point.y + w3 * avg_mid_edge_point.y;
                new_vert.z = w1 * old_vert.z + w2 * avg_face_point.z + w3 * avg_mid_edge_point.z;
            }
            
            new_verts[face * mesh.degree + vert] = new_vert;
        }
    }
    
    /* Subdivide */
    struct ms_mesh new_mesh;
    new_mesh.primitives = mesh.primitives * mesh.degree;
    new_mesh.vertices = malloc(new_mesh.primitives * 4 * sizeof(struct ms_v3));
    
    for (u32 face = 0; face < mesh.primitives; ++face) {
        struct ms_v3 edge_point_ab = edge_points[face * mesh.degree + 0];
        struct ms_v3 edge_point_bc = edge_points[face * mesh.degree + 1];
        struct ms_v3 edge_point_ca = edge_points[face * mesh.degree + 2];
        struct ms_v3 face_point_abc = face_points[face];
        
        struct ms_v3 a = new_verts[face * mesh.degree + 0];
        struct ms_v3 b = new_verts[face * mesh.degree + 1];
        struct ms_v3 c = new_verts[face * mesh.degree + 2];
        
        /* First iteration of Catmull-Clark */
        if (mesh.degree == 3) {
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
        } else {
            /* All the following iterations of Catmull-Clark  */
            struct ms_v3 edge_point_cd = edge_point_ca;
            struct ms_v3 edge_point_da = edge_points[face * mesh.degree + 3];
            struct ms_v3 face_point_abcd = face_point_abc;
            struct ms_v3 d = new_verts[face * mesh.degree + 3];
            
            /* face 0 */
            new_mesh.vertices[face * 16 + 0] = a;
            new_mesh.vertices[face * 16 + 1] = edge_point_ab;
            new_mesh.vertices[face * 16 + 2] = face_point_abcd;
            new_mesh.vertices[face * 16 + 3] = edge_point_da;
            
            /* face 1 */
            new_mesh.vertices[face * 16 + 4] = b;
            new_mesh.vertices[face * 16 + 5] = edge_point_bc;
            new_mesh.vertices[face * 16 + 6] = face_point_abcd;
            new_mesh.vertices[face * 16 + 7] = edge_point_ab;
            
            /* face 2 */
            new_mesh.vertices[face * 16 + 8] = c;
            new_mesh.vertices[face * 16 + 9] = edge_point_cd;
            new_mesh.vertices[face * 16 + 10] = face_point_abcd;
            new_mesh.vertices[face * 16 + 11] = edge_point_bc;
            
            /* face 3 */
            new_mesh.vertices[face * 16 + 12] = d;
            new_mesh.vertices[face * 16 + 13] = edge_point_da;
            new_mesh.vertices[face * 16 + 14] = face_point_abcd;
            new_mesh.vertices[face * 16 + 15] = edge_point_cd;
        }
    }
    
    free(new_verts);
    free(face_points);
    free(edge_points);
    
    new_mesh.degree = 4;
    new_mesh.normals = NULL;
    
    return(new_mesh);
}