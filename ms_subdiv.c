static struct ms_mesh
ms_subdiv_catmull_clark_new(struct ms_mesh mesh)
{
    TracyCZone(__FUNC__, true);
    
    /* Construct acceleration structure */
    struct ms_edges accel = init_acceleration_struct(mesh);
    
    TracyCZoneN(alloc_new_mesh, "allocate", true);
    /* Face points */
    struct ms_v3 *face_points = malloc64(mesh.nfaces * sizeof(struct ms_v3));
    
    /* Edge points */
    struct ms_v3 *edge_pointsv = malloc64(accel.count * sizeof(struct ms_v3));
    int *edge_points = malloc64(mesh.nfaces * 4 * sizeof(int));
    
    /* Update points */
    struct ms_v3 *new_verts = malloc64(mesh.nverts * sizeof(struct ms_v3));
    
    /* Subdivide */
    struct ms_mesh new_mesh = { 0 };
    new_mesh.nfaces = mesh.nfaces * 4;
    
    /* Updated vertices + edge points + 1 face point per face */
    new_mesh.faces = malloc64(new_mesh.nfaces * 4 * sizeof(int));
    TracyCZoneEnd(alloc_new_mesh);
    
    TracyCZoneN(compute_face_points, "face points", true);
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        struct ms_v3 vertex1 = mesh.vertices[mesh.faces[face * 4 + 0]];
        struct ms_v3 vertex2 = mesh.vertices[mesh.faces[face * 4 + 1]];
        struct ms_v3 vertex3 = mesh.vertices[mesh.faces[face * 4 + 2]];
        struct ms_v3 vertex4 = mesh.vertices[mesh.faces[face * 4 + 3]];
        
        face_points[face].x = (vertex1.x + vertex2.x + vertex3.x + vertex4.x) * 0.25f;
        face_points[face].y = (vertex1.y + vertex2.y + vertex3.y + vertex4.y) * 0.25f;
        face_points[face].z = (vertex1.z + vertex2.z + vertex3.z + vertex4.z) * 0.25f;
    }
    
    TracyCZoneEnd(compute_face_points);
    
    TracyCZoneN(compute_edge_points, "edge_points", true);
    
    for (int e = 0; e < accel.count; ++e) {
        struct ms_edge edge = accel.edges[e];
        
        struct ms_v3 startv = mesh.vertices[edge.start];
        struct ms_v3 endv = mesh.vertices[edge.end];
        
        int face = edge.face_1;
        int adj  = edge.face_2;
        
        struct ms_v3 edge_point;
        
        if (adj != face) {
            struct ms_v3 face_point_me = face_points[face];
            struct ms_v3 face_point_adj = face_points[adj];
            
            edge_point.x = (face_point_me.x + face_point_adj.x + startv.x + endv.x) * 0.25f;
            edge_point.y = (face_point_me.y + face_point_adj.y + startv.y + endv.y) * 0.25f;
            edge_point.z = (face_point_me.z + face_point_adj.z + startv.z + endv.z) * 0.25f;
        } else {
            /* This is an edge of a hole */
            edge_point.x = (startv.x + endv.x) * 0.5f;
            edge_point.y = (startv.y + endv.y) * 0.5f;
            edge_point.z = (startv.z + endv.z) * 0.5f;
        }
        
        edge_pointsv[e] = edge_point;
        
        edge_points[face * 4 + edge.findex_1] = e;
        edge_points[adj  * 4 + edge.findex_2] = e;
    }
    TracyCZoneEnd(compute_edge_points);
    
    TracyCZoneN(update_positions, "update old points", true);
    for (int v = 0; v < mesh.nverts; ++v) {
        struct ms_v3 vertex = mesh.vertices[v];
        struct ms_v3 new_vert;
        
        int adj_verts_base = accel.verts_starts[v];
        int adj_verts_count = accel.verts_starts[v + 1] - adj_verts_base;
        f32 norm_coeff = 1.0f / adj_verts_count;
        
        struct ms_v3 avg_face_point = { 0 };
        struct ms_v3 avg_mid_edge_point = { 0 };
        
        for (int i = 0; i < adj_verts_count; ++i) {
            /* Average of face points of all the faces this vertex is adjacent to */
            struct ms_v3 fp = face_points[accel.faces_matrix[adj_verts_base + i]];
            avg_face_point.x += fp.x;
            avg_face_point.y += fp.y;
            avg_face_point.z += fp.z;
            
            /* Average of mid points of all the edges this vertex is adjacent to */
            int end = accel.verts_matrix[adj_verts_base + i];
            struct ms_v3 endv = mesh.vertices[end];
            
            avg_mid_edge_point.x += (vertex.x + endv.x) * 0.5f;
            avg_mid_edge_point.y += (vertex.y + endv.y) * 0.5f;
            avg_mid_edge_point.z += (vertex.z + endv.z) * 0.5f;
        }
        
        avg_face_point.x *= norm_coeff;
        avg_face_point.y *= norm_coeff;
        avg_face_point.z *= norm_coeff;
        
        avg_mid_edge_point.x *= norm_coeff;
        avg_mid_edge_point.y *= norm_coeff;
        avg_mid_edge_point.z *= norm_coeff;
        
        /* Weights */
        f32 w1 = (f32) (adj_verts_count - 3) * norm_coeff;
        f32 w2 = norm_coeff;
        f32 w3 = 2.0f * w2;
        
        /* Weighted average to obtain a new vertex */
        new_vert.x = w1 * vertex.x + w2 * avg_face_point.x + w3 * avg_mid_edge_point.x;
        new_vert.y = w1 * vertex.y + w2 * avg_face_point.y + w3 * avg_mid_edge_point.y;
        new_vert.z = w1 * vertex.z + w2 * avg_face_point.z + w3 * avg_mid_edge_point.z;
        
        new_verts[v] = new_vert;
    }
    
    TracyCZoneEnd(update_positions);
    
    TracyCZoneN(copy_data, "copy unique points", true);
    new_mesh.nverts = mesh.nverts + accel.count + mesh.nfaces;
    new_mesh.vertices = malloc64(new_mesh.nverts * sizeof(struct ms_v3));
    
    memcpy(new_mesh.vertices, new_verts, mesh.nverts * sizeof(struct ms_v3));
    memcpy(new_mesh.vertices + mesh.nverts, edge_pointsv, accel.count * sizeof(struct ms_v3));
    memcpy(new_mesh.vertices + mesh.nverts + accel.count, face_points, mesh.nfaces * sizeof(struct ms_v3));
    TracyCZoneEnd(copy_data);
    
    TracyCZoneN(subdivide, "do subdivision", true);
    int vert_base = mesh.nverts + accel.count;
    int ep_base = mesh.nverts;
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        int face_base = face * 16;
        
        int a = mesh.faces[face * 4 + 0];
        int b = mesh.faces[face * 4 + 1];
        int c = mesh.faces[face * 4 + 2];
        int d = mesh.faces[face * 4 + 3];
        
        int edge_point_ab = ep_base + edge_points[face * 4 + 0];
        int edge_point_bc = ep_base + edge_points[face * 4 + 1];
        int edge_point_cd = ep_base + edge_points[face * 4 + 2];
        int edge_point_da = ep_base + edge_points[face * 4 + 3];
        
        int face_point = vert_base + face;
        
        /* Add faces */
        {
            /* face 0 */
            new_mesh.faces[face_base + 0] = a;
            new_mesh.faces[face_base + 1] = edge_point_ab;
            new_mesh.faces[face_base + 2] = face_point;
            new_mesh.faces[face_base + 3] = edge_point_da;
            
            /* face 1 */
            new_mesh.faces[face_base + 4] = b;
            new_mesh.faces[face_base + 5] = edge_point_bc;
            new_mesh.faces[face_base + 6] = face_point;
            new_mesh.faces[face_base + 7] = edge_point_ab;
            
            /* face 2 */
            new_mesh.faces[face_base + 8] = c;
            new_mesh.faces[face_base + 9] = edge_point_cd;
            new_mesh.faces[face_base + 10] = face_point;
            new_mesh.faces[face_base + 11] = edge_point_bc;
            
            /* face 3 */
            new_mesh.faces[face_base + 12] = d;
            new_mesh.faces[face_base + 13] = edge_point_da;
            new_mesh.faces[face_base + 14] = face_point;
            new_mesh.faces[face_base + 15] = edge_point_cd;
        }
    }
    TracyCZoneEnd(subdivide);
    
    
    free(accel.edges);
    
    free(accel.verts_starts);
    free(accel.verts_matrix);
    free(accel.faces_matrix);
    
    free(edge_pointsv);
    free(new_verts);
    free(edge_points);
    free(face_points);
    
    TracyCZoneEnd(__FUNC__);
    
    return(new_mesh);
}
