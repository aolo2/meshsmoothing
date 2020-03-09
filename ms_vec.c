static struct ms_vec
ms_vec_init(int size)
{
    struct ms_vec result = { 0 };
    
    result.cap = size;
    result.data = malloc(size * sizeof(int));
    result.len = 0;
    
    assert(result.data);
    
    return(result);
    
}

static void
ms_vec_push(struct ms_vec *vec, int item)
{
    if (vec->cap == vec->len) {
        /* Most vertices will have just a few neighbours. Using small 
initial size, and not growing by much */
        int new_cap = vec->cap + 2;
        vec->data = realloc(vec->data, new_cap * sizeof(int));
        vec->cap = new_cap;
        assert(vec->data);
    }
    
    vec->data[vec->len++] = item;
}