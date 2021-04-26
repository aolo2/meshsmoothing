#include "ms_common.h"

#include "ms_partition.c"
#include "ms_system.c"
#include "ms_subdiv_csr.c"
#include "ms_subdiv_mpi.c"

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
    
    if (rank == MASTER) {
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
    
    distribute_mesh_with_overlap(comm, rank, size, &mesh);
    
    if (mesh.nfaces > 0) {
        for (int i = 0; i < iterations; ++i) {
            struct ms_mesh new_mesh = ms_subdiv_catmull_clark_tagged(&mesh);
            
            free(mesh.vertices_x);
            free(mesh.vertices_y);
            free(mesh.vertices_z);
            free(mesh.faces);
            
            mesh = new_mesh;
        }
    }
    
#if 1
    char output_filename_1[512] = { 0 };
    int len = snprintf(output_filename_1, 512, "%s_%d_PIECE_%d.obj", argv[1], iterations, rank);
    output_filename_1[len] = 0;
    ms_file_obj_write_file(output_filename_1, mesh);
#endif
    
    stitch_back_mesh(comm, rank, size, &mesh);
    
    if (rank == MASTER) {
        char output_filename[512] = { 0 };
        int len = snprintf(output_filename, 512, "%s_%d_MASTER.obj", argv[1], iterations);
        output_filename[len] = 0;
        ms_file_obj_write_file(output_filename, mesh);
    }
    
    
    free(mesh.vertices_x);
    free(mesh.vertices_y);
    free(mesh.vertices_z);
    free(mesh.faces);
    
    int rt = MPI_Finalize();
    return(rt);
}
