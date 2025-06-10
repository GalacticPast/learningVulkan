#version 450
#extension GL_EXT_debug_printf : enable

layout(location = 0) out vec4 out_color;

//dir_light
layout(set = 0, binding = 1)uniform uniform_lights
{
    vec3 direction;
    vec4 color;
    vec3 view_position;
}lights;

layout(set = 1, binding = 0) uniform sampler2D albedo_map;
layout(set = 1, binding = 1) uniform sampler2D alpha_map;
layout(set = 1, binding = 2) uniform sampler2D normal_map;
layout(set = 1, binding = 3) uniform sampler2D specular_map;

// Data Transfer Object
layout(location = 1) in struct dto {
	vec4 ambient;
    vec4 diffuse_color;
	vec2 tex_coord;
	vec3 normal;
    vec3 frag_position;
    vec4 tangent;
} in_dto;


mat3 TBN;

vec4 calculate_directional_light(vec3 normal, vec3 view_direction);


void main() {

    vec3 normal = in_dto.normal;
    normal = normalize(in_dto.normal);
    vec3 tangent = in_dto.tangent.xyz;
    tangent = (tangent - dot(tangent, normal) *  normal);

    vec3 bitangent = cross(normal, in_dto.tangent.xyz) * in_dto.tangent.w;
    TBN = mat3(tangent, bitangent, normal);

    vec3 localNormal = (2.0 * texture(normal_map, in_dto.tex_coord).rgb) - 1.0;
    normal = normalize(TBN * localNormal);
    debugPrintfEXT("Position = %v3f", normal);
    vec3 view_direction = normalize(lights.view_position - in_dto.frag_position);

    out_color = calculate_directional_light(normal, view_direction);

}

vec4 calculate_directional_light(vec3 normal, vec3 view_direction) {
    float diffuse_factor = max(dot(normal, -lights.direction), 0.0);

    vec3 half_direction = normalize(view_direction - lights.direction);
    float specular_factor = pow(max(dot(half_direction, normal), 0.0), 64);

    vec4 diff_samp = texture(albedo_map, in_dto.tex_coord);
    vec4 ambient = vec4(vec3(in_dto.diffuse_color * in_dto.ambient), diff_samp.a);
    vec4 diffuse = vec4(vec3(lights.color * diffuse_factor), diff_samp.a);
    vec4 specular = vec4(vec3(lights.color * specular_factor), diff_samp.a);

    diffuse *= diff_samp;
    ambient *= diff_samp;
    specular *= vec4(texture(specular_map, in_dto.tex_coord).rgb, diff_samp.a);

    return (ambient + diffuse + specular);
}
