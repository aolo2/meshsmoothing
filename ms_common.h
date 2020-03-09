#include <stdint.h>  /* for fixed-size types */
#include <stdlib.h>  /* for malloc/free */
#include <stdio.h>   /* for fopen/fread/fclose */
#include <stdbool.h> /* for bool, true, false */
#include <string.h>  /* for memset */
#include <assert.h>  /* for assert */
#include <math.h>    /* for sinf, cosf, tanf */
#include <ctype.h>   /* for isspace, isalpha etc */
#include <time.h>    /* for clock_gettime */

#include "external/tracy/TracyC.h"

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

static const f32 ERR = 1e-5f;
static const f32 PI = 3.1415926f;

struct ms_vec {
    int *data;
    int cap;
    int len;
};

struct ms_v3 {
    f32 x;
    f32 y;
    f32 z;
};

struct ms_edgep {
    struct ms_vec ends;
    struct ms_vec face_indices;
    struct ms_vec value_indices;
};

struct ms_mesh {
    struct ms_v3 *vertices;
    int *faces;
    
    int degree;
    int nverts;
    int nfaces;
};
