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

void *array_create(u64 length, u64 stride);
void  array_destroy(void *block);
u64   array_get_length(void *block);
u64   array_get_capacity(void *block);

void array_insert_at(void *block, s32 index, void *data);
void array_push_value(void *block, void *value);
