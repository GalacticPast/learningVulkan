#version 450

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_tex_coord;

layout(location = 0) out vec2 out_tex_coord;

layout(set = 0, binding = 0) uniform ui_globals
{
    mat4 view;
    mat4 projection;
}ugo;


void main()
{
    out_tex_coord = in_tex_coord;

    gl_Position = ugo.projection * ugo.view * vec4(in_position,0,1.0f);
}
