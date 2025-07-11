#include "core/dasserts.hpp"
#include "core/dmemory.hpp"
#include "core/logger.hpp"
#include "dstring.hpp"

// TODO: temporary
#include <cstdio>
#include <cstring>
#include <stdarg.h>

u32 string_length(const char *string)
{
    if (string == nullptr)
    {
        return 0;
    }
    return strlen(string);
}

bool string_compare(const char *str0, const char *str1)
{
    // INFO: stcmp returns 0 for sucess...wow...
    bool result = strcmp(str0, str1);
    return result == 0 ? 1 : 0;
}

u32 string_copy(char *dest, const char *src, u32 offset_to_dest)
{
    u32   str_len     = static_cast<u32>(strlen(src));
    void *dest_offset = reinterpret_cast<void *>(reinterpret_cast<u8 *>(dest) + offset_to_dest);
    dcopy_memory(dest_offset, reinterpret_cast<const void *>(src), static_cast<u64>(str_len));
    return str_len;
}
void string_ncopy(char *dest, const char *src, u32 length)
{
    dcopy_memory(dest, reinterpret_cast<const void *>(src), length);
}

u32 string_copy_format(char *dest, const char *format, u32 offset_to_dest, ...)
{
    DASSERT(offset_to_dest != INVALID_ID);

    __builtin_va_list arg_ptr;
    va_start(arg_ptr, format);

    char buffer[32000];
    s32  written    = vsnprintf(buffer, 32000, format, arg_ptr);
    buffer[written] = 0;

    void *dest_offset = reinterpret_cast<void *>(reinterpret_cast<u8 *>(dest) + offset_to_dest);

    dcopy_memory(reinterpret_cast<void *>(dest_offset), reinterpret_cast<const void *>(buffer), written + 1);

    va_end(arg_ptr);

    return written + 1;
}

char &dstring::operator[](u32 index)
{
    DASSERT_MSG(index < MAX_STRING_LENGTH, "Index should be less than MAX_STRING_LENGTH");
    return string[index];
}
const char &dstring::operator[](u32 index) const
{
    DASSERT_MSG(index < MAX_STRING_LENGTH, "Index should be less than MAX_STRING_LENGTH");
    return string[index];
}

dstring &dstring::operator=(const dstring *in_string)
{
    dcopy_memory(this, string, sizeof(dstring));
    DASSERT_MSG(in_string->str_len, "The passed string cannot have a size 0");
    DASSERT_MSG(in_string->str_len != MAX_STRING_LENGTH, "The passed string doesn't have a valid length");
    str_len = in_string->str_len;
    return *this;
}
dstring &dstring::operator=(const char *c_string)
{
    u64 len = strlen(c_string);
    if (len > MAX_STRING_LENGTH)
    {
        DWARN("String you want to assign is bigger than 512bytes.");
        return *this;
    }
    if (string[0])
    {
        DWARN("Overiding string %s", string);
        dzero_memory(string, 512);
    }
    dcopy_memory(string, reinterpret_cast<const void *>(c_string), len * sizeof(char));
    str_len     = len;
    string[len] = '\0';
    return *this;
}

void dstring::clear()
{
    dzero_memory(string, MAX_STRING_LENGTH);
    str_len = 0;
}

const char *dstring::c_str()
{
    return string;
}

void string_copy_length(char *dest, const char *src, u32 len)
{
    dcopy_memory(dest, reinterpret_cast<const void *>(src), static_cast<u64>(len));
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

void string_split(dstring *string, const char ch, darray<dstring> *split_strings)
{
    DASSERT(string);
    DASSERT(split_strings);

    u32 len = string->str_len;
    DASSERT(len != INVALID_ID);
    DASSERT(len < MAX_STRING_LENGTH);

    dstring a{};

    u32 a_ind = 0;

    for (u32 i = 0; i < len; i++)
    {
        if ((*string)[i] == ' ')
        {
            continue;
        }
        if ((*string)[i] == ch)
        {
            a.str_len = a_ind;
            split_strings->push_back(a);

            a.clear();
            a_ind = 0;
            continue;
        }
        a[a_ind++] = (*string)[i];
    }
    a.str_len = a_ind;
    split_strings->push_back(a);

    return;
}

s32 string_num_of_substring_occurence(const char *string, const char *sub_str)
{

    char *found = nullptr;
    char *ptr   = const_cast<char *>(string);
    u64   count = 0;

    while ((found = strstr(ptr, sub_str)) != NULL)
    {
        count++;
        ptr = found + 1;
    }
    return count;
}

const char *string_first_string_occurence(const char *string, const char *sub_str)
{
    char *ptr   = const_cast<char *>(string);
    char *found = strstr(ptr, sub_str);
    return found;
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
    dcopy_memory(string, reinterpret_cast<const void *>(c_string), len * sizeof(char));
    str_len     = len;
    string[len] = '\0';
}

// @param: ch-> keep searching till the first occurecne of the character
bool string_to_vec4(const char *string, vec4 *vector)
{
    char        floats[64] = {};
    const char *ptr        = string;

    s32 occurence = string_first_char_occurence(ptr, '{');
    if (occurence == -1)
    {
        DERROR("Couldnt find opening brace '{' in %s", string);
        return false;
    }
    ptr += occurence + 1;

    occurence = string_first_char_occurence(string, '}');
    if (occurence == -1)
    {
        DERROR("Couldnt find closing brace '}' in %s", string);
        return false;
    }

    string_copy_length(floats, ptr, occurence - 1);

    for (s32 i = 0; i < occurence - 1; i++)
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

bool string_to_u32(const char *string, u32 *integer)
{
    u32 res = INVALID_ID;

#ifdef DPLATFORM_WINDOWS
    sscanf_s(string, "%ld", &res);
#elif DPLATFORM_LINUX
    sscanf(string, "%ld", &res);
#endif
    DASSERT(res != INVALID_ID);
    return true;
}


u32 u32_to_string(char * string, u32 integer)
{
    u32 bytes_written = sprintf(string, "%d", integer);
    return bytes_written;
}

u32 f32_to_string(char * string, f32 integer)
{
    u32 bytes_written = sprintf(string, "%f", integer);
    return bytes_written;
}
