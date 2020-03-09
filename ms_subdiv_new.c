#define NOACCEL 1
#define HASH_PLAIN 0
#define HASH_LOCAL 0
#define EXPLICIT 0

static struct ms_vec
vert_adjacent_faces(struct ms_mesh mesh, int vertex)
{
    struct ms_vec result = ms_vec_init(4);
    
#if NOACCEL
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            if (vertex == mesh.faces[face * mesh.degree + vert]) {
                ms_vec_push(&result, face);
            }
        }
    }
#elif HASH_PLAIN
    assert(!"implemented");
#elif HASH_LOCAL
    assert(!"implemented");
#elif EXPLICIT
    assert(!"implemented");
#endif
    
    
    return(result);
}


static struct ms_vec
vert_adjacent_edges(struct ms_mesh mesh, int vertex)
{
    struct ms_vec result = ms_vec_init(4);
    
#if NOACCEL
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int next = (vert + 1) % mesh.degree;
            
            int start = mesh.faces[face * mesh.degree + vert];
            int end = mesh.faces[face * mesh.degree + next];
            
            if (start > end) { SWAP(start, end); }
            
            if (vertex == start || vertex == end) {
                bool found = false;
                for (int i = 0; i < result.len / 2; ++i) {
                    int other_start = result.data[2 * i + 0];
                    int other_end = result.data[2 * i + 1];
                    
                    if (other_start == start && other_end == end) {
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    ms_vec_push(&result, start);
                    ms_vec_push(&result, end);
                }
            }
        }
    }
#elif HASH_PLAIN
    assert(!"implemented");
#elif HASH_LOCAL
    assert(!"implemented");
#elif EXPLICIT
    assert(!"implemented");
#endif
    
    
    return(result);
}

static int
edge_adjacent_face(struct ms_mesh mesh, int me, int start, int end)
{
#if NOACCEL
    for (int face = 0; face < mesh.nfaces; ++face) {
        if (face == me) {
            continue;
        }
        
        bool found_start = false;
        bool found_end = false;
        
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int vertex = mesh.faces[face * mesh.degree + vert];
            if (vertex == start) { found_start = true; }
            if (vertex == end)   { found_end = true; }
        }
        
        if (found_start && found_end) {
            return(face);
        }
    }
#elif HASH_PLAIN
    assert(!"implemented");
#elif HASH_LOCAL
    assert(!"implemented");
#elif EXPLICIT
    assert(!"implemented");
#endif
    
    return(me);
}

