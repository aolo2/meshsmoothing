static struct ms_mesh
ms_subdiv_catmull_clark_tagged(struct ms_mesh *mesh)
{
    TracyCZone(__FUNC__, true);
    
    /* Construct acceleration structure */
    struct ms_edges accel = init_acceleration_struct(mesh);
    
    TracyCZoneN(alloc_new_mesh, "allocate", true);
    
    /* Face points */
    f32 *face_points_x = malloc64(mesh->nfaces * sizeof(f32));
    f32 *face_points_y = malloc64(mesh->nfaces * sizeof(f32));
    f32 *face_points_z = malloc64(mesh->nfaces * sizeof(f32));
    
    /* Edge points */
    f32 *edge_pointsv_x = malloc64(accel.count * sizeof(f32));
    f32 *edge_pointsv_y = malloc64(accel.count * sizeof(f32));
    f32 *edge_pointsv_z = malloc64(accel.count * sizeof(f32));
    int *edge_points    = malloc64(mesh->nfaces * 4 * sizeof(int));
    
    /* Update points */
    struct ms_vertex *pv = calloc64(mesh->nverts * sizeof(struct ms_vertex));
    
    f32 *new_verts_x = malloc64(mesh->nverts * sizeof(f32));
    f32 *new_verts_y = malloc64(mesh->nverts * sizeof(f32));
    f32 *new_verts_z = malloc64(mesh->nverts * sizeof(f32));
    
    /* Subdivide */
    struct ms_mesh new_mesh = { 0 };
    new_mesh.nfaces = mesh->nfaces * 4;
    
    /* Updated vertices + edge points + 1 face point per face */
    new_mesh.faces = malloc64(new_mesh.nfaces * 4 * sizeof(int));
    
    TracyCZoneEnd(alloc_new_mesh);
    
    TracyCZoneN(compute_face_points, "face points", true);
    
    //#pragma omp parallel for
    for (int face = 0; face < mesh->nfaces; ++face) {
        int v1 = mesh->faces[face * 4 + 0];
        int v2 = mesh->faces[face * 4 + 1];
        int v3 = mesh->faces[face * 4 + 2];
        int v4 = mesh->faces[face * 4 + 3];
        
        face_points_x[face] = (mesh->vertices_x[v1] + mesh->vertices_x[v2] + 
                               mesh->vertices_x[v3] + mesh->vertices_x[v4]) * 0.25f;
        
        face_points_y[face] = (mesh->vertices_y[v1] + mesh->vertices_y[v2] + 
                               mesh->vertices_y[v3] + mesh->vertices_y[v4]) * 0.25f;
        
        face_points_z[face] = (mesh->vertices_z[v1] + mesh->vertices_z[v2] + 
                               mesh->vertices_z[v3] + mesh->vertices_z[v4]) * 0.25f;
    }
    
    TracyCZoneEnd(compute_face_points);
    
    TracyCZoneN(compute_edge_points, "edge_points", true);
    
    //#pragma omp parallel for
    for (int e = 0; e < accel.count; ++e) {
        struct ms_edge edge = accel.edges[e];
        
        int face  = edge.face_1;
        int adj   = edge.face_2;
        int start = edge.start;
        int end   = edge.end;
        
        pv[start].nedges += 1;
        pv[end].nedges += 1;
        
        if (face != adj) {
            edge_pointsv_x[e] = (face_points_x[face] + face_points_x[adj] + mesh->vertices_x[start] + mesh->vertices_x[end]) * 0.25f;
            edge_pointsv_y[e] = (face_points_y[face] + face_points_y[adj] + mesh->vertices_y[start] + mesh->vertices_y[end]) * 0.25f;
            edge_pointsv_z[e] = (face_points_z[face] + face_points_z[adj] + mesh->vertices_z[start] + mesh->vertices_z[end]) * 0.25f;
        } else {
            edge_pointsv_x[e] = (mesh->vertices_x[start] + mesh->vertices_x[end]) * 0.5f;
            edge_pointsv_y[e] = (mesh->vertices_y[start] + mesh->vertices_y[end]) * 0.5f;
            edge_pointsv_z[e] = (mesh->vertices_z[start] + mesh->vertices_z[end]) * 0.5f;
            
            pv[start].smex += edge_pointsv_x[e];
            pv[start].smey += edge_pointsv_y[e];
            pv[start].smez += edge_pointsv_z[e];
            
            pv[end].smex += edge_pointsv_x[e];
            pv[end].smey += edge_pointsv_y[e];
            pv[end].smez += edge_pointsv_z[e];
        }
        
        edge_points[face * 4 + edge.findex_1] = e;
        edge_points[adj  * 4 + edge.findex_2] = e;
    }
    TracyCZoneEnd(compute_edge_points);
    
    TracyCZoneN(average_compute, "average compute", true);
    for (int face = 0; face < mesh->nfaces; ++face) {
        int v1 = mesh->faces[face * 4 + 0];
        int v2 = mesh->faces[face * 4 + 1];
        int v3 = mesh->faces[face * 4 + 2];
        int v4 = mesh->faces[face * 4 + 3];
        
        f32 x1 = mesh->vertices_x[v1];
        f32 y1 = mesh->vertices_y[v1];
        f32 z1 = mesh->vertices_z[v1];
        
        f32 x2 = mesh->vertices_x[v2];
        f32 y2 = mesh->vertices_y[v2];
        f32 z2 = mesh->vertices_z[v2];
        
        f32 x3 = mesh->vertices_x[v3];
        f32 y3 = mesh->vertices_y[v3];
        f32 z3 = mesh->vertices_z[v3];
        
        f32 x4 = mesh->vertices_x[v4];
        f32 y4 = mesh->vertices_y[v4];
        f32 z4 = mesh->vertices_z[v4];
        
        f32 fx = face_points_x[face];
        f32 fy = face_points_y[face];
        f32 fz = face_points_z[face];
        
        f32 mex12 = (x1 + x2) * 0.5f;
        f32 mey12 = (y1 + y2) * 0.5f;
        f32 mez12 = (z1 + z2) * 0.5f;
        
        f32 mex23 = (x2 + x3) * 0.5f;
        f32 mey23 = (y2 + y3) * 0.5f;
        f32 mez23 = (z2 + z3) * 0.5f;
        
        f32 mex34 = (x3 + x4) * 0.5f;
        f32 mey34 = (y3 + y4) * 0.5f;
        f32 mez34 = (z3 + z4) * 0.5f;
        
        f32 mex41 = (x4 + x1) * 0.5f;
        f32 mey41 = (y4 + y1) * 0.5f;
        f32 mez41 = (z4 + z1) * 0.5f;
        
        /* v1 */
        pv[v1].fpx += fx;
        pv[v1].fpy += fy;
        pv[v1].fpz += fz;
        
        pv[v1].mex += mex41;
        pv[v1].mey += mey41;
        pv[v1].mez += mez41;
        
        pv[v1].mex += mex12;
        pv[v1].mey += mey12;
        pv[v1].mez += mez12;
        
        pv[v1].nfaces += 1;
        
        /* v2 */
        pv[v2].fpx += fx;
        pv[v2].fpy += fy;
        pv[v2].fpz += fz;
        
        pv[v2].mex += mex12;
        pv[v2].mey += mey12;
        pv[v2].mez += mez12;
        
        pv[v2].mex += mex23;
        pv[v2].mey += mey23;
        pv[v2].mez += mez23;
        
        pv[v2].nfaces += 1;
        
        /* v3 */
        pv[v3].fpx += fx;
        pv[v3].fpy += fy;
        pv[v3].fpz += fz;
        
        pv[v3].mex += mex23;
        pv[v3].mey += mey23;
        pv[v3].mez += mez23;
        
        pv[v3].mex += mex34;
        pv[v3].mey += mey34;
        pv[v3].mez += mez34;
        
        pv[v3].nfaces += 1;
        
        /* v4 */
        pv[v4].fpx += fx;
        pv[v4].fpy += fy;
        pv[v4].fpz += fz;
        
        pv[v4].mex += mex34;
        pv[v4].mey += mey34;
        pv[v4].mez += mez34;
        
        pv[v4].mex += mex41;
        pv[v4].mey += mey41;
        pv[v4].mez += mez41;
        
        pv[v4].nfaces += 1;
    }
    TracyCZoneEnd(average_compute);
    
    TracyCZoneN(update_positions, "update old points", true);
    for (int v = 0; v < mesh->nverts; ++v) {
        struct ms_vertex pvv = pv[v];
        
        f32 norm_coeff = 1.0f / pvv.nfaces;
        f32 norm_coeff2 = 0.5f / pvv.nedges;
        
        f32 vertex_x = mesh->vertices_x[v];
        f32 vertex_y = mesh->vertices_y[v];
        f32 vertex_z = mesh->vertices_z[v];
        
        if (pvv.nfaces == pvv.nedges) {
            /* Weights */
            f32 w1 = pvv.nfaces - 3.0f;
            f32 w2 = 1.0f;
            f32 w3 = 2.0f;
            
            /* Weighted average to obtain a new vertex */
            new_verts_x[v] = norm_coeff * (w1 * vertex_x + w2 * pvv.fpx * norm_coeff + w3 * pvv.mex * norm_coeff2);
            new_verts_y[v] = norm_coeff * (w1 * vertex_y + w2 * pvv.fpy * norm_coeff + w3 * pvv.mey * norm_coeff2);
            new_verts_z[v] = norm_coeff * (w1 * vertex_z + w2 * pvv.fpz * norm_coeff + w3 * pvv.mez * norm_coeff2);
        } else {
            new_verts_x[v] = (pvv.smex + vertex_x) / 3.0f;
            new_verts_y[v] = (pvv.smey + vertex_y) / 3.0f;
            new_verts_z[v] = (pvv.smez + vertex_z) / 3.0f;
        }
    }
    
    TracyCZoneEnd(update_positions);
    
    TracyCZoneN(copy_data, "copy unique points", true);
    new_mesh.nverts = mesh->nverts + accel.count + mesh->nfaces;
    
    new_mesh.vertices_x = malloc64(new_mesh.nverts * sizeof(f32));
    new_mesh.vertices_y = malloc64(new_mesh.nverts * sizeof(f32));
    new_mesh.vertices_z = malloc64(new_mesh.nverts * sizeof(f32));
    
    memcpy(new_mesh.vertices_x, new_verts_x, mesh->nverts * sizeof(f32));
    memcpy(new_mesh.vertices_y, new_verts_y, mesh->nverts * sizeof(f32));
    memcpy(new_mesh.vertices_z, new_verts_z, mesh->nverts * sizeof(f32));
    
    memcpy(new_mesh.vertices_x + mesh->nverts, edge_pointsv_x, accel.count * sizeof(f32));
    memcpy(new_mesh.vertices_y + mesh->nverts, edge_pointsv_y, accel.count * sizeof(f32));
    memcpy(new_mesh.vertices_z + mesh->nverts, edge_pointsv_z, accel.count * sizeof(f32));
    
    memcpy(new_mesh.vertices_x + mesh->nverts + accel.count, face_points_x, mesh->nfaces * sizeof(f32));
    memcpy(new_mesh.vertices_y + mesh->nverts + accel.count, face_points_y, mesh->nfaces * sizeof(f32));
    memcpy(new_mesh.vertices_z + mesh->nverts + accel.count, face_points_z, mesh->nfaces * sizeof(f32));
    
    TracyCZoneEnd(copy_data);
    
    TracyCZoneN(subdivide, "do subdivision", true);
    int vert_base = mesh->nverts + accel.count;
    int ep_base = mesh->nverts;
    
    int face_base = 0;
    new_mesh.nfaces = 0;
    
    //#pragma omp parallel for
    for (int face = 0; face < mesh->nfaces; ++face) {
        int a = mesh->faces[face * 4 + 0];
        int b = mesh->faces[face * 4 + 1];
        int c = mesh->faces[face * 4 + 2];
        int d = mesh->faces[face * 4 + 3];
        
        int edge_point_ab = ep_base + edge_points[face * 4 + 0];
        int edge_point_bc = ep_base + edge_points[face * 4 + 1];
        int edge_point_cd = ep_base + edge_points[face * 4 + 2];
        int edge_point_da = ep_base + edge_points[face * 4 + 3];
        
        int face_point = vert_base + face;
        
        /* Add faces */
        {
            /* face 0 */
            if (!mesh->halo[a]) {
                new_mesh.faces[face_base + 0] = a;
                new_mesh.faces[face_base + 1] = edge_point_ab;
                new_mesh.faces[face_base + 2] = face_point;
                new_mesh.faces[face_base + 3] = edge_point_da;
                
                face_base += 4;
                new_mesh.nfaces += 1;
            }
            
            /* face 1 */
            if (!mesh->halo[b]) {
                new_mesh.faces[face_base + 0] = b;
                new_mesh.faces[face_base + 1] = edge_point_bc;
                new_mesh.faces[face_base + 2] = face_point;
                new_mesh.faces[face_base + 3] = edge_point_ab;
                
                face_base += 4;
                new_mesh.nfaces += 1;
            }
            
            /* face 2 */
            if (!mesh->halo[c]) {
                new_mesh.faces[face_base + 0] = c;
                new_mesh.faces[face_base + 1] = edge_point_cd;
                new_mesh.faces[face_base + 2] = face_point;
                new_mesh.faces[face_base + 3] = edge_point_bc;
                
                face_base += 4;
                new_mesh.nfaces += 1;
            }
            
            /* face 3 */
            if (!mesh->halo[d]) {
                new_mesh.faces[face_base + 0] = d;
                new_mesh.faces[face_base + 1] = edge_point_da;
                new_mesh.faces[face_base + 2] = face_point;
                new_mesh.faces[face_base + 3] = edge_point_cd;
                
                face_base += 4;
                new_mesh.nfaces += 1;
            }
        }
    }
    TracyCZoneEnd(subdivide);
    
    free(accel.edges);
    
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
    
    
    free(pv);
    
    TracyCZoneEnd(__FUNC__);
    
    return(new_mesh);
}
