#version 450
#extension GL_EXT_debug_printf : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_tangent;
layout(location = 3) in vec2 in_texcoord;

layout(set = 0, binding = 0) uniform global_uniform_object {
	mat4 view;
    mat4 projection;
	vec4 ambient_colour;
} global_ubo;

layout(push_constant) uniform push_constants {
	mat4 model; // 64 bytes
    vec4 diffuse_color;
} u_push_constants;

// Data Transfer Object
layout(location = 1) out struct dto {
	vec4 ambient;
    vec4 diffuse_color;
	vec2 tex_coord;
	vec3 normal;
    vec3 frag_position;
    vec4 tangent;
} out_dto;

void main() {
    mat3 model_vec = transpose(inverse(mat3(u_push_constants.model)));

    out_dto.frag_position = vec3(u_push_constants.model * vec4(in_position, 1.0));
	out_dto.tex_coord     = in_texcoord;
	out_dto.normal        = normalize(model_vec * in_normal);
	out_dto.ambient       = global_ubo.ambient_colour;
    out_dto.diffuse_color = u_push_constants.diffuse_color;
    out_dto.tangent       = vec4(normalize(model_vec * in_tangent.xyz), in_tangent.w);


    gl_Position = global_ubo.projection * global_ubo.view * u_push_constants.model * vec4(in_position, 1.0);
}
