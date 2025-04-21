#pragma once
#include "defines.hpp"

// strings because I dont know c strings good enough :(
//
#define MAX_STRING_LENGTH 512
class dstring
{
  private:
    u64  capacity    = 0;
    u64  str_len     = 0;
    char string[512] = {0};

  public:
    void operator=(const char *c_string);
    void operator=(const dstring *str);

    const char *c_str();
};

// @param: return false if strings are not equal, true if they are equal
bool string_compare(const char *str0, const char *str1);

u32 string_copy(char *dest, const char *src, u32 offset_to_dest);

u32 string_copy_format(char *dest, const char *format, u32 offset_to_dest, ...);
