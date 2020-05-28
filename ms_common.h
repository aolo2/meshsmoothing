#include <stdint.h>  /* for fixed-size types */
#include <stdlib.h>  /* for malloc/free */
#include <stdio.h>   /* for fopen/fread/fclose */
#include <stdbool.h> /* for bool, true, false */
#include <string.h>  /* for memset */
#include <assert.h>  /* for assert */
#include <ctype.h>   /* for isspace, isalpha etc */
#include <time.h>    /* for clock_gettime */
#include <math.h>    /* for sqrtf */

#ifdef PROFILE
#    include "external/tracy/TracyC.h"
#    ifndef NOSTACK2
#        define CALLSTACK_DEPTH 3
#    else
#        undef TracyCZoneS
#        undef TracyCZoneNS
#        undef TracyCAllocS
#        define TracyCZoneS(a, b, c) TracyCZone(a, b)
#        define TracyCZoneNS(a, b, c, d) TracyCZoneN(a, b, c)
#    endif
#else
#    define TracyCZone(...)
#    define TracyCZoneN(...)
#    define TracyCZoneEnd(...)
#    define TracyCAlloc(...)
#    define TracyCFree(...)
#    define TracyCZoneS(...)
#    define TracyCZoneNS(...)
#    define TracyCAllocS(...)
#    define TracyCFreeS(...)
#    define TracyCMessage(...)
#endif

#define MAX_BENCH_ITERATIONS 100
#define SWAP(a, b) { typeof(a) ___tmp___ = (a); (a) = (b); (b) = ___tmp___; }

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

struct ms_v2i {
    int a;
    int b;
};

struct ms_v3i {
    s32 a;
    s32 b;
    s32 c;
};

struct ms_v3 {
    f32 x;
    f32 y;
    f32 z;
};

struct ms_mesh {
    struct ms_v3 *vertices;
    int *faces;
    int degree;
    int nverts;
    int nfaces;
};

/* 40 bytes */
struct ms_edge {
    struct ms_v3 endv;
    struct ms_v3 face_point_1;
    struct ms_v3 face_point_2;
    int edge_index_1;
    int edge_index_2;
    int end;
};

struct ms_average {
    u32 face_count;
    struct ms_v3 point;
};

struct ms_accel {
    int *verts_starts;
    int *verts_matrix;
    int *face_counts;
    
    struct ms_edge *pack;
    struct ms_v3 *fp_averages;
};
