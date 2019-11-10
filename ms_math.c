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
ms_math_projection(f32 aspect, f32 fov_deg, f32 p_near, f32 p_far)
{
    f32 tan_fov_2 = tanf(fov_deg / 2.0f);
    
    struct ms_m4 result = {
        .data = {
            { 1.0f / (aspect * tan_fov_2), 0.0f,             0.0f,                                0.0f },
            { 0.0f,                        1.0f / tan_fov_2, 0.0f,                                0.0f },
            { 0.0f,                        0.0f,             (p_far + p_near) / (p_near - p_far), (2.0f * p_far * p_near) / (p_near - p_far) },
            { 0.0f,                        0.0f,            -1.0f,                                1.0f }
        }
    };
    
    return(result);
}


static struct ms_m4
ms_math_rot(struct ms_v3 unit_axis, f32 angle_rad)
{
    f32 cos_t = cosf(angle_rad);
    f32 sin_t = sinf(angle_rad);
    
    f32 x = unit_axis.data[0];
    f32 y = unit_axis.data[1];
    f32 z = unit_axis.data[2];
    
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
