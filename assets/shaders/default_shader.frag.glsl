#version 450
#extension GL_EXT_debug_printf : enable

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

float radius = 1.0f;

void main() {

    // ambient lighting
	float ambient = 0.10f;

	// diffuse lighting
	vec3 normal = normalize(frag_normal);
	vec3 light_dir = ulo.position - frag_position;

    float dis = length(light_dir);

    vec3 norm_light_dir = normalize(light_dir);

	float diffuse = max(dot(frag_normal, norm_light_dir), 0.0f);


    if(dis < radius)
    {
        diffuse = 0.5f;
        ambient = 0.5f;
    }


    out_color = texture(albedo_map, frag_tex_coord) * vec4(ulo.color * (diffuse + ambient), 1.0f);
}
