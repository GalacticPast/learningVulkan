#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in vec4 in_tangent;

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 view;
    mat4 projection;
    vec4 ambient_color;
    vec3 camera_pos;
} global_ubo;

layout(push_constant) uniform push_constants {
    mat4 model;
    vec4 diffuse_color;
} u_push_constants;

layout(location = 0) out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 1) out struct dto {
    vec4 ambient;
    vec4 diffuse_color;
    vec2 tex_coord;
    vec3 normal;
    vec3 frag_position;
    vec4 tangent;
    vec3 camera_pos;
} out_dto;

void main() {

    out_dto.tex_coord = in_texcoord;
	// Fragment position in world space.
	out_dto.frag_position = vec3(u_push_constants.model * vec4(in_position, 1.0));
	// Copy the normal over.
	mat3 m3_model = mat3(u_push_constants.model);
	out_dto.normal = m3_model * in_normal;
	out_dto.tangent = vec4(normalize(m3_model * in_tangent.xyz), in_tangent.w);
    out_dto.ambient = global_ubo.ambient_color;
    out_dto.diffuse_color = u_push_constants.diffuse_color;
    out_dto.camera_pos = global_ubo.camera_pos;

    gl_Position = global_ubo.projection * global_ubo.view * u_push_constants.model * vec4(in_position, 1.0);
}
