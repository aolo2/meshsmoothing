struct ms_edge {
    /* Computed during preprocessing */
    int from;
    int to;
    int face[2];
    int opposite;
    
    int index;
    int sorted_index;
    
    struct ms_v3 startv;
    struct ms_v3 endv;
    
    /* Computed during subdivision */
    struct ms_v3 edge_point;
    struct ms_v3 face_point;
    struct ms_v3 face_point_opposite;
};

struct ms_mesh_dhe {
    int nverts;
    int nfaces;
    int nedges;
    
    int *edges_from;
    
    struct ms_v3 *vertices;
    struct ms_edge *edges;
    struct ms_edge *sorted_edges;
};

static int
lexicographical_sort(const void *pa, const void *pb)
{
    struct ms_edge a = *(struct ms_edge *) pa;
    struct ms_edge b = *(struct ms_edge *) pb;
    
    if (a.from < b.from) {
        return(-1);
    } else if (a.from > b.from) {
        return(1);
    } else {
        if (a.to < b.to) {
            return(-1);
        } else if (a.to > b.to) {
            return(1);
        }
    }
    
    return(0);
}

static int
index_sort(const void *pa, const void *pb)
{
    const struct ms_edge *a = pa;
    const struct ms_edge *b = pb;
    
    if (a->index < b->index) {
        return(-1);
    } else if (a->index > b->index) {
        return(1);
    }
    
    return(0);
}

static struct ms_mesh_dhe
ms_convert_to_dhe(struct ms_mesh mesh)
{
    TracyCZone(__FUNC__, true);
    
    struct ms_mesh_dhe result = { 0 };
    
    result.nedges = mesh.faces_from[mesh.nfaces];
    result.nverts = mesh.nverts;
    result.nfaces = mesh.nfaces;
    result.edges_from = mesh.faces_from;
    result.vertices = mesh.vertices;
    result.edges = malloc(result.nedges * sizeof(struct ms_edge));
    result.sorted_edges = malloc(result.nedges * sizeof(struct ms_edge));
    
    TracyCZoneN(unsorted_edges, "unsorted", true);
    
    for (int f = 0; f < mesh.nfaces; ++f) {
        int from = mesh.faces_from[f];
        int to = mesh.faces_from[f + 1];
        int degree = to - from;
        
        for (int e = from; e < to; ++e) {
            int start = mesh.faces[e];
            int end = mesh.faces[(e - from == degree - 1 ? from : e + 1)];
            
            struct ms_edge edge = { 0 };
            
            if (start < end) {
                edge.from = start;
                edge.to = end;
            } else {
                edge.from = end;
                edge.to = start;
            }
            
            edge.index = e;
            edge.face[0] = f;
            edge.startv = mesh.vertices[start];
            edge.endv = mesh.vertices[end];
            
            result.edges[e] = edge;
        }
    }
    
    TracyCZoneEnd(unsorted_edges);
    
    TracyCZoneN(copy_edges, "memcpy", true);
    memcpy(result.sorted_edges, result.edges, result.nedges * sizeof(struct ms_edge));
    TracyCZoneEnd(copy_edges);
    
    TracyCZoneN(sort_edges, "qsort", true);
    qsort(result.sorted_edges, result.nedges, sizeof(struct ms_edge), lexicographical_sort);
    TracyCZoneEnd(sort_edges);
    
    TracyCZoneN(propogate_info, "propogate", true);
    for (int i = 0; i < result.nedges - 1; ++i) {
        struct ms_edge *e1 = result.sorted_edges + i;
        struct ms_edge *e2 = result.sorted_edges + i + 1;
        
        if (e1->from == e2->from && e1->to == e2->to) {
            e1->opposite = e2->index;
            e2->opposite = e1->index;
            
            e1->face[1] = e2->face[0];
            e2->face[1] = e1->face[0];
        }
    }
    
    /* Sorted edges have all the adjacency info, overwrite regular edges */
#if 0
    memcpy(result.edges, result.sorted, result.nedges * sizeof(struct ms_edge));
    qsort(result.sorted_edges, result.nedges, sizeof(struct ms_edge), index_sort);
#else
    for (int i = 0; i < result.nedges; ++i) {
        result.sorted_edges[i].sorted_index = i;
        struct ms_edge edge = result.sorted_edges[i];
        result.edges[edge.index] = edge;
    }
#endif
    
    TracyCZoneEnd(propogate_info);
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}

