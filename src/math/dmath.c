#include "dmath.h"
#include "platform/platform.h"

#include <math.h>
#include <stdlib.h>

static b8 rand_seeded = false;

/**
 * Note that these are here in order to prevent having to import the
 * entire <math.h> everywhere.
 */
f32 dsin(f32 x)
{
    return sinf(x);
}

f32 dcos(f32 x)
{
    return cosf(x);
}

f32 dtan(f32 x)
{
    return tanf(x);
}

f32 dacos(f32 x)
{
    return acosf(x);
}

f32 dsqrt(f32 x)
{
    return sqrtf(x);
}

f32 dabs(f32 x)
{
    return fabsf(x);
}
