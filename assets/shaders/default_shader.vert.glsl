#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in vec4 in_tangent;

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 view;
    mat4 projection;
    vec4 ambient_color;
} global_ubo;

layout(push_constant) uniform push_constants {
    mat4 model;
    vec4 diffuse_color;
} u_push_constants;

layout(location = 0) out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 1) out struct dto {
    vec4 ambient;
    vec4 diffuse_color;
    vec2 tex_coord;
    vec3 normal;
    vec3 frag_position;
    vec4 tangent;
} out_dto;

void main() {
    mat4 model = u_push_constants.model;
    mat3 normal_matrix = transpose(inverse(mat3(model)));

    // Transform vertex position to world space
    vec4 world_pos = model * vec4(in_position, 1.0);
    out_dto.frag_position = world_pos.xyz;

    // Transform normal and tangent to world space
    out_dto.normal = normalize(normal_matrix * in_normal);

    vec3 world_tangent = normalize(normal_matrix * in_tangent.xyz);
    out_dto.tangent = vec4(world_tangent, in_tangent.w); // Preserve handedness

    // Pass through texture coordinates and colors
    out_dto.tex_coord = in_texcoord;
    out_dto.diffuse_color = u_push_constants.diffuse_color;
    out_dto.ambient = global_ubo.ambient_color;

    // Compute clip-space position
    gl_Position = global_ubo.projection * global_ubo.view * world_pos;
}

