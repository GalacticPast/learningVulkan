#pragma once
#include "containers/darray.hpp"
#include "defines.hpp"
#include "math/dmath_types.hpp"
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
    char       &operator[](u32 index);
    const char &operator[](u32 index) const;
    dstring    &operator=(const char *c_string);
    dstring    &operator=(const dstring *str);
    dstring    &operator=(const char ch);
    void        clear();

    const char *c_str();
};

// @param: return false if strings are not equal, true if they are equal
bool string_compare(const char *str0, const char *str1);
// ch -> identifer to split the string to or if it cannot find the identifer
// it will copy everything till it encounters a new line char
void string_split(dstring *string, const char ch, darray<dstring> *split_strings);

u32 string_copy(char *dest, const char *src, u32 offset_to_dest);
u32 string_length(const char *string);

void string_ncopy(char *dest, const char *src, u32 length);

// void string_copy_length(char *dest, const char *src, u32 len);

u32 string_copy_format(char *dest, const char *format, u32 offset_to_dest, ...);

s32         string_first_char_occurence(const char *string, const char ch);
const char *string_first_string_occurence(const char *string, const char *sub_str);

s32 string_num_of_substring_occurence(const char *string, const char *str);

bool string_to_vec4(const char *string, math::vec4 *vector);
bool string_to_u32(const char *string, u32 *integer);
