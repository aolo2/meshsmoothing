#include "ms_common.h"

#include "ms_time.c"
#include "ms_math.c"
#include "ms_file_new.c"
#include "ms_subdiv_csr.c"
#include "ms_subdiv_new.c"

s32
main(s32 argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "[ERROR] Usage: %s path/to/model.obj ITERATIONS\n", argv[0]);
        return(1);
    }
    
    struct ms_mesh mesh = ms_file_obj_read_file_new(argv[1]);
    int iterations = atoi(argv[2]);
    char output_filename[512] = { 0 };
    
    bool fortex = false;
    
    if (fortex) {
        printf("\\addplot[color=red,mark=x] coordinates {\n");
    }
    
    for (int i = 0; i < iterations; ++i) {
        
        int size = mesh.nfaces * 4;
        
        u64 before = cycles_now();
        struct ms_mesh new_mesh = ms_subdiv_catmull_clark_new(mesh);
        u64 after = cycles_now();
        
        free(mesh.vertices);
        free(mesh.faces);
        
        mesh = new_mesh;
        
        u64 total = after - before;
        
        if (fortex) {
            printf("\t(%d, %f)\n", size, total / 1000.0f);
        } else {
            printf("[INFO] Finished Catmull-Clark [%d]\n", i + 1);
            printf("[TIME] %d vertices, %lu cycles, %f cycles/v\n", size, total, total / (f32) size);
        }
        
        int len = snprintf(output_filename, 512, "%s_%d.obj", argv[1], i + 1);
        output_filename[len] = 0;
        //ms_file_obj_write_file_new(output_filename, mesh);
    }
    
    if (fortex) {
        printf("};\n");
    }
    
    free(mesh.vertices);
    free(mesh.faces);
    
    return(0);
}
