#version 450

layout(location = 0) in vec3 in_position;

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 view;
    mat4 projection;
} global_ubo;

layout(location = 0) out vec3 tex_coord;

void main()
{
    tex_coord = in_position;
    gl_Position = global_ubo.projection * global_ubo.view * vec4(in_position, 1.0);
}
