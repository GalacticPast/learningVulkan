#include "strings.h"
#include <string.h>

b8 string_compare(const char *str1, const char *str2)
{
    s8 result = (s8)strcmp(str1, str2);
    return result == 0 ? true : false;
}
