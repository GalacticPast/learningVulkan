#include "dstring.hpp"
#include "core/dasserts.hpp"
#include "core/dmemory.hpp"

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
    u32 str_len       = strlen(src);
    void *dest_offset = (u8 *)dest + offset_to_dest;
    dcopy_memory(dest_offset, (void *)src, (u64)str_len);
    return str_len;
}

u32 string_copy_format(char *dest, const char *src, u32 offset_to_dest, ...)
{
    DASSERT(offset_to_dest != INVALID_ID);

    __builtin_va_list arg_ptr;
    va_start(arg_ptr, src);

    char buffer[32000];
    s32 written     = vsnprintf(buffer, 32000, src, arg_ptr);
    buffer[written] = 0;

    void *dest_offset = (u8 *)dest + offset_to_dest;

    dcopy_memory((void *)dest_offset, buffer, written + 1);

    va_end(arg_ptr);

    return written + 1;
}
