#include "array.h"
#include "core/logger.h"

#include <stdlib.h>
#include <string.h>

void *__array_create(u64 capacity, u64 stride)
{
    u64 array_capacity = capacity;
    u64 array_stride = stride;
    u64 array_size = array_capacity * array_stride + ARRAY_HEADER_SIZE;

    u64 *block = calloc(array_size, array_stride);

    block[ARRAY_CAPACITY] = array_capacity;
    block[ARRAY_LENGTH] = 0;
    block[ARRAY_STRIDE] = array_stride;

    return (void *)block + ARRAY_HEADER_SIZE;
}

void array_destroy(void *block)
{
    void *array = block - ARRAY_HEADER_SIZE;
    free(array);
}

void array_set_length(void *array, u64 length)
{
    u64 *array_start = array - ARRAY_HEADER_SIZE;
    array_start[ARRAY_LENGTH] = length;
}

void array_set_capacity(void *block, u64 capacity)
{
    u64 *array = block - ARRAY_HEADER_SIZE;
    array[ARRAY_CAPACITY] = capacity;
}

u64 array_get_length(void *block)
{
    u64 *array = block - ARRAY_HEADER_SIZE;
    return array[ARRAY_LENGTH];
}

u64 array_get_capacity(void *block)
{
    u64 *array = block - ARRAY_HEADER_SIZE;
    return array[ARRAY_CAPACITY];
}

u64 array_get_stride(void *block)
{
    u64 *array = block - ARRAY_HEADER_SIZE;
    return array[ARRAY_STRIDE];
}

// @param: array -> array that you want to resize
// @param: resize factor sepcifies by which num multiple that you want to resize the array by
void *array_resize(void *array, u32 resize_factor)
{
    if (resize_factor >= 1e4)
    {
        ERROR("Provided reisze_factor is %d. Maybe an ERROR? ", resize_factor);
        exit(1);
    }
    u64 array_length = array_get_length(array);
    u64 array_capacity = array_get_capacity(array);
    u64 array_stride = array_get_stride(array);

    u64   new_array_capacity = array_capacity * resize_factor;
    void *new_array = __array_create(new_array_capacity, array_stride);
    memcpy(new_array, array, array_length * array_stride);
    array_set_length(new_array, array_length);

    array_destroy(array);

    return new_array;
}

// @param: vaule size must be the same as the array stride
void *array_push_value(void *array, const void *value_ptr)
{
    u64 array_length = array_get_length(array);
    u64 array_stride = array_get_stride(array);
    u64 array_capacity = array_get_capacity(array);

    if (array_length >= array_capacity)
    {
        array = array_resize(array, ARRAY_DEFAULT_RESIZE_FACTOR);
    }
    void *addr = array + (array_length * array_stride);
    memcpy(addr, value_ptr, array_stride);
    array_set_length(array, array_length + 1);
    return array;
}

void array_pop_value(void *array)
{
    u64 array_length = array_get_length(array);
    array_set_length(array, array_length - 1);
}

void *array_pop_at(void *array, s32 index)
{
    u64 array_length = array_get_length(array);
    u64 array_stride = array_get_stride(array);

    if (index >= (s32)array_length || index < 0)
    {
        ERROR("Provided Index:%d is out of bounds, index must be 0 <= index < %d . Exiting application", index, array_length);
        exit(1);
    }

    void *src = array + ((index + 1) * array_stride);
    void *dest = array + (index * array_stride);

    u64 src_size = (array_length - (index + 1)) * array_stride;
    memmove(dest, src, src_size);
    array_set_length(array, array_length - 1);

    return array;
}

void *array_insert_at(void *array, s32 index, void *data)
{
    u64 array_length = array_get_length(array);
    u64 array_stride = array_get_stride(array);
    u64 array_capacity = array_get_capacity(array);

    if (index >= (s32)array_length || index < 0)
    {
        ERROR("Provided Index:%d is out of bounds, index must be 0 <= index < %d . Exiting application", index, array_length);
        exit(1);
    }

    if (index >= (s32)array_capacity)
    {
        array = array_resize(array, ARRAY_DEFAULT_RESIZE_FACTOR);
    }

    void *dest = array + ((index + 1) * array_stride);
    void *src = array + (index * array_stride);

    u64 src_size = (array_length - index) * array_stride;
    memmove(dest, src, src_size);
    memcpy(src, data, array_stride);
    array_set_length(array, array_length + 1);
    return array;
}
