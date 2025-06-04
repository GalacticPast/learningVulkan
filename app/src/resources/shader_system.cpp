#include "containers/dhashtable.hpp"

#include "core/dasserts.hpp"
#include "core/dfile_system.hpp"
#include "core/dmemory.hpp"
#include "core/dstring.hpp"
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

static void shader_system_add_uniform(shader_config *out_config, dstring *name, shader_stage stage, shader_scope scope,
                                      u32 set, u32 binding, attribute_types type);
static void shader_system_add_attributes(shader_config *out_config, const char *name, u32 location,
                                         attribute_types type);

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
static void shader_system_calculate_offsets(shader_config *out_config)
{
    DASSERT(out_config);
    u32                            size    = out_config->uniforms.size();
    darray<shader_uniform_config> &configs = out_config->uniforms;

    u32 offset = 0;

    for (u32 i = 0; i < size; i++)
    {
        if (configs[i].scope == SHADER_PER_FRAME_UNIFORM)
        {
            u32 num_types = configs[i].types_count;
            for (u32 j = 0; j < num_types; j++)
            {
                if (configs[i].types[j] == VEC_3)
                {
                    offset += VEC_4;
                }
                else
                {
                    offset += configs[i].types[j];
                }
            }
            out_config->per_frame_uniform_offsets.push_back(offset);
        }
        else if (configs[i].scope == SHADER_PER_GROUP_UNIFORM)
        {
            u32 num_types = configs[i].types_count;
            for (u32 j = 0; j < num_types; j++)
            {
                if (configs[i].types[j] == VEC_3)
                {
                    offset += VEC_4;
                }
                else
                {
                    offset += configs[i].types[j];
                }
            }
            out_config->per_group_uniform_offsets.push_back(offset);
        }
    }
}

// TODO: Should use a dedicated parser to parse this
static void shader_parse_configuration_file(dstring conf_file_name, shader_config *out_config)
{
    DASSERT_MSG(out_config, "Parameters passed to shader_parse_configuration_file were nullptr");

    dstring     full_file_path{};
    const char *prefix = "../assets/shaders/";
    string_copy_format(full_file_path.string, "%s%s", 0, prefix, conf_file_name.c_str());

    std::ifstream file;
    bool          result = file_open(full_file_path, &file, false);
    DASSERT(result);

    // NOTE:  this is literraly an adhoc aproach. So future me learn to write a proper parser :)

    // LOL this is such a smooth brained operation.
    bool has_vertex   = false;
    bool has_fragment = false;

    auto go_to_colon = [](const char *line) -> const char * {
        u32 index = 0;
        while (line[index] != ':')
        {
            if (line[index] == '\n' || line[index] == '\0')
            {
                return nullptr;
            }
            index++;
        }
        return line + index + 1;
    };
    auto translate_shader_scope_type = [](dstring string) -> shader_scope {
        const char *str = string.c_str();
        if (string_compare(str, "per_frame"))
        {
            return SHADER_PER_FRAME_UNIFORM;
        }
        else if (string_compare(str, "per_group"))
        {
            return SHADER_PER_GROUP_UNIFORM;
        }
        else if (string_compare(str, "per_object"))
        {
            return SHADER_PER_OBJECT_UNIFORM;
        }

        return SHADER_SCOPE_UNKNOWN;
    };

    auto translate_shader_stage_type = [](dstring string) -> shader_stage {
        const char *str = string.c_str();

        if (string_compare(str, "vertex"))
        {
            return STAGE_VERTEX;
        }
        else if (string_compare(str, "fragment"))
        {
            return STAGE_FRAGMENT;
        }
        return STAGE_UNKNOWN;
    };

    auto translate_attr_type = [](dstring string) -> attribute_types {
        const char *str = string.c_str();

        if (string_compare(str, "vec3"))
        {
            return VEC_3;
        }
        else if (string_compare(str, "vec4"))
        {
            return VEC_4;
        }
        else if (string_compare(str, "vec2"))
        {
            return VEC_2;
        }
        else if (string_compare(str, "mat4"))
        {
            return MAT_4;
        }
        else if (string_compare(str, "vec4"))
        {
            return VEC_4;
        }
        else if (string_compare(str, "sampler2D"))
        {
            return SAMPLER_2D;
        }
        return MAX_SHADER_ATTRIBUTE_TYPE;
    };

    dstring line;
    u32     num_stages = 0;
    while (file_get_line(file, &line))
    {
        // INFO: if comment skip line
        if (line[0] == '#' || line[0] == '\0')
        {
            continue;
        }
        dstring identifier;

        u32 index = 0;
        u32 j     = 0;

        while (line[index] != ':' && line[index] != '\n')
        {
            if (line[index] == ' ')
            {
                index++;
                continue;
            }
            identifier[j++] = line[index++];
        }

        if (string_compare(identifier.c_str(), "stages"))
        {
            darray<dstring> stages;

            const char *str = go_to_colon(line.c_str());
            DASSERT(str);
            dstring string = str;

            string_split(&string, ',', &stages);
            num_stages = stages.size();

            for (u32 i = 0; i < num_stages; i++)
            {
                if (string_compare(stages[i].c_str(), "vertex"))
                {
                    out_config->stages.push_back(STAGE_VERTEX);
                }
                else if (string_compare(stages[i].c_str(), "fragment"))
                {
                    out_config->stages.push_back(STAGE_FRAGMENT);
                }
                else
                {
                    DERROR("Unsupported stage %s", stages[i].c_str());
                }
            }
        }
        else if (string_compare(identifier.c_str(), "filepaths"))
        {
            darray<dstring> filepaths;

            const char *str = go_to_colon(line.c_str());
            DASSERT(str);

            dstring string = str;

            string_split(&string, ',', &filepaths);
            u32 num_filepaths = filepaths.size();
            DASSERT_MSG(num_filepaths == num_stages, "There should be a corresponding filepath for each stages");
            for (u32 i = 0; i < num_stages; i++)
            {
                if (string_num_of_substring_occurence(filepaths[i].c_str(), "vert"))
                {
                    out_config->vert_spv_full_path = filepaths[i];
                }
                else if (string_num_of_substring_occurence(filepaths[i].c_str(), "frag"))
                {
                    out_config->frag_spv_full_path = filepaths[i];
                }
                else
                {
                    DERROR("Unsupported filepath %s", filepaths[i].c_str());
                }
            }
        }
        else if (string_compare(identifier.c_str(), "has_per_frame"))
        {
            out_config->has_per_frame = true;
        }
        else if (string_compare(identifier.c_str(), "has_per_group"))
        {
            out_config->has_per_group = true;
        }
        else if (string_compare(identifier.c_str(), "has_per_object"))
        {
            out_config->has_per_object = true;
        }
        else if (string_compare(identifier.c_str(), "attribute"))
        {
            darray<dstring> list;
            const char     *str = go_to_colon(line.c_str());
            DASSERT(str);

            dstring string = str;

            string_split(&string, ',', &list);
            u32 size = list.size();
            DASSERT_MSG(size == 3, "There should be a name for the attribute, location for it , and a type.");

            u32             location = list[1].string[0] - '0';
            attribute_types type     = translate_attr_type(list[2]);
            shader_system_add_attributes(out_config, list[0].c_str(), location, type);
        }
        else if (string_compare(identifier.c_str(), "uniform"))
        {
            darray<dstring> list;
            const char     *str = go_to_colon(line.c_str());
            DASSERT(str);

            dstring string = str;
            string_split(&string, ',', &list);

            u32 size = list.size();
            DASSERT_MSG(size == 6, "Uniforms must have 6 identifiers associated with it");

            // NOTE: uniform_name,stage,scope,set,binding,type. Make sure there are 6 in total.
            //            0        1     2     3    4      5

            shader_uniform_config conf{};

            shader_stage stage = translate_shader_stage_type(list[1]);
            shader_scope scope = translate_shader_scope_type(list[2]);

            u32             set     = list[3].string[0] - '0';
            u32             binding = list[4].string[0] - '0';
            attribute_types type    = translate_attr_type(list[5]);

            shader_system_add_uniform(out_config, &list[0], stage, scope, set, binding, type);
        }
        else
        {
            DERROR("Unidentified identifier %s.", identifier.c_str());
            return;
        }
        line.clear();
    }
    shader_system_calculate_offsets(out_config);
}

