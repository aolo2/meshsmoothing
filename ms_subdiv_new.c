#ifndef ADJACENCY_ACCEL
#define ADJACENCY_ACCEL 0
#endif

struct ms_accel;

#include "ms_vec.c"

#if ADJACENCY_ACCEL == 0
#include "ms_subdiv_noaccel.c"
#elif ADJACENCY_ACCEL == 1 || ADJACENCY_ACCEL == 2
#include "ms_hash.c"
#include "ms_subdiv_hash.c"
#elif ADJACENCY_ACCEL == 3
#include "ms_subdiv_linear.c"
#else
#error ADJACENCY_ACCEL must be 0, 1, or 2
#endif

static int
compare_edges(const void *pa, const void *pb)
{
    const int *s1 = pa;
    const int *s2 = pb;
    
    int diff = *s1 - *s2;
    
    if (diff) {
        return(diff);
    }
    
    const int *e1 = s1 + 1;
    const int *e2 = s2 + 1;
    
    return(*e1 - *e2);
}

static struct ms_mesh
ms_subdiv_catmull_clark_new(struct ms_mesh mesh)
{
    TracyCZone(__FUNC__, true);
    
    /* Construct acceleration structure */
    struct ms_accel accel = init_hashtable(mesh);
    
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
#if 0
    TracyCZoneN(edge_point_table, "edge_points construct table", true);
    struct ms_v3 *edge_pointsv = malloc(mesh.nfaces * mesh.degree * sizeof(struct ms_v3));
    struct ms_edgep *edgep_lookup = calloc(1, mesh.nverts * sizeof(struct ms_edgep));
    int nedge_pointsv = 0;
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int next_vert = (vert + 1) % mesh.degree;
            
            int start = mesh.faces[face * mesh.degree + vert];
            int end = mesh.faces[face * mesh.degree + next_vert];
            
            if (start > end) { SWAP(start, end) }
            
            int adj = edge_adjacent_face(&accel, mesh, face, start, end);
            
            struct ms_edgep *slot = edgep_lookup + start;
            int face_index = face * mesh.degree + vert;
            
            bool found = false;
            for (int i = 0; i < slot->ends.len; ++i) {
                if (slot->ends.data[i] == end) {
                    /* Second neighbour. This edge point is already added */
                    slot->face_indices.data[i * 2 + 1] = face_index;
                    found = true;
                    break;
                }
            }
            
            if (slot->ends.len == 0 || !found) {
                /* This is the first edge starting at this vertex */
                if (slot->ends.len == 0) {
                    slot->ends = ms_vec_init(4);
                    slot->value_indices = ms_vec_init(4);
                    slot->face_indices = ms_vec_init(4);
                }
                
                ms_vec_push(&slot->ends, end);
                ms_vec_push(&slot->value_indices, nedge_pointsv);
                
                /* One of two neighbours */
                ms_vec_push(&slot->face_indices, face_index);
                ms_vec_push(&slot->face_indices, -1);
                
                struct ms_v3 edge_point;
                struct ms_v3 startv = mesh.vertices[start];
                struct ms_v3 endv = mesh.vertices[end];
                
                if (adj != face) {
                    struct ms_v3 face_avg = ms_math_avg(face_points[face], face_points[adj]);
                    struct ms_v3 edge_avg = ms_math_avg(startv, endv);
                    edge_point = ms_math_avg(face_avg, edge_avg);
                } else {
                    /* This is an edge of a hole */
                    edge_point = ms_math_avg(startv, endv);
                }
                
                edge_pointsv[nedge_pointsv] = edge_point;
                ++nedge_pointsv;
            }
        }
    }
    TracyCZoneEnd(edge_point_table);
    
    TracyCZoneN(edge_point_table_convert, "edge_points convert table", true);
    /* Convert lookup 'table' into an array */
    int *edge_points = malloc(mesh.nfaces * mesh.degree * sizeof(int));
    for (int start = 0; start < mesh.nverts; ++start) {
        struct ms_edgep *slot = edgep_lookup + start;
        
        for (int i = 0; i < slot->ends.len; ++i) {
            
            
            // all the misses here
            int first_index = slot->face_indices.data[i * 2 + 0];
            int second_index = slot->face_indices.data[i * 2 + 1];
            int value_index = slot->value_indices.data[i];
            edge_points[first_index] = value_index;
            /////////////////
            
            
            if (second_index != -1) {
                edge_points[second_index] = value_index;
            }
        }
    }
    TracyCZoneEnd(edge_point_table_convert);
