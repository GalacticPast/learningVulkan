#version 450
#extension GL_EXT_debug_printf : enable

layout(location = 0) out vec4 out_color;

// Directional light
layout(set = 0, binding = 1) uniform uniform_lights {
    vec3 direction;       // Light direction in world space
    vec4 color;           // Light color and intensity
    vec3 view_position;   // Camera position in world space
} lights;

// Textures
layout(set = 1, binding = 0) uniform sampler2D albedo_map;
layout(set = 1, binding = 1) uniform sampler2D alpha_map;
layout(set = 1, binding = 2) uniform sampler2D normal_map;
layout(set = 1, binding = 3) uniform sampler2D specular_map;

// Input from vertex shader
layout(location = 1) in struct dto {
    vec4 ambient;
    vec4 diffuse_color;
    vec2 tex_coord;
    vec3 normal;
    vec3 frag_position;
    vec4 tangent;  // xyz = tangent, w = handedness
} in_dto;

void main() {
    // === Sample textures ===
    vec4 albedo_color = texture(albedo_map, in_dto.tex_coord);
    float alpha = texture(alpha_map, in_dto.tex_coord).r;

    // Alpha test (discard transparent fragments)
    if (alpha < 0.1) discard;

    vec3 normal = normalize(in_dto.normal);

    // === Tangent space normal mapping ===
    vec3 tangent = normalize(in_dto.tangent.xyz);
    vec3 bitangent = normalize(cross(normal, tangent) * in_dto.tangent.w);
    mat3 TBN = mat3(tangent, bitangent, normal);

    vec3 sampled_normal = texture(normal_map, in_dto.tex_coord).xyz * 2.0 - 1.0;
    vec3 world_normal = normalize(TBN * sampled_normal);

    // === Lighting calculations ===
    vec3 light_dir = normalize(-lights.direction); // light direction *toward* surface
    vec3 view_dir = normalize(lights.view_position - in_dto.frag_position);
    vec3 half_vec = normalize(light_dir + view_dir);

    // Diffuse component
    float diff = max(dot(world_normal, light_dir), 0.0);

    // Specular component (Blinn-Phong)
    float spec_strength = texture(specular_map, in_dto.tex_coord).r;
    float spec = pow(max(dot(world_normal, half_vec), 0.0), 32.0) * spec_strength;

    vec3 ambient = in_dto.ambient.rgb * albedo_color.rgb;
    vec3 diffuse = diff * in_dto.diffuse_color.rgb * albedo_color.rgb;
    vec3 specular = spec * lights.color.rgb;

    vec3 final_color = ambient + diffuse + specular;
    out_color = vec4(final_color, albedo_color.a);
}

