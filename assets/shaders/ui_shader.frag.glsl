#version 460 core
#extension GL_EXT_debug_printf : enable

layout (location=0) out vec4 out_color;

// mode 0 -> display text
// mode 1 -> display text_quad

layout(location = 0) flat in int mode;

layout(location = 1) in dto
{
    vec2 tex_coord;
    vec4 color;
}in_dto;

layout (set = 1 , binding = 0) uniform sampler2D font_atlas;

void main()
{
    bool no_tex = all(equal(in_dto.tex_coord,vec2(0,0)));
    float alpha = texture(font_atlas, in_dto.tex_coord).r;

    if(no_tex || mode == 1)
    {
        out_color = in_dto.color;
    }
    else
    {
        out_color = vec4(in_dto.color.rgb, alpha);
    }

}
