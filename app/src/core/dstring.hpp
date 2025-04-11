#pragma once
#include "defines.hpp"

// @param: return false if strings are not equal, true if they are equal
bool string_compare(const char *str0, const char *str1);

u32 string_copy(char *dest, const char *src, u32 offset_to_dest);

u32 string_copy_format(char *dest, const char *src, u32 offset_to_dest, ...);
