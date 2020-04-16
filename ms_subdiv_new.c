#include "ms_subdiv_linear.c"

static struct ms_mesh
ms_subdiv_catmull_clark_new(struct ms_mesh mesh)
{
    TracyCZone(__FUNC__, true);
    
    /* Construct acceleration structure */
    struct ms_accel accel = init_acceleration_struct(mesh);
    
    /* Face points */
    TracyCZoneN(compute_face_points, "face points", true);
    struct ms_v3 *face_points = malloc(mesh.nfaces * sizeof(struct ms_v3));
    f32 one_over_mesh_degree = 1.0f / mesh.degree;
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        struct ms_v3 fp = { 0 };
        
        for (int vert = 0; vert < mesh.degree; ++vert) {
            struct ms_v3 vertex = mesh.vertices[mesh.faces[face * mesh.degree + vert]];
            fp.x += vertex.x;
            fp.y += vertex.y;
            fp.z += vertex.z;
        }
        
        fp.x *= one_over_mesh_degree;
        fp.y *= one_over_mesh_degree;
        fp.z *= one_over_mesh_degree;
        
        face_points[face] = fp;
    }
    TracyCZoneEnd(compute_face_points);
    
    /* Edge points */
    TracyCZoneN(compute_edge_points, "edge_points", true);
    
    TracyCZoneN(edge_point_bin, "bin edges by start", true);
    int *end_counts = calloc(1, (mesh.nverts + 1) * sizeof(int));
    int nedges = 0;
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int edge = face * mesh.degree + vert;
            int next_vert = (vert + 1) % mesh.degree;
            int start = mesh.faces[edge];
            int end = mesh.faces[face * mesh.degree + next_vert];
            if (start > end) { SWAP(start, end) }
            
            end_counts[start + 1] += 1;
            
            ++nedges;
        }
    }
    
    for (int v = 1; v < mesh.nverts + 1; ++v) {
        end_counts[v] += end_counts[v - 1];
    }
    
    int *edge_points = malloc(mesh.nfaces * mesh.degree * sizeof(int));
    int *accum = calloc(1, mesh.nverts * sizeof(int));
    int *ends = malloc(nedges * sizeof(int));
    int *edges = malloc(nedges * sizeof(int));
    int *faces = malloc(nedges * sizeof(int));
    struct ms_v3 *edge_pointsv = malloc(nedges * 2 * sizeof(struct ms_v3));
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int edge = face * mesh.degree + vert;
            int next_vert = (vert + 1) % mesh.degree;
            int start = mesh.faces[edge];
            int end = mesh.faces[face * mesh.degree + next_vert];
            
            if (start > end) { SWAP(start, end) }
            
            int count = accum[start];
            int base = end_counts[start];
            
            faces[base + count] = face;
            ends[base + count] = end;
            edges[base + count] = edge;
            
            accum[start]++;
        }
    }
    
    TracyCZoneEnd(edge_point_bin);
    
    
    TracyCZoneN(edge_points_compute, "compute unique edge points", true);
    int nedge_pointsv = 0;
    
    for (int start = 0; start < mesh.nverts; ++start) {
        int from = end_counts[start];
        int to = end_counts[start + 1];
        
        for (int e = from; e < to; ++e) {
            int face = faces[e];
            int edge = edges[e];
            int end = ends[e];
            
            int found = -1;
            for (int e2 = from; e2 < e; ++e2) {
                if (ends[e2] == end) {
                    int edge2 = edges[e2];
                    found = edge_points[edge2];
                    break;
                }
            }
            
            if (found == -1) {
                struct ms_v3 edge_point;
                struct ms_v3 startv = mesh.vertices[start];
                struct ms_v3 endv = mesh.vertices[end];
                
                int adj = edge_adjacent_face(&accel, face, start, end);
                
                if (adj != face) {
                    struct ms_v3 face_avg = ms_math_avg(face_points[face], face_points[adj]);
                    struct ms_v3 edge_avg = ms_math_avg(startv, endv);
                    edge_point = ms_math_avg(face_avg, edge_avg);
                } else {
                    /* This is an edge of a hole */
                    edge_point = ms_math_avg(startv, endv);
                }
                
                edge_pointsv[nedge_pointsv] = edge_point;
                edge_points[edge] = nedge_pointsv;
                ++nedge_pointsv;
            } else {
                edge_points[edge] = found;
            }
        }
    }
    TracyCZoneEnd(edge_points_compute);
    TracyCZoneEnd(compute_edge_points);
    
    /* Update points */
    TracyCZoneN(update_positions, "update old points", true);
    struct ms_v3 *new_verts = malloc(mesh.nverts * sizeof(struct ms_v3));
    
    for (int v = 0; v < mesh.nverts; ++v) {
        struct ms_v3 vertex = mesh.vertices[v];
        struct ms_v3 new_vert;
        
        int adj_verts_base = accel.verts_starts[v];
        int adj_faces_base = accel.faces_starts[v];
        
        int adj_verts_count = accel.verts_starts[v + 1] - adj_verts_base;
        int adj_faces_count = accel.faces_starts[v + 1] - adj_faces_base;
        
        f32 one_over_adj_faces_count = 1.0f / adj_faces_count;
        
        if (adj_faces_count != adj_verts_count) {
            /* This vertex is on an edge of a hole */
            int nedges_adj_to_hole = 0;
            struct ms_v3 avg_mid_edge_point = { 0 };
            
            for (int i = 0; i < adj_verts_count; ++i) {
                int start = v;
                int end = accel.verts_matrix[adj_verts_base + i];
                
                struct ms_v3 startv = mesh.vertices[start];
                struct ms_v3 endv = mesh.vertices[end];
                struct ms_v3 mid = ms_math_avg(startv, endv);
                
                /* Only take into account edges that are also on the edge of a hole */
                int adj_face = edge_adjacent_face(&accel, 0, start, end);
                int another_adj_face = edge_adjacent_face(&accel, adj_face, start, end);
                
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
            for (int i = 0; i < adj_faces_count; ++i) {
                struct ms_v3 fp = face_points[accel.faces_matrix[adj_faces_base + i]];
                avg_face_point.x += fp.x;
                avg_face_point.y += fp.y;
                avg_face_point.z += fp.z;
            }
            avg_face_point.x *= one_over_adj_faces_count;
            avg_face_point.y *= one_over_adj_faces_count;
            avg_face_point.z *= one_over_adj_faces_count;
            
            /* Average of mid points of all the edges this vertex is adjacent to */
            struct ms_v3 avg_mid_edge_point = { 0 };
            for (int i = 0; i < adj_verts_count; ++i) {
                int start = v;
                int end = accel.verts_matrix[adj_verts_base + i];
                
                struct ms_v3 startv = mesh.vertices[start];
                struct ms_v3 endv = mesh.vertices[end];
                struct ms_v3 mid = ms_math_avg(startv, endv);
                
                avg_mid_edge_point.x += mid.x;
                avg_mid_edge_point.y += mid.y;
                avg_mid_edge_point.z += mid.z;
            }
            
            f32 norm_coeff = 1.0f / adj_verts_count;
            avg_mid_edge_point.x *= norm_coeff;
            avg_mid_edge_point.y *= norm_coeff;
            avg_mid_edge_point.z *= norm_coeff;
            
            /* Weights */
            f32 w1 = (f32) (adj_faces_count - 3) * one_over_adj_faces_count;
            f32 w2 = one_over_adj_faces_count;
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
    
    free_acceleration_struct(&accel);
    
    free(end_counts);
    
    free(edge_pointsv);
    free(new_verts);
    free(face_points);
    free(edge_points);
    
    free(accum);
    free(ends);
    free(edges);
    free(faces);
    
    TracyCZoneEnd(__FUNC__);
    
    return(new_mesh);
}