#include <stdint.h>  /* for fixed-size types */
#include <stdlib.h>  /* for malloc/free */
#include <stdio.h>   /* for fopen/fread/fclose */
#include <stdbool.h> /* for bool, true, false */
#include <string.h>  /* for memset */
#include <assert.h>  /* for assert */
#include <math.h>    /* for sinf, cosf, tanf */

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

static const f32 ERR = 1e-3f;

struct ms_gl_bufs {
    u32 VAO;
    u32 VBO;
};

struct ms_m4 {
    f32 data[4][4];
};

#pragma pack(push, 4)
struct ms_v3 {
    f32 x;
    f32 y;
    f32 z;
};
#pragma pack(pop)

struct ms_mesh {
    u8 degree;
    u32 primitives;
    struct ms_v3 *vertices;
    struct ms_v3 *normals;
};