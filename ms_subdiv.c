static struct ms_mesh
ms_subdiv_catmull_clark_new(struct ms_mesh mesh)
{
    TracyCZone(__FUNC__, true);
    
    /* Construct acceleration structure */
    struct ms_edges accel = init_acceleration_struct(mesh);
    
    TracyCZoneN(alloc_new_mesh, "allocate", true);
    
    /* Face points */
    f32 *face_points_x = malloc64(mesh.nfaces * sizeof(f32));
    f32 *face_points_y = malloc64(mesh.nfaces * sizeof(f32));
    f32 *face_points_z = malloc64(mesh.nfaces * sizeof(f32));
    
    /* Edge points */
    f32 *edge_pointsv_x = malloc64(accel.count * sizeof(f32));
    f32 *edge_pointsv_y = malloc64(accel.count * sizeof(f32));
    f32 *edge_pointsv_z = malloc64(accel.count * sizeof(f32));
    int *edge_points    = malloc64(mesh.nfaces * 4 * sizeof(int));
    
    /* Update points */
    f32 *new_verts_x = malloc64(mesh.nverts * sizeof(f32));
    f32 *new_verts_y = malloc64(mesh.nverts * sizeof(f32));
    f32 *new_verts_z = malloc64(mesh.nverts * sizeof(f32));
    
    /* Subdivide */
    struct ms_mesh new_mesh = { 0 };
    new_mesh.nfaces = mesh.nfaces * 4;
    
    /* Updated vertices + edge points + 1 face point per face */
    new_mesh.faces = malloc64(new_mesh.nfaces * 4 * sizeof(int));
    TracyCZoneEnd(alloc_new_mesh);
    
    TracyCZoneN(compute_face_points, "face points", true);
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        int v1 = mesh.faces[face * 4 + 0];
        int v2 = mesh.faces[face * 4 + 1];
        int v3 = mesh.faces[face * 4 + 2];
        int v4 = mesh.faces[face * 4 + 3];
        
        f32 vertex1_x = mesh.vertices_x[v1];
        f32 vertex1_y = mesh.vertices_y[v1];
        f32 vertex1_z = mesh.vertices_z[v1];
        
        f32 vertex2_x = mesh.vertices_x[v2];
        f32 vertex2_y = mesh.vertices_y[v2];
        f32 vertex2_z = mesh.vertices_z[v2];
        
        f32 vertex3_x = mesh.vertices_x[v3];
        f32 vertex3_y = mesh.vertices_y[v3];
        f32 vertex3_z = mesh.vertices_z[v3];
        
        f32 vertex4_x = mesh.vertices_x[v4];
        f32 vertex4_y = mesh.vertices_y[v4];
        f32 vertex4_z = mesh.vertices_z[v4];
        
        face_points_x[face] = (vertex1_x + vertex2_x + vertex3_x + vertex4_x) * 0.25f;
        face_points_y[face] = (vertex1_y + vertex2_y + vertex3_y + vertex4_y) * 0.25f;
        face_points_z[face] = (vertex1_z + vertex2_z + vertex3_z + vertex4_z) * 0.25f;
    }
    
    TracyCZoneEnd(compute_face_points);
    
    TracyCZoneN(compute_edge_points, "edge_points", true);
    
    for (int e = 0; e < accel.count; ++e) {
        struct ms_edge edge = accel.edges[e];
        
        f32 startv_x = mesh.vertices_x[edge.start];
        f32 startv_y = mesh.vertices_y[edge.start];
        f32 startv_z = mesh.vertices_z[edge.start];
        
        f32 endv_x = mesh.vertices_x[edge.end];
        f32 endv_y = mesh.vertices_y[edge.end];
        f32 endv_z = mesh.vertices_z[edge.end];
        
        int face = edge.face_1;
        int adj  = edge.face_2;
        
        f32 face_point_me_x = face_points_x[face];
        f32 face_point_me_y = face_points_y[face];
        f32 face_point_me_z = face_points_z[face];
        
        f32 face_point_adj_x = face_points_x[adj];
        f32 face_point_adj_y = face_points_y[adj];
        f32 face_point_adj_z = face_points_z[adj];
        
        edge_pointsv_x[e] = (face_point_me_x + face_point_adj_x + startv_x + endv_x) * 0.25f;
        edge_pointsv_y[e] = (face_point_me_y + face_point_adj_y + startv_y + endv_y) * 0.25f;
        edge_pointsv_z[e] = (face_point_me_z + face_point_adj_z + startv_z + endv_z) * 0.25f;
        
        edge_points[face * 4 + edge.findex_1] = e;
        edge_points[adj  * 4 + edge.findex_2] = e;
    }
    TracyCZoneEnd(compute_edge_points);
    
    TracyCZoneN(update_positions, "update old points", true);
    for (int v = 0; v < mesh.nverts; ++v) {
        f32 vertex_x = mesh.vertices_x[v];
        f32 vertex_y = mesh.vertices_y[v];
        f32 vertex_z = mesh.vertices_z[v];
        
        int adj_verts_base = accel.verts_starts[v];
        int adj_verts_count = accel.verts_starts[v + 1] - adj_verts_base;
        f32 norm_coeff = 1.0f / adj_verts_count;
        
        f32 avg_face_point_x = 0.0f;
        f32 avg_face_point_y = 0.0f;
        f32 avg_face_point_z = 0.0f;
        
        f32 avg_mid_edge_point_x = 0.0f;
        f32 avg_mid_edge_point_y = 0.0f;
        f32 avg_mid_edge_point_z = 0.0f;
        
        for (int i = 0; i < adj_verts_count; ++i) {
            /* Average of face points of all the faces this vertex is adjacent to */
            avg_face_point_x += face_points_x[accel.faces_matrix[adj_verts_base + i]];
            avg_face_point_y += face_points_y[accel.faces_matrix[adj_verts_base + i]];
            avg_face_point_z += face_points_z[accel.faces_matrix[adj_verts_base + i]];
            
            /* Average of mid points of all the edges this vertex is adjacent to */
            int end = accel.verts_matrix[adj_verts_base + i];
            avg_mid_edge_point_x += (vertex_x + mesh.vertices_x[end]) * 0.5f;
            avg_mid_edge_point_y += (vertex_y + mesh.vertices_y[end]) * 0.5f;
            avg_mid_edge_point_z += (vertex_z + mesh.vertices_z[end]) * 0.5f;
        }
        
        avg_face_point_x *= norm_coeff;
        avg_face_point_y *= norm_coeff;
        avg_face_point_z *= norm_coeff;
        
        avg_mid_edge_point_x *= norm_coeff;
        avg_mid_edge_point_y *= norm_coeff;
        avg_mid_edge_point_z *= norm_coeff;
        
        /* Weights */
        f32 w1 = (f32) (adj_verts_count - 3) * norm_coeff;
        f32 w2 = norm_coeff;
        f32 w3 = 2.0f * w2;
        
        /* Weighted average to obtain a new vertex */
        new_verts_x[v] = w1 * vertex_x + w2 * avg_face_point_x + w3 * avg_mid_edge_point_x;
        new_verts_y[v] = w1 * vertex_y + w2 * avg_face_point_y + w3 * avg_mid_edge_point_y;
        new_verts_z[v] = w1 * vertex_z + w2 * avg_face_point_z + w3 * avg_mid_edge_point_z;
    }
    
    TracyCZoneEnd(update_positions);
    
    TracyCZoneN(copy_data, "copy unique points", true);
    new_mesh.nverts = mesh.nverts + accel.count + mesh.nfaces;
    
    new_mesh.vertices_x = malloc64(new_mesh.nverts * sizeof(f32));
    new_mesh.vertices_y = malloc64(new_mesh.nverts * sizeof(f32));
    new_mesh.vertices_z = malloc64(new_mesh.nverts * sizeof(f32));
    
    memcpy(new_mesh.vertices_x, new_verts_x, mesh.nverts * sizeof(f32));
    memcpy(new_mesh.vertices_y, new_verts_y, mesh.nverts * sizeof(f32));
    memcpy(new_mesh.vertices_z, new_verts_z, mesh.nverts * sizeof(f32));
    
    memcpy(new_mesh.vertices_x + mesh.nverts, edge_pointsv_x, accel.count * sizeof(f32));
    memcpy(new_mesh.vertices_y + mesh.nverts, edge_pointsv_y, accel.count * sizeof(f32));
    memcpy(new_mesh.vertices_z + mesh.nverts, edge_pointsv_z, accel.count * sizeof(f32));
    
    memcpy(new_mesh.vertices_x + mesh.nverts + accel.count, face_points_x, mesh.nfaces * sizeof(f32));
    memcpy(new_mesh.vertices_y + mesh.nverts + accel.count, face_points_y, mesh.nfaces * sizeof(f32));
    memcpy(new_mesh.vertices_z + mesh.nverts + accel.count, face_points_z, mesh.nfaces * sizeof(f32));
    
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
    
    free(edge_pointsv_x);
    free(edge_pointsv_y);
    free(edge_pointsv_z);
    
    free(new_verts_x);
    free(new_verts_y);
    free(new_verts_z);
    
    free(face_points_x);
    free(face_points_y);
    free(face_points_z);
    
    free(edge_points);
    
    TracyCZoneEnd(__FUNC__);
    
    return(new_mesh);
}
