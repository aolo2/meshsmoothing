#include <stdint.h>     /* for fixed-size types */
#include <stdlib.h>     /* for malloc/free */
#include <stdio.h>      /* for fopen/fread/fclose */
#include <stdbool.h>    /* for bool, true, false */
#include <string.h>     /* for memset */
#include <assert.h>     /* for assert */
#include <ctype.h>      /* for isspace, isalpha etc */
#include <time.h>       /* for clock_gettime */
#include <immintrin.h>  /* for SIMD */

#ifdef MT
#include <omp.h>
#endif

#ifdef MP
#include <mpi.h>
#endif

#ifdef PROFILE
#    include "external/tracy/TracyC.h"
#else
#    define TracyCZone(...)
#    define TracyCZoneEnd(...)
#    define TracyCZoneN(...)
#    define TracyCAlloc(...)
#    define TracyCFree(...)
#endif

#define MAX_BENCH_ITERATIONS 1000
#define SWAP(a, b) { typeof(a) tmp___ = (a); (a) = (b); (b) = tmp___; }
#define MASTER 0

#ifndef MIN 
#    define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef float  f32;
typedef double f64;

struct ms_buffer {
    void *data;
    u64 size;
};

#pragma pack(push, 1)
struct ms_edge {
    int face_1;
    int face_2;
    int start;
    int end;
    u8 findex_1;
    u8 findex_2;
};
#pragma pack(pop)

struct ms_mesh {
    f32 *vertices_x;
    f32 *vertices_y;
    f32 *vertices_z;
    int *faces;
    int nverts;
    int nfaces;
};

struct ms_vertex {
    f32 fpx, fpy, fpz;
    f32 mex, mey, mez;
    f32 smex, smey, smez;
    int nfaces;
    int nedges;
};

struct ms_edges {
    struct ms_edge *edges;
    int count;
};