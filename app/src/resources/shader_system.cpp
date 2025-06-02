#include "containers/dhashtable.hpp"

#include "defines.hpp"
#include "math/dmath_types.hpp"
#include "renderer/vulkan/vulkan_backend.hpp"
#include "resources/resource_types.hpp"

#include "shader_system.hpp"

struct shader_system_state
{
    darray<dstring>    loaded_shaders;
    dhashtable<shader> hashtable;
    u64                default_shader_id = INVALID_ID_64;
};
static shader_system_state *shader_sys_state_ptr = nullptr;

static bool shader_system_create_default_shader(shader *out_shader, u64 *out_shader_id);
static void shader_system_add_uniform(shader_config *out_config, const char *name, shader_stage stage,
                                      shader_scope scope, u32 set, u32 binding, darray<attribute_types> types);

bool shader_system_startup(u64 *shader_system_mem_requirements, void *state)
{
    *shader_system_mem_requirements = sizeof(shader_system_state);
    if (!state)
    {
        return true;
    }

    shader_sys_state_ptr = static_cast<shader_system_state *>(state);

    shader_sys_state_ptr->hashtable.c_init(MAX_SHADER_COUNT);
    shader_sys_state_ptr->loaded_shaders.c_init(MAX_SHADER_COUNT);

    return true;
}

bool shader_system_shutdown(void *state)
{
    if (!state)
    {
        DERROR("Provided shader system state ptr is nullptr");
        return false;
    }
    if (state != shader_sys_state_ptr)
    {
        DFATAL("Provided state ptr is not equal to the ptr which was provided when creating the shader system. Provide "
               "the correct ptr");
    }
    // TODO: release the shaders
    shader_sys_state_ptr->hashtable.clear();
    shader_sys_state_ptr->hashtable.~dhashtable();
    shader_sys_state_ptr->loaded_shaders.~darray();
    shader_sys_state_ptr = nullptr;
    return true;
}
static bool _create_shader(shader_config *config, shader *out_shader)
{
    if (!config || !out_shader)
    {
        DERROR("The parameters provied to _create_shader() were nullptr.");
        return false;
    }

    bool result = vulkan_initialize_shader(config, out_shader);
    DASSERT(result);

    u64 id = shader_sys_state_ptr->hashtable.insert(INVALID_ID_64, *out_shader);
    DASSERT(id != INVALID_ID_64);

    out_shader->id = id;

    return true;
}

bool shader_system_create_default_shader(shader *out_shader, u64 *out_shader_id)
{
    // TODO: should use a configuration file.
    DASSERT_MSG(out_shader, "out shader is nullptr.");
    DASSERT_MSG(out_shader_id, "out_shader_id is nullptr");

    shader_config default_shader_conf{};
    default_shader_conf.frag_spv_full_path = "../assets/shaders/default_shader.frag.spv";
    default_shader_conf.vert_spv_full_path = "../assets/shaders/default_shader.vert.spv";

    darray<shader_uniform_config> &uni_conf = default_shader_conf.uniforms;

    {
        darray<attribute_types> types{};
        types.push_back(MAT_4);
        types.push_back(MAT_4);
        shader_system_add_uniform(&default_shader_conf, "uniform_global_object", STAGE_VERTEX, SHADER_PER_FRAME_UNIFORM,
                                  0, 0, types);

        types.clear();
        types.push_back(VEC_3);
        types.push_back(VEC_3);
        shader_system_add_uniform(&default_shader_conf, "uniform_light_object", STAGE_VERTEX, SHADER_PER_FRAME_UNIFORM,
                                  0, 1, types);

        types.clear();
        types.push_back(SAMPLER_2D);
        shader_system_add_uniform(&default_shader_conf, "albedo_map", STAGE_VERTEX, SHADER_PER_GROUP_UNIFORM, 1, 0,
                                  types);

        types.clear();
        types.push_back(SAMPLER_2D);
        shader_system_add_uniform(&default_shader_conf, "alpha_map", STAGE_VERTEX, SHADER_PER_GROUP_UNIFORM, 1, 1,
                                  types);
    }

    default_shader_conf.attributes.resize(3);
    darray<shader_attribute_config> &att_conf = default_shader_conf.attributes;
    {
        att_conf[0].name     = "in_position";
        att_conf[0].location = 0;
        att_conf[0].type     = VEC_3;

        att_conf[1].name     = "in_normal";
        att_conf[1].location = 1;
        att_conf[1].type     = VEC_3;

        att_conf[2].name     = "in_tex_coord";
        att_conf[2].location = 2;
        att_conf[2].type     = VEC_2;
    }

    default_shader_conf.stages = static_cast<shader_stage>(STAGE_VERTEX | STAGE_FRAGMENT);

    default_shader_conf.has_per_frame  = true;
    default_shader_conf.has_per_group  = true;
    default_shader_conf.has_per_object = true;

    *out_shader_id                          = _create_shader(&default_shader_conf, out_shader);
    shader_sys_state_ptr->default_shader_id = *out_shader_id;
    return true;
}

