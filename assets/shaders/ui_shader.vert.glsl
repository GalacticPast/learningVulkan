#version 450

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_tex_coord;

layout(push_constant) uniform push_constants
{
    mat4 projection; // orthograpic projection
}pc;

void main()
{
    gl_Position = pc.projection * vec4(in_position,0,1.0f);
}
