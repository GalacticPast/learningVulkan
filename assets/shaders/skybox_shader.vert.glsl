#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in vec4 in_tangent;

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 view;
    mat4 projection;
} global_ubo;

layout(location = 0) out vec3 tex_coord;

void main()
{
    tex_coord = in_position;
    mat4 view = mat4(mat3(global_ubo.view));
    view = global_ubo.view;
    gl_Position = global_ubo.projection * view * vec4(in_position, 1.0);
}
