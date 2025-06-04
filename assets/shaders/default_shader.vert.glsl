#version 450
#extension GL_EXT_debug_printf : enable

layout(set = 0, binding = 0) uniform uniform_global_object{
    mat4 view;
    mat4 proj;
} ugo;

layout(push_constant) uniform push_constants{
    mat4 model;
    vec4 diffuse_color;
    vec4 padding1;
    vec4 padding2;
    vec4 padding3;
} pc;

//attributes
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;


layout(location = 0) out vec3   frag_color;
layout(location = 1) out vec3   frag_normal;
layout(location = 2) out vec2   frag_tex_coord;
layout(location = 3) out vec3   frag_position;


void main() {
    frag_color      = vec3(pc.diffuse_color);
    frag_normal     = in_normal;
    frag_tex_coord  = in_tex_coord;
    frag_position   = vec3(pc.model * vec4(in_position, 1.0f));


    gl_Position = ugo.proj * ugo.view * pc.model * vec4(in_position, 1.0);
}
