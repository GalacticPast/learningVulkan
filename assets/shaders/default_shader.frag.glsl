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
layout(set = 1, binding = 3) uniform sampler2D specular_map;

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec2 frag_tex_coord;
layout(location = 3) in vec3 frag_position;

layout(location = 0) out vec4 out_color;

float RADIUS = 1.0f;


vec4 point_light()
{
	// used in two variables so I calculate it here to not have to do it twice
	vec3 light_vector = ulo.position - frag_position;

	// intensity of light with respect to distance
	float dist = length(light_vector);
	float a = 3.0;
	float b = 0.7;
	float inten = 1.0f / (a * dist * dist + b * dist + 1.0f);

	// ambient lighting
	float ambient = 0.20f;

	// diffuse lighting
	vec3 normal = normalize(frag_normal);
	vec3 light_dir = normalize(light_vector);
	float diffuse = max(dot(normal, light_dir), 0.0f);

	// specular lighting
	float specular_light = 0.50f;
	vec3 view_dir = normalize(ulo.camera_pos - frag_position);
	vec3 reflection_direction = reflect(-light_dir, normal);
	float spec_amount = pow(max(dot(view_dir, reflection_direction), 0.0f), 16);
	float specular = spec_amount * specular_light;

    if(dist < RADIUS)
    {
        diffuse = 0.5;
        specular = 0.0f;
        inten = 1;
    }

	return (texture(albedo_map, frag_tex_coord) * (diffuse * inten + ambient) + texture(specular_map, frag_tex_coord).r * specular * inten) * vec4(ulo.color, 1.0);
}

vec4 direct_light()
{
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

    if(dis < RADIUS)
    {
        diffuse = 0.5f;
        ambient = 0.5f;
        specular = 0.0f;
    }

    specular = texture(specular_map, frag_tex_coord).r * specular;
    vec4 result = (ambient + diffuse ) * texture(albedo_map, frag_tex_coord) + specular ;
    return result * vec4(ulo.color, 0.0f);
}

vec4 spot_light()
{
	// controls how big the area that is lit up is
	float outerCone = 0.90f;
	float innerCone = 0.95f;

	// ambient lighting
	float ambient = 0.20f;

	// diffuse lighting
	vec3 normal = normalize(frag_normal);
    float light_dis = length(ulo.position - frag_position);
	vec3 light_dir = normalize(ulo.position - frag_position);
	float diffuse = max(dot(normal, light_dir), 0.0f);

	// specular lighting
	float specular_light = 0.50f;
	vec3 view_dir = normalize(ulo.camera_pos - frag_position);
	vec3 reflection_direction = reflect(-light_dir, normal);
	float spec_amount = pow(max(dot(view_dir, reflection_direction), 0.0f), 16);
	float specular = spec_amount * specular_light;


	// calculates the intensity of the frag_position based on its angle to the center of the light cone
	float angle = dot(vec3(0.0f, -1.0f, 0.0f), -light_dir);
	float inten = clamp((angle - outerCone) / (innerCone - outerCone), 0.0f, 1.0f);

    if(light_dis < RADIUS)
    {
        diffuse = 0.5;
        inten = 1;
        specular = 0.0f;
    }
	return (texture(albedo_map, frag_tex_coord) * (diffuse * inten + ambient) + texture(specular_map, frag_tex_coord).r * specular * inten) * vec4(ulo.color,1.0);
}

void main() {
    out_color  = spot_light();
}
