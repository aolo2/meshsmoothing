static u64
usec_now()
{
    u64 result;
    struct timespec ts;
    
    clock_gettime(CLOCK_MONOTONIC, &ts);
    result = ts.tv_sec * 1000000UL + ts.tv_nsec / 1000UL;
    
    return(result);
}