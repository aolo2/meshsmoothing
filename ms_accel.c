static struct ms_accel
init_old_acceleration_struct(struct ms_mesh mesh)
{
    TracyCZone(__FUNC__, true);
    
    int *offsets = calloc64((mesh.nverts + 1) * sizeof(int));
    
    /* Count edges */
    for (int face = 0; face < mesh.nfaces * 4; face += 4) {
        int v1 = mesh.faces[face + 0];
        int v2 = mesh.faces[face + 1];
        int v3 = mesh.faces[face + 2];
        int v4 = mesh.faces[face + 3];
        
        offsets[v1 + 1]++;
        offsets[v2 + 1]++;
        offsets[v3 + 1]++;
        offsets[v4 + 1]++;
        
        /* Face doesnt have repeated vertices. This guarantees that edge->face map wont have duplicates.
 Moreover, face counts match edge counts! */
    }
    
    /* Propogate offsets */
    for (int v = 1; v < mesh.nverts + 1; ++v) {
        offsets[v] += offsets[v - 1];
    }
    
    int nedges = offsets[mesh.nverts];
    int *edges = malloc64(nedges * sizeof(int));
    int *faces = malloc64(nedges * sizeof(int));
    
    int *edges_accum = calloc64(nedges * sizeof(int));
    
    for (int face = 0; face < mesh.nfaces * 4; face += 4) {
        int v1 = mesh.faces[face + 0];
        int v2 = mesh.faces[face + 1];
        int v3 = mesh.faces[face + 2];
        int v4 = mesh.faces[face + 3];
        
        int actual_face = face / 4;
        
        /*
v1 ----- v2 
           |    0    | 
         |3        | 
         |        1| 
         |    2    | 
         v4 ----- v3
*/
        edges[offsets[v1] + edges_accum[v1]] = v2;
        edges[offsets[v2] + edges_accum[v2]] = v3;
        edges[offsets[v3] + edges_accum[v3]] = v4;
        edges[offsets[v4] + edges_accum[v4]] = v1;
        
        faces[offsets[v1] + edges_accum[v1]] = actual_face;
        faces[offsets[v2] + edges_accum[v2]] = actual_face;
        faces[offsets[v3] + edges_accum[v3]] = actual_face;
        faces[offsets[v4] + edges_accum[v4]] = actual_face;
        
        edges_accum[v1]++;
        edges_accum[v2]++;
        edges_accum[v3]++;
        edges_accum[v4]++;
    }
    
    free(edges_accum);
    
    struct ms_accel result;
    
    result.verts_starts = offsets;
    result.verts_matrix = edges;
    result.faces_matrix = faces;
    
    TracyCZoneEnd(__FUNC__);
    
    return(result);
}