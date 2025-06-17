#version 460 core

layout (location=0) out vec4 out_color;

layout(location = 0) in vec2 tex_coord;

layout (set = 1 , binding = 0) uniform sampler2D font_atlas;

void main()
{
   out_color = texture(font_atlas, tex_coord);
}
