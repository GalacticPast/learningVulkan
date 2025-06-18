#pragma once

#include "resource_types.hpp"

bool geometry_system_initialize(arena* arena, u64 *geometry_system_mem_requirements, void *state);
bool geometry_system_shutdowm(void *state);

// call this first
u64 geometry_system_create_geometry(geometry_config *config, bool use_name);

geometry_config* geometry_system_generate_config(dstring obj_file_name);

geometry *geometry_system_get_geometry(u64 id);

geometry *geometry_system_get_default_geometry();
geometry *geometry_system_get_default_plane();

void geometry_system_get_geometries_from_file(const char *obj_file_name, const char *mtl_file_name, geometry ***geos,
                                              u32 *geometry_count);

// width= width of the plane
// height = height of the plane
// x_segment_count = how many subdisions that you want in the horizontal direction, more the subdivisions more the
// clarity y_segment_count =how many subdisions that you want in the vertical direction, more the subdivisions more the
// clarity tile_x how many times do you want the texture to tile in the horizontal direction tile_y how many times do
// you want the texture to tile in the vertical direction

geometry_config geometry_system_generate_plane_config(f32 width, f32 height, u32 x_segment_count, u32 y_segment_count,
                                                      f32 tile_x, f32 tile_y, const char *name,
                                                      const char *material_name);
geometry_config geometry_system_generate_quad_config(f32 width, f32 height,f32 posx, f32 posy, dstring *name);

void geometry_system_copy_config(geometry_config *dst_config, const geometry_config *src_config);

bool geometry_system_generate_text_geometry(dstring* text, vec2 position, vec2 dimensions);
u64 geometry_system_flush_text_geometries();

