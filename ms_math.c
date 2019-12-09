static struct ms_v3
ms_math_avg(struct ms_v3 a, struct ms_v3 b)
{
    struct ms_v3 result;
    
    result.x = (a.x + b.x) / 2.0f;
    result.y = (a.y + b.y) / 2.0f;
    result.z = (a.z + b.z) / 2.0f;
    
    return(result);
}

static f32
ms_math_dot(struct ms_v3 a, struct ms_v3 b)
{
    f32 result = a.x * b.x + a.y * b.y + a.z * b.z;
    return(result);
}

/* Alias  */
static f32
ms_math_inner(struct ms_v3 a, struct ms_v3 b)
{
    return(ms_math_dot(a, b));
}

static f32
ms_math_len(struct ms_v3 vec)
{
    f32 result = sqrtf(vec.x * vec.x * vec.y * vec.y + vec.z * vec.z);
    return(result);
}

static struct ms_v3
ms_math_normalize(struct ms_v3 vec)
{
    f32 len = ms_math_len(vec);
    f32 fact = 1.0f / len;
    
    struct ms_v3 result = vec;
    
    result.x *= fact;
    result.y *= fact;
    result.z *= fact;
    
    return(result);
}

static struct ms_v3
ms_math_navg(struct ms_v3 *verts, u32 count)
{
    struct ms_v3 result = { 0 };
    
    for (u32 i = 0; i < count; ++i) {
        result.x += verts[i].x;
        result.y += verts[i].y;
        result.z += verts[i].z;
    }
    
    result.x /= (f32) count;
    result.y /= (f32) count;
    result.z /= (f32) count;
    
    return(result);
}

static struct ms_m4
ms_math_unitm4()
{
    struct ms_m4 result = {
        .data = {
            { 1.0f, 0.0f, 0.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 1.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f }
        }
    };
    
    return(result);
}

static struct ms_m4
ms_math_scale(f32 scale)
{
    struct ms_m4 result = {
        .data = {
            { scale, 0.0f,  0.0f,  0.0f },
            { 0.0f,  scale, 0.0f,  0.0f },
            { 0.0f,  0.0f,  scale, 0.0f },
            { 0.0f,  0.0f,  0.0f,  1.0f }
        }
    };
    return(result);
}

static struct ms_m4
ms_math_translate(f32 x, f32 y, f32 z)
{
    struct ms_m4 result = {
        .data = {
            { 1.0f, 0.0f, 0.0f, x },
            { 0.0f, 1.0f, 0.0f, y },
            { 0.0f, 0.0f, 1.0f, z },
            { 0.0f, 0.0f, 0.0f, 1.0f }
        }
    };
    
    return(result);
}

static struct ms_m4
ms_math_perspective(f32 aspect, f32 fov_deg, f32 p_near, f32 p_far)
{
    f32 fov_rad = fov_deg / 180.0f * M_PI;
    f32 tan_fov_2 = tanf(fov_rad / 2.0f);
    
    struct ms_m4 result = {
        .data = {
            { 1.0f / (aspect * tan_fov_2), 0.0f,             0.0f,                                       0.0f                                       },
            { 0.0f,                        1.0f / tan_fov_2, 0.0f,                                       0.0f                                       },
            { 0.0f,                        0.0f,             -(p_far + p_near) / (p_far - p_near),       (2.0f * p_far * p_near) / (p_near - p_far) },
            { 0.0f,                        0.0f,             -1.0f,                                      0.0f                                       }
        }
    };
    
    return(result);
}

static struct ms_m4
ms_math_rot(f32 x, f32 y, f32 z, f32 angle_rad)
{
    f32 cos_t = cosf(angle_rad);
    f32 sin_t = sinf(angle_rad);
    
    struct ms_m4 result = {
        .data = {
            { cos_t + x * x * (1.0f - cos_t),     x * y * (1.0f - cos_t) - z * sin_t, x * z * (1.0f - cos_t) + y * sin_t, 0.0f },
            { y * x * (1.0f - cos_t) + z * sin_t, cos_t + y * y * (1.0f - cos_t),     y * z * (1 - cos_t) - x * sin_t,    0.0f },
            { z * x * (1.0f - cos_t) - y * sin_t, z * y * (1.0f - cos_t) + x * sin_t, cos_t + z * z * (1.0f - cos_t),     0.0f },
            { 0.0f,                               0.0f,                               0.0f,                               1.0f }
        }
    };
    
    return(result);
}

static struct ms_v3
ms_math_mv(struct ms_m4 M, struct ms_v3 v)
{
    struct ms_v3 result;
    
    result.x = M.data[0][0] * v.x + M.data[0][1] * v.y + M.data[0][2] * v.z + M.data[0][3] * 1.0f;
    result.y = M.data[1][0] * v.x + M.data[1][1] * v.y + M.data[1][2] * v.z + M.data[1][3] * 1.0f;
    result.z = M.data[2][0] * v.x + M.data[2][1] * v.y + M.data[2][2] * v.z + M.data[2][3] * 1.0f;
    
    f32 w = M.data[3][0] * v.x + M.data[3][1] * v.y + M.data[3][2] * v.z + M.data[3][3] * 1.0f;
    
    result.x /= w;
    result.y /= w;
    result.z /= w;
    
    return(result);
}

static struct ms_m4
ms_math_mm(struct ms_m4 B, struct ms_m4 A)
{
    struct ms_m4 result = { 0 };
    
    for (u32 y = 0; y < 4; ++y) {
        for (u32 x = 0; x < 4; ++x) {
            for (u32 i = 0; i < 4; ++i) {
                result.data[y][x] += A.data[y][i] * B.data[i][x];
            }
        }
    }
    
    return(result);
}

static struct ms_m4
ms_math_mm3(struct ms_m4 A, struct ms_m4 B, struct ms_m4 C)
{
    struct ms_m4 result;
    
    result = ms_math_mm(A, ms_math_mm(B, C));
    
    return(result);
}