bool shader_system_create_default_shader(shader *out_shader, u64 *out_shader_id)
{
    DASSERT_MSG(out_shader, "out shader is nullptr.");
    DASSERT_MSG(out_shader_id, "out_shader_id is nullptr");

    shader_config default_shader_conf{};

    dstring conf_file = "default_shader.conf";
    shader_parse_configuration_file(conf_file, &default_shader_conf);

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

static void shader_system_add_attributes(shader_config *out_config, const char *name, u32 location,
                                         attribute_types type)
{
    DASSERT(out_config);
    shader_attribute_config att_conf{};

    att_conf.name     = name;
    att_conf.location = location;
    att_conf.type     = type;
    out_config->attributes.push_back(att_conf);
    return;
}

static void shader_system_add_uniform(shader_config *out_config, dstring *name, shader_stage stage, shader_scope scope,
                                      u32 set, u32 binding, attribute_types type)
{
    DASSERT(out_config);

    shader_uniform_config conf{};

    DASSERT_MSG(stage != STAGE_UNKNOWN, "Specified shader stage is unknown");
    DASSERT_MSG(scope != SHADER_SCOPE_UNKNOWN,
                "Unknown shader scope. This could mean that there is a bug within the shader configuration parser.");
    DASSERT_MSG(type != MAX_SHADER_ATTRIBUTE_TYPE, "Unknown attribute type.");

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
        // TODO: do smth about push_constants
        return;
    }
    default: {
    }
    break;
    }
    // NOTE: Maybe we should'nt be doing this like this.
    u32                            size    = out_config->uniforms.size();
    darray<shader_uniform_config> &configs = out_config->uniforms;
    // check if the set is already in the array if it is update its types
    for (u32 i = 0; i < size; i++)
    {
        if (configs[i].set == set && configs[i].binding == binding && configs[i].stage == stage &&
            configs[i].scope == scope)
        {
            configs[i].types[configs[i].types_count] = type;
            configs[i].types_count++;
            return;
        }
    }

    conf.name        = name;
    conf.stage       = stage;
    conf.scope       = scope;
    conf.set         = set;
    conf.binding     = binding;
    conf.types_count = 1;
    dzero_memory(conf.types, MAX_UNIFORM_ATTRIBUTES * sizeof(attribute_types));
    conf.types[0] = type;

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
