#version 450

layout(set = 0, binding = 1)uniform uniform_light_object
{
    vec3 light_color;
}ulo;

layout(set = 1, binding = 1) uniform sampler2D albedo_map;
layout(set = 1, binding = 2) uniform sampler2D alpha_map;

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_tex_coord;

layout(location = 0) out vec4 out_color;

void main() {
    vec4 color = vec4(ulo.light_color * frag_color, 1.0f);
    out_color = color;
}
