#include <stdint.h>  /* for fixed-size types */
#include <stdlib.h>  /* for malloc/free */
#include <stdio.h>   /* for fopen/fread/fclose */
#include <stdbool.h> /* for bool, true, false */
#include <string.h>  /* for memset */
#include <assert.h>  /* for assert */
#include <ctype.h>   /* for isspace, isalpha etc */
#include <time.h>    /* for clock_gettime */

#ifdef PROFILE
#include "external/tracy/TracyC.h"
#else
#define TracyCZone(a, b)
#define TracyCZoneN(a, b, c)
#define TracyCZoneEnd(a)
#endif

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

struct ms_accel {
    int *faces_starts;
    int *verts_starts;
    
    int *edge_indices;
    int *edge_faces;
    
    int *faces_matrix;
    int *verts_matrix;
    
    int *verts_starts_repeats;
    int *verts_matrix_repeats;
};
