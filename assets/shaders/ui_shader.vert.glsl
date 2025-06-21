#version 460 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in vec4 in_color;

layout(set = 0, binding = 0) uniform ui_globals
{
    mat4 view;
    mat4 projection;
    int mode;
}ugo;

// I didnt know that the shader would mutuate the values of dto's member variables
layout(location = 0) flat out int mode;

layout(location = 1) out dto
{
    vec2 tex_coord;
    vec4 color;
}out_dto;


void main()
{
    mode = ugo.mode;
    out_dto.tex_coord = in_tex_coord;
    out_dto.color = in_color;

    gl_Position = ugo.projection * ugo.view * vec4(in_position,0,1.0f);
}
