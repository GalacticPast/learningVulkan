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
    // dcopy_memory(string, c_string, len * sizeof(char));
    // capacity = len * sizeof(char);
    // str_len  = len;
}

void dstring::operator=(const char *c_string)
{
    u64 len = strlen(c_string);
    if (string)
    {
        DWARN("Overiding string %s", string);
        dfree(string, capacity, MEM_TAG_DSTRING);
    }

    string = (char *)dallocate((len + 1) * sizeof(char), MEM_TAG_DSTRING);
    dset_memory_value(string, '\0', len + 1);
    dcopy_memory(string, (char *)c_string, len * sizeof(char));
    capacity = sizeof(char) * (len + 1);
    str_len  = len + 1;
}
const char *dstring::c_str()
{
    return string;
}