static struct ms_v3 *
ms_subdivide_new(struct ms_mesh_dhe mesh)
{
    TracyCZone(__FUNC__, true);
    
    struct ms_v3 *new_verts = cacheline_alloc(mesh.nverts * sizeof(struct ms_v3));
    
    TracyCZoneN(face_points, "facepoints", true);
    
    /* Face points */
    for (int face = 0; face < mesh.nfaces; ++face) {
        int e_start = mesh.edges_from[face];
        int e_end = mesh.edges_from[face + 1];
        
        struct ms_v3 face_point = { 0 };
        
        for (int e = e_start; e < e_end; ++e) {
            struct ms_edge edge = mesh.edges[e];
            struct ms_v3 vertex = edge.startv;
            
            face_point.x += vertex.x;
            face_point.y += vertex.y;
            face_point.z += vertex.z;
        }
        
        f32 factor = 1.0f / (e_end - e_start);
        
        face_point.x *= factor;
        face_point.y *= factor;
        face_point.z *= factor;
        
        for (int e = e_start; e < e_end; ++e) {
            struct ms_edge edge = mesh.edges[e];
            
            /* DC miss FESTIVAL! */
            
            mesh.edges[e].face_point = face_point;
            mesh.edges[edge.opposite].face_point_opposite = face_point;
            
            mesh.sorted_edges[edge.sorted_index].face_point = face_point;
            mesh.sorted_edges[mesh.edges[edge.opposite].sorted_index].face_point_opposite = face_point;
        }
    }
    TracyCZoneEnd(face_points);
    
    TracyCZoneN(edge_points, "edgepoints", true);
    
    /* Edge points */
    for (int e = 0; e < mesh.nedges; ++e) {
        struct ms_edge edge = mesh.sorted_edges[e];
        int edge_face = edge.face[0];
        int adj_face = edge.face[1];
        
        struct ms_v3 edge_point;
        
        if (edge_face != adj_face) {
            edge_point.x = (edge.face_point.x + edge.face_point_opposite.x + edge.startv.x + edge.endv.x) * 0.25f;
            edge_point.y = (edge.face_point.y + edge.face_point_opposite.y + edge.startv.y + edge.endv.y) * 0.25f;
            edge_point.z = (edge.face_point.z + edge.face_point_opposite.z + edge.startv.z + edge.endv.z) * 0.25f;
        } else {
            edge_point.x = (edge.startv.x + edge.endv.x) * 0.5f;
            edge_point.y = (edge.startv.y + edge.endv.y) * 0.5f;
            edge_point.z = (edge.startv.z + edge.endv.z) * 0.5f;
        }
        
        mesh.sorted_edges[e].edge_point = edge_point;
    }
    
    TracyCZoneEnd(edge_points);
    
    TracyCZoneN(update_positions, "updatepositions", true);
#if 0
    /* Update points */
    int e = 0;
    
    for (int v = 0; v < mesh.nverts; ++v) {
        /* Average of face points of all the faces this vertex is adjacent to */
        struct ms_v3 avg_face_point = { 0 };
        
        /* Average of mid points of all the edges this vertex is adjacent to */
        struct ms_v3 avg_mid_edge_point = { 0 };
        struct ms_v3 vertex = mesh.vertices[v];
        
        int adj_faces_count = 0;
        
        while (e < mesh.nedges && mesh.sorted_edges[e].from == v) {
            avg_face_point.x += mesh.sorted_edges[e].face_point.x;
            avg_face_point.y += mesh.sorted_edges[e].face_point.y;
            avg_face_point.z += mesh.sorted_edges[e].face_point.z;
            
            avg_mid_edge_point.x += (mesh.sorted_edges[e].startv.x + mesh.sorted_edges[e].endv.x) * 0.5f;
            avg_mid_edge_point.y += (mesh.sorted_edges[e].startv.y + mesh.sorted_edges[e].endv.y) * 0.5f;
            avg_mid_edge_point.z += (mesh.sorted_edges[e].startv.z + mesh.sorted_edges[e].endv.z) * 0.5f;
            
            ++adj_faces_count;
            ++e;
        }
        
        /* Weights */
        f32 w1 = (f32) (adj_faces_count - 3) / adj_faces_count;
        f32 w2 = 1.0f / adj_faces_count;
        f32 w3 = 2.0f * w2;
        
        struct ms_v3 new_vert;
        
        /* Weighted average to obtain a new vertex */
        new_vert.x = w1 * vertex.x + w2 * avg_face_point.x + w3 * avg_mid_edge_point.x;
        new_vert.y = w1 * vertex.y + w2 * avg_face_point.y + w3 * avg_mid_edge_point.y;
        new_vert.z = w1 * vertex.z + w2 * avg_face_point.z + w3 * avg_mid_edge_point.z;
        
        new_verts[v] = new_vert;
    }
#endif
    
    TracyCZoneEnd(update_positions);
    
    TracyCZoneN(subdivide, "subdivide", true);
    
    struct ms_v3 *result = NULL;
    
    /* Subdivide */
    for (int face = 0; face < mesh.nfaces; ++face) {
        int e_start = mesh.edges_from[face];
        int e_end = mesh.edges_from[face + 1];
        
        struct ms_v3 face_point = mesh.edges[e_start].face_point;
        
        for (int e = e_start; e < e_end; ++e) {
            int next = (e + 1 == e_end ? e_start : e + 1);
            struct ms_edge edge = mesh.edges[e];
            struct ms_edge next_edge = mesh.edges[next];
            
            struct ms_v3 vertex = new_verts[edge.from];
            struct ms_v3 edge_point = edge.edge_point;
            struct ms_v3 edge_point_next = next_edge.edge_point;
            
            sb_push(result, vertex);
            sb_push(result, edge_point_next);
            sb_push(result, face_point);
            sb_push(result, edge_point);
        }
    }
    
    TracyCZoneEnd(subdivide);
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}