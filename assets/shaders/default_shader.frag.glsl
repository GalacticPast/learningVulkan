#version 450
#extension GL_EXT_debug_printf : enable

layout(location = 0) out vec4 out_color;

//dir_light
layout(set = 0, binding = 1)uniform directional_light
{
    vec3 direction;
    vec4 color;
    vec3 camera_pos;
}dir_light;

layout(set = 1, binding = 0) uniform sampler2D diffuse_map;
layout(set = 1, binding = 1) uniform sampler2D normal_map;
layout(set = 1, binding = 2) uniform sampler2D specular_map;

// Data Transfer Object
layout(location = 1) in struct dto {
    vec4 ambient;
    vec4 diffuse_color;
    vec2 tex_coord;
    vec3 normal;
    vec3 frag_position;
    vec4 tangent;
} in_dto;

//TODO: transfer form cpu
struct point_light
{
    vec3  position;
    float radius;
    vec4  color;

    float constant;
    float linear;
    float quadratic;
};

vec4 calculate_directional_light(vec3 view_direction, vec3 normal);
vec4 calculate_point_light(vec3 view_direction, vec3 normal, point_light light);

void main()
{
    //TODO: transfer from cpu
    point_light one = point_light(
        vec3(2, 0, 0),           // Red light
        0.5,
        vec4(1.0, 0.0, 0.0, 1),
        1.0f,
        0.2f,
        0.1f
    );

    //TODO: transfer from cpu
    point_light two = point_light(
        vec3(0, 2, 0),           // Green light
        0.5,
        vec4(0.0, 1.0, 0.0, 1),
        1.0f,
        0.2f,
        0.1f
    );

    //TODO: transfer from cpu
    point_light three = point_light(
        vec3(0, 0, 2),           // Blue light
        0.5,
        vec4(0.0, 0.0, 1.0, 1),
        1.0f,
        0.2f,
        0.1f
    );



    vec3 normal = in_dto.normal;
    vec3 tangent = in_dto.tangent.xyz;
    tangent = normalize((tangent - dot(tangent, normal) *  normal));

    vec3 bitangent = cross(in_dto.normal, in_dto.tangent.xyz) * in_dto.tangent.w;
    mat3 TBN = mat3(tangent, bitangent, normal);

    vec3 local_normal = 2.0 * texture(normal_map, in_dto.tex_coord).rgb - 1.0;

    normal = normalize(TBN * local_normal);

    vec3 view_direction = normalize(dir_light.camera_pos - in_dto.frag_position);

    out_color = calculate_directional_light(view_direction, normal);
    out_color += calculate_point_light(view_direction, normal, one);
    out_color += calculate_point_light(view_direction, normal, two);
    out_color += calculate_point_light(view_direction, normal, three);
}

vec4 calculate_directional_light(vec3 view_direction, vec3 normal) {
    float diffuse_factor = max(dot(normal, -dir_light.direction), 0.0);

    vec3 half_direction = normalize(view_direction - dir_light.direction);
    float specular_factor = pow(max(dot(half_direction, normal), 0.0), 32);

    vec4 diff_samp = texture(diffuse_map, in_dto.tex_coord);

    vec4 ambient = vec4(vec3(in_dto.ambient * in_dto.diffuse_color), diff_samp.a);
    vec4 diffuse = vec4(vec3(dir_light.color * diffuse_factor), diff_samp.a);
    vec4 specular = vec4(vec3(dir_light.color * specular_factor), diff_samp.a);

    diffuse *= diff_samp;
    ambient *= diff_samp;
    specular *= vec4(texture(specular_map, in_dto.tex_coord).rgb, diffuse.a);

    return (ambient + diffuse + specular);
}

vec4 calculate_point_light(vec3 view_direction, vec3 normal, point_light light)
{
    vec3 light_direction =  normalize(light.position - in_dto.frag_position);
    float diff = max(dot(normal, light_direction), 0.0);

    vec3 reflect_direction = reflect(-light_direction, normal);
    float spec = pow(max(dot(view_direction, reflect_direction), 0.0), 32);

    // Calculate attenuation, or light falloff over distance.
    float distance = length(light.position - in_dto.frag_position);
    if(distance < light.radius)
    {
        return light.color;
    }
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec4 ambient = in_dto.ambient;
    vec4 diffuse = light.color * diff;
    vec4 specular = light.color * spec;

    vec4 diff_samp = texture(diffuse_map, in_dto.tex_coord);
    diffuse *= diff_samp;
    ambient *= diff_samp;
    specular *= vec4(texture(specular_map, in_dto.tex_coord).rgb, diffuse.a);

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

