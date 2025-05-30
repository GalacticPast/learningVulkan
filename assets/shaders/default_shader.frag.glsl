#version 450

//light
layout(set = 0, binding = 1)uniform uniform_light_object
{
    vec3 position;
    vec3 color;
}ulo;

layout(set = 1, binding = 0) uniform sampler2D albedo_map;
layout(set = 1, binding = 1) uniform sampler2D alpha_map;

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec2 frag_tex_coord;
layout(location = 3) in vec3 frag_position;

layout(location = 0) out vec4 out_color;

void main() {
    vec3 norm = normalize(frag_normal);
    vec3 light_dir = normalize(ulo.position - frag_position);

    float diffuse_strength = max(dot(norm, light_dir),0.0);
    vec3 diffuse = ulo.color * diffuse_strength;

    float ambient_strength = 0.1f;
    vec3 ambient = ulo.color * ambient_strength;

    vec3 result = (ambient + diffuse) * frag_color;

    vec4 color = texture(albedo_map, frag_tex_coord);

    out_color = color;
}