#elif 0
    TracyCZoneN(collect_edges, "collect all edges", true);
    int nedges = 0;
    int *arr = malloc(mesh.nfaces * mesh.degree * 4 * sizeof(int));
    
    for (int face = 0; face < mesh.nfaces; ++face) {
        for (int vert = 0; vert < mesh.degree; ++vert) {
            int next_vert = (vert + 1) % mesh.degree;
            
            int edge = face * mesh.degree + vert;
            int start = mesh.faces[edge];
            int end = mesh.faces[face * mesh.degree + next_vert];
            
            if (start > end) { SWAP(start, end) }
            
            arr[nedges * 4 + 0] = start;
            arr[nedges * 4 + 1] = end;
            arr[nedges * 4 + 2] = edge;
            arr[nedges * 4 + 3] = face;
            
            ++nedges;
        }
    }
    
    TracyCZoneEnd(collect_edges);
    
    TracyCZoneN(sort_edges, "sort edges", true);
    qsort(arr, nedges, 4 * sizeof(int), compare_edges);
    TracyCZoneEnd(sort_edges);
    
    
    TracyCZoneN(find_unique_edges, "create unique edge points", true);
    struct ms_v3 *edge_pointsv = malloc(nedges * 2 * sizeof(struct ms_v3));
    int *edge_points = malloc(mesh.nfaces * mesh.degree * sizeof(int));
    int nedge_pointsv = 0;
    
    int prev_start = -1;
    int prev_end = -1;
    
    for (int e = 0; e < nedges; ++e) {
        int start = arr[e * 4 + 0];
        int end   = arr[e * 4 + 1];
        int edge  = arr[e * 4 + 2];
        int face  = arr[e * 4 + 3];
        
        if (prev_start != start || prev_end != end) {
            struct ms_v3 edge_point;
            struct ms_v3 startv = mesh.vertices[start];
            struct ms_v3 endv = mesh.vertices[end];
            
            int adj = edge_adjacent_face(&accel, mesh, face, start, end);
            
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
            edge_points[edge] = nedge_pointsv - 1;
        }
        
        prev_start = start;
        prev_end = end;
    }
    
    
    TracyCZoneEnd(find_unique_edges);
#else
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
    
    int *accum = calloc(1, mesh.nverts * sizeof(int));
    int *ends = malloc(nedges * sizeof(int));
    int *edges = malloc(nedges * sizeof(int));
    int *faces = malloc(nedges * sizeof(int));
    
    for (int v = 1; v < mesh.nverts + 1; ++v) {
        end_counts[v] += end_counts[v - 1];
    }
    
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
    
    struct ms_v3 *edge_pointsv = malloc(nedges * 2 * sizeof(struct ms_v3));
    int *edge_points = malloc(mesh.nfaces * mesh.degree * sizeof(int));
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
                }
            }
            
            if (found == -1) {
                struct ms_v3 edge_point;
                struct ms_v3 startv = mesh.vertices[start];
                struct ms_v3 endv = mesh.vertices[end];
                
                int adj = edge_adjacent_face(&accel, mesh, face, start, end);
                
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
#endif
    TracyCZoneEnd(compute_edge_points);
    
    /* Update points */
    TracyCZoneN(update_positions, "update old points", true);
    struct ms_v3 *new_verts = malloc(mesh.nverts * sizeof(struct ms_v3));
    
    for (int v = 0; v < mesh.nverts; ++v) {
        struct ms_v3 vertex = mesh.vertices[v];
        struct ms_v3 new_vert;
        
        struct ms_vec adj_faces = vert_adjacent_faces(&accel, mesh, v);
        struct ms_vec adj_verts = vert_adjacent_vertices(&accel, mesh, v);
        
        if (adj_faces.len != adj_verts.len) {
            /* This vertex is on an edge of a hole */
            int nedges_adj_to_hole = 0;
            struct ms_v3 avg_mid_edge_point = { 0 };
            
            for (int i = 0; i < adj_verts.len; ++i) {
                int start = v;
                int end = adj_verts.data[i];
                
                struct ms_v3 startv = mesh.vertices[start];
                struct ms_v3 endv = mesh.vertices[end];
                struct ms_v3 mid = ms_math_avg(startv, endv);
                
                /* Only take into account edges that are also on the edge of a hole */
                int adj_face = edge_adjacent_face(&accel, mesh, 0, start, end);
                int another_adj_face = edge_adjacent_face(&accel, mesh, adj_face, start, end);
                
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
            for (int i = 0; i < adj_verts.len; ++i) {
                int start = v;
                int end = adj_verts.data[i];
                
                struct ms_v3 startv = mesh.vertices[start];
                struct ms_v3 endv = mesh.vertices[end];
                struct ms_v3 mid = ms_math_avg(startv, endv);
                
                avg_mid_edge_point.x += mid.x;
                avg_mid_edge_point.y += mid.y;
                avg_mid_edge_point.z += mid.z;
            }
            
            f32 norm_coeff = 1.0f / adj_verts.len;
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
        
        free_results_vec(&adj_faces);
        free_results_vec(&adj_verts);
        
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
    
    free_hashtable(&accel);
    
    free(edge_pointsv);
    free(new_verts);
    free(face_points);
    free(edge_points);
    
#if 0
    for (int i = 0; i < mesh.nverts; ++i) {
        ms_vec_free(&edgep_lookup[i].ends);
        ms_vec_free(&edgep_lookup[i].face_indices);
        ms_vec_free(&edgep_lookup[i].value_indices);
    }
    free(edgep_lookup);
#endif
    
    
    TracyCZoneEnd(__FUNC__);
    
    return(new_mesh);
}