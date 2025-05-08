#pragma once
#include "defines.hpp"
//
// strings because I dont know c strings good enough :(
//

#define MAX_STRING_LENGTH 512

class dstring
{
  private:
    u64 capacity = MAX_STRING_LENGTH;

  public:
    u64  str_len     = 0;
    char string[512] = {0};

    dstring();
    dstring(const char *c_string);
    void operator=(const char *c_string);
    void operator=(const dstring *str);
    void clear();

    const char *c_str();
};

// @param: return false if strings are not equal, true if they are equal
bool string_compare(const char *str0, const char *str1);

u32 string_copy(char *dest, const char *src, u32 offset_to_dest);
u32 string_length(const char *string);

void string_ncopy(char *dest, const char *src, u32 length);

// void string_copy_length(char *dest, const char *src, u32 len);

u32 string_copy_format(char *dest, const char *format, u32 offset_to_dest, ...);

s32 string_first_char_occurence(const char *string, const char ch);
s32 string_first_string_occurence(const char *string, const char *str);
s32 string_num_of_substring_occurence(const char *string, const char *str);

// @param: ch-> keep searching till the first occurecne of the character
bool string_to_vec4(const char *string, class vec4 *vector, const char ch);