// TODO: finish this
bool shader_system_create_shader(dstring *shader_config_file_path, shader *out_shader, u64 *out_shader_id)
{
    if (!shader_config_file_path || !out_shader || !out_shader_id)
    {
        DERROR("Provided shader parameters were nullptr.");
        return false;
    }
    // NOTE:  Maybe I should do some prelimenary file checking to see if the provided file path is correct and the file
    // exists.
    const char *prefix = "../assets/shaders/";
    dstring     config_file_full_path{};
    string_copy_format(config_file_full_path.string, "%s%s", 0, prefix, shader_config_file_path->string);

    return true;
}

shader *shader_system_get_default_shader()
{
    DASSERT(shader_sys_state_ptr);
    shader *out_shader = shader_sys_state_ptr->hashtable.find(shader_sys_state_ptr->default_shader_id);
    if (!out_shader)
    {
        DFATAL("Default shader not found but have default_id %llu", shader_sys_state_ptr->default_shader_id);
        return nullptr;
    }
    return out_shader;
}

bool shader_use(u64 shader_id)
{
    return true;
}

static void shader_system_add_uniform(shader_config *out_config, const char *name, shader_stage stage,
                                      shader_scope scope, u32 set, u32 binding, darray<attribute_types> types)
{
    DASSERT(out_config);

    shader_uniform_config conf{};

    DASSERT_MSG(set < 2, "You cannot have a set in local scope. You can have uniforms for only global scope (aka set "
                         "0) or for instance scope (aka set 1)")
    switch (scope)
    {
        // global
    case SHADER_PER_FRAME_UNIFORM: {
        DASSERT_MSG(set == 0, "Specified per_frame_uniform but provided set is not zero");
    }
    break;
        // local
    case SHADER_PER_GROUP_UNIFORM: {
        DASSERT_MSG(set == 1, "Specified shader_per_group_uniform but provided set is not one");
    }
    break;
        // object
    case SHADER_PER_OBJECT_UNIFORM: {
        DFATAL("Cannot have uniforms in object scope. Use push constants for it");
    }
    break;
    }

    conf.name    = name;
    conf.stage   = stage;
    conf.scope   = scope;
    conf.set     = set;
    conf.binding = binding;
    conf.types   = types;

    u32 size = types.size();
    DASSERT(size);
    u32 offset = 0;
    for (u32 i = 0; i < size; i++)
    {
        if (types[i] == VEC_3)
        {
            // For uniform buffers the vec_3 has to be aligned to a vec4 aka 16 bytes
            offset += 16;
        }
        else
        {
            offset += types[i];
        }
    }
    DASSERT(offset);
    if (scope == SHADER_PER_FRAME_UNIFORM)
    {
        out_config->per_frame_uniform_offsets.push_back(offset);
    }
    else if(scope == SHADER_PER_GROUP_UNIFORM)
    {
        out_config->per_group_uniform_offsets.push_back(offset);
    }
    out_config->uniforms.push_back(conf);
}

// HACK: this should be abstracted away to support more shaders
bool shader_system_update_per_frame(shader *shader, scene_global_uniform_buffer_object *scene_global,
                                    light_global_uniform_buffer_object *light_global)
{
    return false;
}
// HACK: this should be abstracted away to support more shaders
bool shader_system_update_per_group(shader *shader, u64 offset, void *data)
{
    return true;
}
bool shader_system_update_per_object(shader *shader, u64 offset, void *data)
{
    return true;
}
// might not need this
bool shader_system_bind_per_frame(shader *shader, u64 offset)
{

    return true;
}
bool shader_system_bind_per_group(shader *shader, u64 offset)
{
    return true;
}
bool shader_system_bind_per_object(shader *shader, u64 offset)
{
    return true;
}
