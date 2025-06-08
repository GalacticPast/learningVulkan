#version 450
#extension GL_EXT_debug_printf : enable

layout(location = 0) out vec4 out_colour;

//light
layout(set = 0, binding = 1)uniform directional_light
{
    vec3 direction;
    vec4 color;
    vec3 camera_pos;
}dir_light;

layout(set = 1, binding = 0) uniform sampler2D albedo_map;
layout(set = 1, binding = 1) uniform sampler2D alpha_map;
layout(set = 1, binding = 2) uniform sampler2D normal_map;
layout(set = 1, binding = 3) uniform sampler2D specular_map;

// Data Transfer Object
layout(location = 1) in struct dto {
	vec4 ambient;
    vec4 diffuse_color;
	vec2 frag_tex_coord;
	vec3 normal;
    vec3 frag_position;
    mat3 TBN;
} in_dto;

vec4 calculate_directional_light(vec3 view_direction, vec3 normal);

void main() {
    vec3 view_direction = normalize(dir_light.camera_pos - in_dto.frag_position);

    vec3 normal = in_dto.normal;
    normal = texture(normal_map, in_dto.frag_tex_coord).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize(in_dto.TBN * normal);
    out_colour = calculate_directional_light(view_direction, normal);
}

vec4 calculate_directional_light(vec3 view_direction, vec3 normal) {

    // negate the direction vector because if not the dot product will come out as negative
    float diffuse_factor = dot(-dir_light.direction, normal);
    // remove negative values (ie the pixel is facing the away from the light)
    diffuse_factor    = max(diffuse_factor, 0.0f);
    vec4 base_sampler = texture(albedo_map, in_dto.frag_tex_coord);

    vec3 reflect_dir    = reflect(dir_light.direction, normal);
    float spec_strength = pow(max(dot(view_direction, reflect_dir),0.0),32);

    vec4 specular     = (spec_strength * dir_light.color);
    vec4 diffuse      = (diffuse_factor * dir_light.color );
    vec4 ambient      = (in_dto.ambient * dir_light.color);

    diffuse  *= base_sampler;
    ambient  *= base_sampler;
    specular *= texture(specular_map, in_dto.frag_tex_coord);

    return (diffuse + ambient + specular);
}
