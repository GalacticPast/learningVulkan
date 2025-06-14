#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 tex_coord;

//TODO: set 0 should support image samplers
layout(set = 1, binding = 0) uniform samplerCube skybox;

void main()
{
    out_color = texture(skybox, tex_coord);
}
