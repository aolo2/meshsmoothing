static inline struct ms_v3
ms_math_avg(struct ms_v3 a, struct ms_v3 b)
{
    struct ms_v3 result;
    
    result.x = (a.x + b.x) / 2.0f;
    result.y = (a.y + b.y) / 2.0f;
    result.z = (a.z + b.z) / 2.0f;
    
    return(result);
}