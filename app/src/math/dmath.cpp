#include "dmath.hpp"
#include "platform/platform.hpp"
#include "resources/resource_types.hpp"

static bool rand_seeded = false;

s32 drandom()
{
    if (!rand_seeded)
    {
        srand((u32)platform_get_absolute_time());
        rand_seeded = true;
    }
    return rand();
}

s32 drandom_in_range(s32 min, s32 max)
{
    if (!rand_seeded)
    {
        srand((u32)platform_get_absolute_time());
        rand_seeded = true;
    }
    return (rand() % (max - min + 1)) + min;
}

f32 fdrandom()
{
    return (float)drandom() / (f32)RAND_MAX;
}

f32 fdrandom_in_range(f32 min, f32 max)
{
    return min + ((float)drandom() / ((f32)RAND_MAX / (max - min)));
}

// HACK:
//  for scaling

void scale_geometries(const geometry_config *config, math::vec3 scaling_factor)
{
    u32 vertex_count = config->vertex_count;

    for (u32 i = 0; i < vertex_count; i++)
    {
        config->vertices[i].position.x *= scaling_factor.x;
        config->vertices[i].position.y *= scaling_factor.y;
        config->vertices[i].position.z *= scaling_factor.z;
    }
}
