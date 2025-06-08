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
} in_dto;

vec4 calculate_directional_light(vec3 view_direction, vec3 normal);

void main() {
    vec3 view_direction = normalize(dir_light.camera_pos - in_dto.frag_position);

    out_colour = calculate_directional_light(view_direction, in_dto.normal);
}

vec4 calculate_directional_light(vec3 view_direction, vec3 normal) {
    float diffuse_factor = max(dot(normal, -dir_light.direction), 0.0);

    vec3 half_direction = normalize(view_direction - dir_light.direction);
    float specular_factor = pow(max(dot(half_direction, normal), 0.0), 32);

    vec4 diff_samp = texture(albedo_map, in_dto.frag_tex_coord);
    vec4 ambient = vec4(vec3(in_dto.ambient * in_dto.diffuse_color), diff_samp.a);
    vec4 diffuse = vec4(vec3(dir_light.color * diffuse_factor), diff_samp.a);
    vec4 specular = vec4(vec3(dir_light.color * specular_factor), diff_samp.a);

    diffuse *= diff_samp;
    ambient *= diff_samp;
    specular *= vec4(texture(specular_map, in_dto.frag_tex_coord).rgb, diffuse.a);

    return (ambient + diffuse + specular);
}
