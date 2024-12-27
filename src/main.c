#include "core/logger.h"
#include "defines.h"

s32 main(s32 argc, char **argv)
{
    FATAL("FATAL %f", 3.192123);
    ERROR("ERROR %d", 2);
    DEBUG("DEBUG %d", 3);
    INFO("INFO %d", 4);
    TRACE("TRACE %d", 5);
}
