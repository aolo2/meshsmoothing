static u64
usec_now(void)
{
    u64 result;
    struct timespec ts;
    
    clock_gettime(CLOCK_MONOTONIC, &ts);
    result = ts.tv_sec * 1000000UL + ts.tv_nsec / 1000UL;
    
    return(result);
}

static u64
cycles_now(void)
{
    u32 hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return((u64) lo) | (((u64) hi) << 32);
}