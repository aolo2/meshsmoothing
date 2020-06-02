#include "ms_common.h"

#include "ms_system.c"

#ifdef MT
#    ifndef NTHREADS
#        define NTHREADS 6
#    endif
#    include <omp.h>
#    include "ms_subdiv_csr_mt.c"
#    include "ms_subdiv_mt.c"
#else
#    include "ms_subdiv_csr.c"
#    include "ms_subdiv.c"
#endif

int
main(int argc, char *argv[])
{
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "[ERROR] Usage: %s path/to/model.obj ITERATIONS [BENCH_IRERATIONS]\n", argv[0]);
        return(1);
    }
    
    struct ms_mesh mesh = ms_file_obj_read_fast(argv[1]);
    int iterations = atoi(argv[2]);
    int bench_itearitons = -1;
    char output_filename[512] = { 0 };
    
    bool fortex = false;
    bool writefile = true;
    
    printf("[INFO] Write to file: %s\n       Generate LaTeX: %s\n",
           (writefile ? "ENABLED" : "DISABLED"), (fortex ? "ENABLED" : "DISABLED"));
    
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
        printf("[INFO] Running in REGULAR mode with %d iterations\n", iterations);
        u64 usec_before = usec_now();
        for (int i = 0; i < iterations; ++i) {
            int size = mesh.nfaces * 4;
            
            u64 before = cycles_now();
            struct ms_mesh new_mesh = ms_subdiv_catmull_clark_new(mesh);
            u64 after = cycles_now();
            
            free(mesh.vertices);
            free(mesh.faces);
            
            TracyCFree(mesh.vertices);
            TracyCFree(mesh.faces);
            
            mesh = new_mesh;
            
            u64 total = after - before;
            
            if (fortex) {
                printf("\t(%d, %f)\n", size, total / 1000.0f);
            } else {
                printf("[INFO] Finished Catmull-Clark [%d]\n", i + 1);
                printf("[TIME] New mesh: %d vertices, %lu cycles, %f cycles/v\n", size, total, total / (f32) size);
            }
            
            int len = snprintf(output_filename, 512, "%s_%d.obj", argv[1], i + 1);
            output_filename[len] = 0;
            
            if (writefile) {
                ms_file_obj_write_file(output_filename, mesh);
            }
        }
        
        u64 usec_after = usec_now();
        printf("[TIME] Total time elapsed %.f ms\n", (usec_after - usec_before) / 1000.0f);
    } else {
        printf("[INFO] Running in BENCH mode with %d repeats\n", bench_itearitons);
        unsigned long long total_total_cycles = 0ULL;
        f32 iteration_cycles[MAX_BENCH_ITERATIONS];
        
        u64 usec_before = usec_now();
        for (int i = 0; i < bench_itearitons; ++i) {
            printf("\r[BENCH] %d/%d", i + 1, bench_itearitons);
            fflush(stdout);
            
            u64 before = cycles_now();
            struct ms_mesh new_mesh = ms_subdiv_catmull_clark_new(mesh);
            u64 after = cycles_now();
            
            free(new_mesh.vertices);
            free(new_mesh.faces);
            
            TracyCFree(new_mesh.vertices);
            TracyCFree(new_mesh.faces);
            
            iteration_cycles[i] = (f32) (after - before) / (mesh.nfaces * 4);
            total_total_cycles += (f32) (after - before) / (mesh.nfaces * 4);
        }
        
        u64 usec_after = usec_now();
        
        printf("\n[BENCH] Done\n");
        
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
        
        printf("[TIME] avg: %.1f cycles/v | sigma: %.2f\n", avg, stdev);
        printf("[TIME] avg: %.1f ms per subdivision of %d vertex mesh\n", (usec_after - usec_before) / 1000.0f / bench_itearitons, mesh.nverts);
    }
    
    if (fortex) {
        printf("};\n");
    }
    
    free(mesh.vertices);
    free(mesh.faces);
    
    TracyCFree(mesh.vertices);
    TracyCFree(mesh.faces);
    
    return(0);
}