static struct ms_mesh
ms_subdiv_catmull_clark_new(struct ms_mesh mesh)
{
    TracyCZone(__FUNC__, true);
    
    /* Construct acceleration structure */
#if NOACCEL
#elif HASH_PLAIN
    TracyCZoneN(construct_hashtable, "Construct hash table", true);
    TracyCZoneEnd(construct_hashtable);
#elif HASH_LOCAL
#elif EXPLICIT
#endif
    
    /* Face points */
    TracyCZoneN(compute_face_points, "face points", true);
    struct ms_v3 *face_points = malloc(mesh.nfaces * sizeof(struct ms_v3));
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        struct ms_v3 fp = { 0 };
        
        for (int vert = 0; vert < mesh.degree; ++vert) {
            struct ms_v3 vertex = mesh.vertices[mesh.faces[face * mesh.degree + vert]];
            fp.x += vertex.x;
            fp.y += vertex.y;
            fp.z += vertex.z;
        }
        
        fp.x /= (f32) mesh.degree;
        fp.y /= (f32) mesh.degree;
        fp.z /= (f32) mesh.degree;
        
        face_points[face] = fp;
    }
    TracyCZoneEnd(compute_face_points);
    
    /* Edge points */
    TracyCZoneN(compute_edge_points, "edge_points", true);
    
    struct ms_v3 *edge_pointsv = malloc(mesh.nfaces * mesh.degree * sizeof(struct ms_v3));
    struct ms_edgep *edgep_lookup = calloc(1, mesh.nverts * sizeof(struct ms_edgep));
    int nedge_pointsv = 0;
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int next_vert = (vert + 1) % mesh.degree;
            
            int start = mesh.faces[face * mesh.degree + vert];
            int end = mesh.faces[face * mesh.degree + next_vert];
            
            if (start > end) { SWAP(start, end); }
            
            // TODO: only compute if not found: move to if !found or lem == 0
            struct ms_v3 startv = mesh.vertices[start];
            struct ms_v3 endv = mesh.vertices[end];
            
            int adj = edge_adjacent_face(mesh, face, start, end);
            struct ms_v3 edge_point;
            
            if (adj != face) {
                struct ms_v3 face_avg = ms_math_avg(face_points[face], face_points[adj]);
                struct ms_v3 edge_avg = ms_math_avg(startv, endv);
                edge_point = ms_math_avg(face_avg, edge_avg);
            } else {
                /* This is an edge of a hole */
                edge_point = ms_math_avg(startv, endv);
            }
            
            struct ms_edgep *slot = edgep_lookup + start;
            int face_index = face * mesh.degree + vert;
            
            if (slot->ends.len == 0) {
                /* This is the first edge starting at this vertex */
                slot->ends = ms_vec_init(4);
                slot->value_indices = ms_vec_init(4);
                slot->face_indices = ms_vec_init(4);
                ms_vec_push(&slot->ends, end);
                ms_vec_push(&slot->value_indices, nedge_pointsv);
                
                /* One of two neighbours */
                ms_vec_push(&slot->face_indices, face_index);
                ms_vec_push(&slot->face_indices, -1);
                
                edge_pointsv[nedge_pointsv] = edge_point;
                ++nedge_pointsv;
            } else {
                /* There are other edges starting at this vertex */
                bool found = false;
                for (int i = 0; i < slot->ends.len; ++i) {
                    if (slot->ends.data[i] == end) {
                        /* Second neighbour. This edge point is already added */
                        slot->face_indices.data[i * 2 + 1] = face_index;
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    ms_vec_push(&slot->ends, end);
                    ms_vec_push(&slot->value_indices, nedge_pointsv);
                    
                    /* One of two neighbours */
                    ms_vec_push(&slot->face_indices, face_index);
                    ms_vec_push(&slot->face_indices, -1);
                    
                    edge_pointsv[nedge_pointsv] = edge_point;
                    ++nedge_pointsv;
                }
            }
        }
    }
    
    /* Convert lookup 'table' into an array */
    int *edge_points = malloc(mesh.nfaces * mesh.degree * sizeof(int));
    for (int start = 0; start < mesh.nverts; ++start) {
        struct ms_edgep *slot = edgep_lookup + start;
        
        for (int i = 0; i < slot->ends.len; ++i) {
            int first_index = slot->face_indices.data[i * 2 + 0];
            int second_index = slot->face_indices.data[i * 2 + 1];
            int value_index = slot->value_indices.data[i]; 
            
            edge_points[first_index] = value_index;
            if (second_index != -1) {
                edge_points[second_index] = value_index;
            }
        }
    }
    TracyCZoneEnd(compute_edge_points);
    
    /* Update points */
    TracyCZoneN(update_positions, "update old points", true);
    struct ms_v3 *new_verts = malloc(mesh.nverts * sizeof(struct ms_v3));
    
    for (int v = 0; v < mesh.nverts; ++v) {
        struct ms_v3 vertex = mesh.vertices[v];
        struct ms_v3 new_vert;
        
        struct ms_vec adj_faces = vert_adjacent_faces(mesh, v);
        struct ms_vec adj_edges = vert_adjacent_edges(mesh, v);
        
        if (adj_faces.len != adj_edges.len / 2) {
            /* This vertex is on an edge of a hole */
            int nedges_adj_to_hole = 0;
            struct ms_v3 avg_mid_edge_point = { 0 };
            
            for (int i = 0; i < adj_edges.len / 2; ++i) {
                int start = adj_edges.data[2 * i + 0];
                int end = adj_edges.data[2 * i + 1];
                
                struct ms_v3 startv = mesh.vertices[start];
                struct ms_v3 endv = mesh.vertices[end];
                struct ms_v3 mid = ms_math_avg(startv, endv);
                
                /* Only take into account edges that are also on the edge of a hole */
                int adj_face = edge_adjacent_face(mesh, 0, start, end);
                int another_adj_face = edge_adjacent_face(mesh, adj_face, start, end);
                if (adj_face == another_adj_face) {
                    ++nedges_adj_to_hole;
                    avg_mid_edge_point.x += mid.x;
                    avg_mid_edge_point.y += mid.y;
                    avg_mid_edge_point.z += mid.z;
                }
            }
            
            new_vert.x = (avg_mid_edge_point.x + vertex.x) / (nedges_adj_to_hole + 1);
            new_vert.y = (avg_mid_edge_point.y + vertex.y) / (nedges_adj_to_hole + 1);
            new_vert.z = (avg_mid_edge_point.z + vertex.z) / (nedges_adj_to_hole + 1);
        } else {
            /* Average of face points of all the faces this vertex is adjacent to */
            struct ms_v3 avg_face_point = { 0 };
            for (int i = 0; i < adj_faces.len; ++i) {
                avg_face_point.x += face_points[adj_faces.data[i]].x;
                avg_face_point.y += face_points[adj_faces.data[i]].y;
                avg_face_point.z += face_points[adj_faces.data[i]].z;
            }
            avg_face_point.x /= (f32) adj_faces.len;
            avg_face_point.y /= (f32) adj_faces.len;
            avg_face_point.z /= (f32) adj_faces.len;
            
            /* Average of mid points of all the edges this vertex is adjacent to */
            struct ms_v3 avg_mid_edge_point = { 0 };
            for (int i = 0; i < adj_edges.len / 2; ++i) {
                int start = adj_edges.data[2 * i + 0];
                int end = adj_edges.data[2 * i + 1];
                
                struct ms_v3 startv = mesh.vertices[start];
                struct ms_v3 endv = mesh.vertices[end];
                struct ms_v3 mid = ms_math_avg(startv, endv);
                
                avg_mid_edge_point.x += mid.x;
                avg_mid_edge_point.y += mid.y;
                avg_mid_edge_point.z += mid.z;
            }
            
            f32 norm_coeff = 1.0f / (adj_edges.len / 2.0f);
            avg_mid_edge_point.x *= norm_coeff;
            avg_mid_edge_point.y *= norm_coeff;
            avg_mid_edge_point.z *= norm_coeff;
            
            /* Weights */
            f32 w1 = (f32) (adj_faces.len - 3) / (f32) adj_faces.len;
            f32 w2 = 1.0f / (f32) adj_faces.len;
            f32 w3 = 2.0f * w2;
            
            /* Weighted average to obtain a new vertex */
            new_vert.x = w1 * vertex.x + w2 * avg_face_point.x + w3 * avg_mid_edge_point.x;
            new_vert.y = w1 * vertex.y + w2 * avg_face_point.y + w3 * avg_mid_edge_point.y;
            new_vert.z = w1 * vertex.z + w2 * avg_face_point.z + w3 * avg_mid_edge_point.z;
        }
        
        new_verts[v] = new_vert;
    }
    TracyCZoneEnd(update_positions);
    
    /* Subdivide */
    TracyCZoneN(subdivide, "do subdivision", true);
    
    struct ms_mesh new_mesh = { 0 };
    new_mesh.nfaces = mesh.nfaces * 4;
    
    /* Updated vertices + edge points + 1 face point per face */
    new_mesh.nverts = mesh.nverts + nedge_pointsv + mesh.nfaces;
    new_mesh.vertices = malloc(new_mesh.nverts * sizeof(struct ms_v3));
    
    new_mesh.faces = malloc(new_mesh.nfaces * 4 * sizeof(int));
    new_mesh.degree = 4;
    
    memcpy(new_mesh.vertices, new_verts, mesh.nverts * sizeof(struct ms_v3));
    memcpy(new_mesh.vertices + mesh.nverts, edge_pointsv, nedge_pointsv * sizeof(struct ms_v3));
    
    int vert_base = mesh.nverts + nedge_pointsv;
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        struct ms_v3 face_point_abcd = face_points[face];
        
        int face_base = face * 16;
        int edgep_base = mesh.nverts;
        int facep_index = vert_base + face;
        
        int edge_point_ab = edgep_base + edge_points[face * mesh.degree + 0];
        int edge_point_bc = edgep_base + edge_points[face * mesh.degree + 1];
        int edge_point_cd = edgep_base + edge_points[face * mesh.degree + 2];
        int edge_point_da = edgep_base + edge_points[face * mesh.degree + 3];
        
        int a = mesh.faces[face * mesh.degree + 0];
        int b = mesh.faces[face * mesh.degree + 1];
        int c = mesh.faces[face * mesh.degree + 2];
        int d = mesh.faces[face * mesh.degree + 3];
        
        /* Add face point */
        new_mesh.vertices[facep_index] = face_point_abcd;
        
        /* Add faces */
        {
            /* face 0 */
            new_mesh.faces[face_base + 0] = a;
            new_mesh.faces[face_base + 1] = edge_point_ab;
            new_mesh.faces[face_base + 2] = facep_index;
            new_mesh.faces[face_base + 3] = edge_point_da;
            
            /* face 1 */
            new_mesh.faces[face_base + 4] = b;
            new_mesh.faces[face_base + 5] = edge_point_bc;
            new_mesh.faces[face_base + 6] = facep_index;
            new_mesh.faces[face_base + 7] = edge_point_ab;
            
            /* face 2 */
            new_mesh.faces[face_base + 8] = c;
            new_mesh.faces[face_base + 9] = edge_point_cd;
            new_mesh.faces[face_base + 10] = facep_index;
            new_mesh.faces[face_base + 11] = edge_point_bc;
            
            /* face 3 */
            new_mesh.faces[face_base + 12] = d;
            new_mesh.faces[face_base + 13] = edge_point_da;
            new_mesh.faces[face_base + 14] = facep_index;
            new_mesh.faces[face_base + 15] = edge_point_cd;
        }
    }
    TracyCZoneEnd(subdivide);
    
    free(new_verts);
    free(face_points);
    free(edge_points);
    
    TracyCZoneEnd(__FUNC__);
    
    return(new_mesh);
}