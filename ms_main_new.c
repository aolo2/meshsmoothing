#define MAX_BENCH_ITERATIONS 100
#include <math.h>


#include "ms_common.h"

#include "ms_time.c"
#include "ms_math.c"
#include "ms_file_new.c"
#include "ms_subdiv_csr.c"
#include "ms_subdiv_new.c"

s32
main(s32 argc, char *argv[])
{
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "[ERROR] Usage: %s path/to/model.obj ITERATIONS [BENCH_IRERATIONS]\n", argv[0]);
        return(1);
    }
    
    struct ms_mesh mesh = ms_file_obj_read_file_new(argv[1]);
    int iterations = atoi(argv[2]);
    int bench_itearitons = -1;
    char output_filename[512] = { 0 };
    
    bool fortex = false;
    bool writefile = false;
    
    if (argc == 4) {
        bench_itearitons = atoi(argv[3]);
        if (bench_itearitons > MAX_BENCH_ITERATIONS) {
            fprintf(stderr, "[ERROR] Maximum amount of allowed bench iterations is %d\n", MAX_BENCH_ITERATIONS);
            return(1);
        }
        writefile = false;
    }
    
    if (fortex) {
        printf("\\addplot[color=red,mark=x] coordinates {\n");
    }
    
    if (bench_itearitons == -1) {
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
            
            if (writefile) {
                ms_file_obj_write_file_new(output_filename, mesh);
            }
        }
    } else {
        unsigned long long total_total_cycles = 0ULL;
        f32 iteration_cycles[MAX_BENCH_ITERATIONS];
        
        for (int i = 0; i < bench_itearitons; ++i) {
            u64 before = cycles_now();
            struct ms_mesh new_mesh = ms_subdiv_catmull_clark_new(mesh);
            u64 after = cycles_now();
            
            free(new_mesh.vertices);
            free(new_mesh.faces);
            
            iteration_cycles[i] = (f32) (after - before) / (mesh.nfaces * 4);
            total_total_cycles += (f32) (after - before) / (mesh.nfaces * 4);
        }
        
        f32 avg = (f32) total_total_cycles / bench_itearitons;
        f32 stdev = 0.0f;
        
        for (int i = 0; i < bench_itearitons; ++i) {
            f32 dev = (f32) iteration_cycles[i] - avg;
            stdev += dev * dev;
        }
        
        if (bench_itearitons > 1) {
            stdev /= (bench_itearitons - 1);
        }
        
        stdev = sqrtf(stdev);
        
        printf("[TIME] Average: %f cycles/v, Stdeviation: %f\n", avg, stdev);
    }
    
    if (fortex) {
        printf("};\n");
    }
    
    free(mesh.vertices);
    free(mesh.faces);
    
    return(0);
}
