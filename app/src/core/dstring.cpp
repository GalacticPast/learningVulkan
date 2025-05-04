#include "dstring.hpp"
#include "core/dasserts.hpp"
#include "core/dmemory.hpp"
#include "core/logger.hpp"

// TODO: temporary
#include <cstdio>
#include <cstring>
#include <stdarg.h>

bool string_compare(const char *str0, const char *str1)
{
    // INFO: stcmp returns 0 for sucess...wow...
    bool result = strcmp(str0, str1);
    return result == 0 ? 1 : 0;
}

u32 string_copy(char *dest, const char *src, u32 offset_to_dest)
{
    u32   str_len     = (u32)strlen(src);
    void *dest_offset = (u8 *)dest + offset_to_dest;
    dcopy_memory(dest_offset, (void *)src, (u64)str_len);
    return str_len;
}
void string_ncopy(char *dest, const char *src, u32 length)
{
    dcopy_memory(dest, (void *)src, length);
}

u32 string_copy_format(char *dest, const char *format, u32 offset_to_dest, ...)
{
    DASSERT(offset_to_dest != INVALID_ID);

    __builtin_va_list arg_ptr;
    va_start(arg_ptr, format);

    char buffer[32000];
    s32  written    = vsnprintf(buffer, 32000, format, arg_ptr);
    buffer[written] = 0;

    void *dest_offset = (u8 *)dest + offset_to_dest;

    dcopy_memory((void *)dest_offset, buffer, written + 1);

    va_end(arg_ptr);

    return written + 1;
}

void dstring::operator=(const dstring *in_string)
{
    dcopy_memory(this, string, sizeof(dstring));
}

void dstring::operator=(const char *c_string)
{
    u64 len = strlen(c_string);
    if (len > MAX_STRING_LENGTH)
    {
        DWARN("String you want to assign is bigger than 512bytes.");
        return;
    }
    if (string[0])
    {
        DWARN("Overiding string %s", string);
        dzero_memory(string, 512);
    }
    dcopy_memory(string, (char *)c_string, len * sizeof(char));
    str_len     = len;
    string[len] = '\0';
}
const char *dstring::c_str()
{
    return string;
}

void string_copy_length(char *dest, const char *src, u32 len)
{
    dcopy_memory(dest, (void *)src, (u64)len);
}
s32 string_first_char_occurence(const char *string, const char ch)
{
    u32 str_len = strlen(string);

    for (u32 i = 0; i < str_len; i++)
    {
        if (string[i] == ch)
        {
            return i;
        }
    }
    return -1;
}

s32 string_first_string_occurence(const char *string, const char *sub_str)
{
    u32 str_len     = strlen(string);
    u32 sub_str_len = strlen(sub_str);

    // im trollin :)
    char *ptr  = (char *)string;
    char  temp = ' ';
    u32   ans  = -1;

    for (u32 i = 0; i < str_len - sub_str_len && ans == -1; i++)
    {
        temp             = ptr[sub_str_len];
        ptr[sub_str_len] = '\0';
        if (string_compare(ptr, sub_str))
        {
            ans = i;
        }
        ptr[sub_str_len] = temp;
        temp             = ' ';
        ptr++;
    }
    return ans;
}
dstring::dstring()
{
}

dstring::dstring(const char *c_string)
{
    u64 len = strlen(c_string);
    if (len > MAX_STRING_LENGTH)
    {
        DWARN("String you want to assign is bigger than 512bytes.");
        return;
    }
    dcopy_memory(string, (char *)c_string, len * sizeof(char));
    str_len     = len;
    string[len] = '\0';
}

// @param: ch-> keep searching till the first occurecne of the character
bool string_to_vec4(const char *string, vec4 *vector, const char ch)
{
    char  floats[64] = {};
    char *ptr        = (char *)string;

    u32 occurence = string_first_char_occurence(ptr, '[');
    if (occurence == -1)
    {
        DERROR("Couldnt find %s in %s", ch, string);
        return false;
    }
    ptr += occurence + 1;

    occurence = string_first_char_occurence(string, ']');
    if (occurence == -1)
    {
        DERROR("Couldnt find %s in %s", ch, string);
        return false;
    }

    string_copy_length(floats, ptr, occurence - 1);

    for (int i = 0; i < occurence - 1; i++)
    {
        if (floats[i] == ',' || floats[i] == '"' || floats[i] == '[' || floats[i] == ']')
        {
            floats[i] = ' ';
        }
    }

    vec4 result = vec4();
#ifdef DPLATFORM_WINDOWS
    sscanf_s(floats, "%f %f %f %f", &result.r, &result.g, &result.b, &result.a);
#elif DPLATFORM_LINUX
    sscanf(floats, "%f %f %f %f", &result.r, &result.g, &result.b, &result.a);
#endif
    *vector = result;
    return true;
}
