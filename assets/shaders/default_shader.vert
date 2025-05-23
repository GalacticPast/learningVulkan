#version 450
#extension GL_EXT_debug_printf : enable

layout(set = 0, binding = 0) uniform uniform_buffer_object{
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normals;
layout(location = 2) in vec2 in_tex_coord;


layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_tex_coord;


void main() {
    gl_Position = ubo.proj * ubo.view * vec4(in_position, 1.0);
    frag_color = vec3(0.1,0.4,1.0);
    frag_tex_coord = in_tex_coord;
}
