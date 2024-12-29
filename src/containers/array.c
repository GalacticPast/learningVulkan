#include "array.h"
#include <stdlib.h>
#include <string.h>

void *array_create(u64 length, u64 stride)
{
    u64 array_length = length;
    u64 array_stride = stride;
    u64 array_size = array_length * array_stride;

    u64 *block = malloc(array_size);
    memset(block, 0, array_size);

    block[ARRAY_CAPACITY] = array_length;
    block[ARRAY_LENGTH] = 0;
    block[ARRAY_STRIDE] = array_stride;

    return (void *)block + sizeof(array_header);
}

void array_destroy(void *block)
{
    void *array = block - sizeof(array_header);
    free(array);
}

void array_set_length(void *block, u64 length)
{
    u64 *array = block - sizeof(array_header);
    array[ARRAY_LENGTH] += length;
}

void array_set_capacity(void *block, u64 capacity)
{
    u64 *array = block - sizeof(array_header);
    array[ARRAY_CAPACITY] += capacity;
}

u64 array_get_length(void *block)
{
    u64 *array = block - sizeof(array_header);
    return array[ARRAY_LENGTH];
}

u64 array_get_capacity(void *block)
{
    u64 *array = block - sizeof(array_header);
    return array[ARRAY_CAPACITY];
}

u64 array_get_stride(void *block)
{
    u64 *array = block - sizeof(array_header);
    return array[ARRAY_STRIDE];
}

// @param: block array itself
// @param: resize factor == old_array_capacity * resize_factor
void *array_resize(void *block, u32 resize_factor)
{
    u64 array_length = array_get_length(block);
    u64 array_capacity = array_get_capacity(block);
    u64 array_stride = array_get_stride(block);
    u64 array_size = array_capacity * array_stride;

    void *new_array = array_create(array_capacity * resize_factor, array_stride);

    memcpy(new_array, block, array_size);
    array_set_length(new_array, array_length);

    return new_array;
}

void array_push_value(void *block, void *value)
{
    u64 array_length = array_get_length(block);
    u64 array_stride = array_get_stride(block);
    u64 array_capacity = array_get_capacity(block);

    if (array_length + 1 >= array_capacity)
    {
        block = array_resize(block, ARRAY_DEFAULT_RESIZE_FACTOR);
        array_length = array_get_length(block);
        array_stride = array_get_stride(block);
        array_capacity = array_get_capacity(block);
    }
    void *ind_ptr = block + ((array_length + 1) * array_stride);
    memcpy(ind_ptr, value, array_stride);
    array_set_length(block, array_length + 1);
}

void array_insert_at(void *block, s32 index, void *data)
{
    u64 array_length = array_get_length(block);
    u64 array_stride = array_get_stride(block);
    u64 array_capacity = array_get_capacity(block);
    u64 array_size = array_length * array_stride;

    if (index >= array_capacity)
    {
        block = array_resize(block, ARRAY_DEFAULT_RESIZE_FACTOR);
    }

    array_length = array_get_length(block);
    array_capacity = array_get_capacity(block);
    array_size = array_length * array_stride;
}
