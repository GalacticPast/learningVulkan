#include "dmath.hpp"
#include "core/dasserts.hpp"
#include "platform/platform.hpp"
#include "resources/resource_types.hpp"
//
#include <cstdint>
#include <iostream>
#include <random>
//
static bool rand_seeded = false;

s32 drandom()
{
    if (!rand_seeded)
    {
        srand(static_cast<u32>(platform_get_absolute_time()));
        rand_seeded = true;
    }
    return rand();
}

s64 drandom_s64()
{
    static std::random_device rd;

    u64 high = static_cast<u64>(rd()) << 32;
    u64 low  = static_cast<u64>(rd());

    u64 random_value = high | low;
    DASSERT(random_value != INVALID_ID_64);

    return static_cast<s64>(random_value);
}

s64 drandom_in_range_64(s64 min, s64 max)
{
    s64 value = (drandom_s64() % (max - min + 1)) + min;
    return value;
}

s32 drandom_in_range(s32 min, s32 max)
{
    if (!rand_seeded)
    {
        srand(static_cast<u32>(platform_get_absolute_time()));
        rand_seeded = true;
    }
    return (rand() % (max - min + 1)) + min;
}

f32 fdrandom()
{
    return static_cast<f32>(drandom()) / static_cast<f32>(RAND_MAX);
}

f32 fdrandom_in_range(f32 min, f32 max)
{
    return min + (static_cast<f32>(drandom()) / (static_cast<f32>(RAND_MAX / (max - min))));
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
