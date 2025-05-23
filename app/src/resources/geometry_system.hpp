#pragma once

#include "resource_types.hpp"

bool geometry_system_initialize(u64 *geometry_system_mem_requirements, void *state);
bool geometry_system_shutdowm(void *state);

// width= width of the plane
// height = height of the plane
// x_segment_count = how many subdisions that you want in the horizontal direction, more the subdivisions more the
// clarity y_segment_count =how many subdisions that you want in the vertical direction, more the subdivisions more the
// clarity tile_x how many times do you want the texture to tile in the horizontal direction tile_y how many times do
// you want the texture to tile in the vertical direction

geometry_config geometry_system_generate_plane_config(f32 width, f32 height, u32 x_segment_count, u32 y_segment_count,
                                                      f32 tile_x, f32 tile_y, const char *name,
                                                      const char *material_name);
geometry       *geometry_system_get_geometry(geometry_config *config);
geometry       *geometry_system_get_geometry_by_name(dstring geometry_name);
geometry       *geometry_system_get_default_geometry();
geometry_config geometry_system_generate_cube_config();

void geometry_system_get_geometries_from_file(const char *obj_file_name, const char *mtl_file_name, geometry ***geos,
                                              u32 *geometry_count);
