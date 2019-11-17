static struct ms_v3
ms_math_avg(struct ms_v3 a, struct ms_v3 b)
{
    struct ms_v3 result;
    
    result.x = (a.x + b.x) / 2.0f;
    result.y = (a.y + b.y) / 2.0f;
    result.z = (a.z + b.z) / 2.0f;
    
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
            { 1.0f, 0.0f, 0.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 1.0f, 0.0f },
            { x,    y,    z,    1.0f }
        }
    };
    
    return(result);
}

static struct ms_m4
ms_math_ortho(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far)
{
    struct ms_m4 result = {
        .data = {
            { 2.0f / (right - left), 0.0f,                  0.0f,                (right + left) / (left - right) },
            { 0.0f,                  2.0f / (top - bottom), 0.0f,                (top + bottom) / (bottom - top) },
            { 0.0f,                  0.0f,                  2.0f / (near - far), (far + near) / (near - far) },
            { 0.0f,                  0.0f,                  0.0f,                1.0f }
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

static struct ms_m4
ms_math_mm(struct ms_m4 A, struct ms_m4 B)
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