#version 460 core

layout (location=0) out vec4 out_color;

layout(location = 0) in vec2 tex_coord;

layout (set = 1 , binding = 0) uniform sampler2D font_atlas;

void main()
{
    float alpha = texture(font_atlas, tex_coord).r;
    out_color = vec4(1,1,1,alpha);
}
