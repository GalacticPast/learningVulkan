#version 450

layout(set = 1, binding = 0) uniform sampler2D albedo_map;
layout(set = 1, binding = 1) uniform sampler2D alpha_map;

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_tex_coord;

layout(location = 0) out vec4 out_color;

void main() {
    vec4 color = texture(albedo_map, frag_tex_coord);
    float alpha = texture(alpha_map, frag_tex_coord).r;  

    if (alpha < 0.5)
        discard;  

    out_color = vec4(color.rgb, alpha);
}
