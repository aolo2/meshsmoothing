static inline struct ms_v3
ms_math_avg(struct ms_v3 a, struct ms_v3 b)
{
    struct ms_v3 result;
    
    result.x = (a.x + b.x) * 0.5f;
    result.y = (a.y + b.y) * 0.5f;
    result.z = (a.z + b.z) * 0.5f;
    
    return(result);
}