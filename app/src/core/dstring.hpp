#pragma once
#include "defines.hpp"
#include "math/dmath.hpp"

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
    dstring();
    dstring(const char *c_string);
    void operator=(const char *c_string);
    void operator=(const dstring *str);

    const char *c_str();
};

// @param: return false if strings are not equal, true if they are equal
bool string_compare(const char *str0, const char *str1);

u32 string_copy(char *dest, const char *src, u32 offset_to_dest);

void string_ncopy(char *dest, const char *src, u32 length);

void string_copy_length(char *dest, const char *src, u32 len);

u32 string_copy_format(char *dest, const char *format, u32 offset_to_dest, ...);

s32 string_first_char_occurence(const char *string, const char ch);
s32 string_first_string_occurence(const char *string, const char *str);

// @param: ch-> keep searching till the first occurecne of the character
bool string_to_vec4(const char *string, vec4 *vector, const char ch);
