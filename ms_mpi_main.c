#include <mpi.h>

#define MASTER 0

#include "ms_common.h"

#include "ms_partition.c"
#include "ms_system.c"
#include "ms_subdiv_csr.c"
#include "ms_subdiv.c"


int
main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm comm = MPI_COMM_WORLD;
    
    int rank;
    int size;
    
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);
    
    struct ms_mesh mesh = { 0 };
    int iterations = 0;
    
    if (rank == 0) {
        if (argc != 3) {
            fprintf(stderr, "[ERROR] Usage: %s path/to/model.obj ITERATIONS\n", argv[0]);
            MPI_Abort(comm, 1);
        }
        
        printf("[INFO] Running with %d processes\n", size);
        
        mesh = ms_file_obj_read_fast(argv[1]);
        iterations = atoi(argv[2]);
        
        printf("[INFO] Loaded OBJ file with\n"
               "\t%d unique vertices\n"
               "\t%d quads"
               "\n\t%d face vertices\n", mesh.nverts, mesh.nfaces, mesh.nfaces * 4);
    }
    
    iterations = atoi(argv[2]);
    
    /* Each process needs [iterations + 1] (????) halo levels to independently subdivide for [iteration] steps */
    distribute_mesh_with_overlap(comm, rank, size, &mesh);
    
#if 0
    if (mesh.nfaces > 0) {
        for (int i = 0; i < iterations; ++i) {
            struct ms_mesh new_mesh = ms_subdiv_catmull_clark_new(&mesh);
            
            free(mesh.vertices_x);
            free(mesh.vertices_y);
            free(mesh.vertices_z);
            free(mesh.faces);
            
            mesh = new_mesh;
        }
    }
    
    stitch_back_mesh(comm, rank, size, &mesh);
    
    char output_filename[512] = { 0 };
    int len = snprintf(output_filename, 512, "%s_%d.obj", argv[1], iterations);
    output_filename[len] = 0;
    ms_file_obj_write_file(output_filename, mesh);
#endif
    
    free(mesh.vertices_x);
    free(mesh.vertices_y);
    free(mesh.vertices_z);
    free(mesh.faces);
    
    int rt = MPI_Finalize();
    return(rt);
}
