#version 450
#extension GL_EXT_debug_printf : enable

//light
layout(set = 0, binding = 1)uniform uniform_light_object
{
    vec3 position;
    vec3 color;
    vec3 camera_pos;
}ulo;

layout(set = 1, binding = 0) uniform sampler2D albedo_map;
layout(set = 1, binding = 1) uniform sampler2D alpha_map;
layout(set = 1, binding = 2) uniform sampler2D normal_map;

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

    //specular ligthing
    float specular_strength = 0.5;

    vec3 view_dir = normalize(ulo.camera_pos - frag_position);
    vec3 reflect_dir = reflect(-norm_light_dir,frag_normal);
    float spec_amount =  pow(max(dot(view_dir, reflect_dir),0.0),32);

    float specular = specular_strength * spec_amount;

    if(dis < radius)
    {
        diffuse = 0.5f;
        ambient = 0.5f;
        specular = 0.0f;
    }

    vec3 result = (ambient + diffuse + specular) * ulo.color;
    out_color = texture(albedo_map, frag_tex_coord) * vec4(result, 1.0f);
}
