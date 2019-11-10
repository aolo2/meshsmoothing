#include <stdint.h> /* for fixed-size types */
#include <stdlib.h> /* for malloc/free */
#include <stdio.h>  /* for fopen/fread/fclose */
#include <assert.h> /* for assert */
#include <string.h> /* for memset */
#include <math.h>   /* for sinf, cosf, tanf */

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

struct ms_mesh {
    u32 triangles;
    f32 *vertices;
    f32 *normals;
};

struct ms_v3 {
    f32 data[3];
};

struct ms_m4 {
    f32 data[4][4];
};