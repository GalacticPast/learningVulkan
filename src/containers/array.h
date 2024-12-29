#pragma once

#include "defines.h"

#define ARRAY_DEFAULT_CAPACITY 1
#define ARRAY_DEFAULT_RESIZE_FACTOR 2

typedef enum array_header
{
    ARRAY_CAPACITY,
    ARRAY_LENGTH,
    ARRAY_STRIDE,
    MAX_ARRAY_HEADER
} array_header;

#define ARRAY_HEADER_SIZE (u64)sizeof(array_header) * (u64)sizeof(u64)

void *array_create(u64 length, u64 stride);
void  array_destroy(void *array);

u64 array_get_length(void *array);
u64 array_get_capacity(void *array);
u64 array_get_stride(void *array);

void *array_insert_at(void *array, s32 index, void *data);
void *array_pop_at(void *array, s32 index);
void *array_push_value(void *array, const void *value_ptr);
void  array_pop_value(void *array);
