#include "containers/dhashtable.hpp"

#include "core/dasserts.hpp"
#include "core/dfile_system.hpp"
#include "core/dmemory.hpp"
#include "core/dstring.hpp"
#include "defines.hpp"
#include "math/dmath_types.hpp"
#include "renderer/renderer.hpp"
#include "renderer/vulkan/vulkan_backend.hpp"
#include "resources/resource_types.hpp"

#include "shader_system.hpp"

struct shader_system_state
{
    arena                    *arena = nullptr;
    darray<dstring>           loaded_shaders;
    dhashtable<shader>        shaders;
    dhashtable<shader_config> shader_configs;

    u64 default_material_shader_id = INVALID_ID_64;
    u64 default_skybox_shader_id   = INVALID_ID_64;
    u64 default_grid_shader_id     = INVALID_ID_64;
    u64 default_ui_shader_id       = INVALID_ID_64;
    u64 bound_shader_id            = INVALID_ID_64;
};
static shader_system_state *shader_sys_state_ptr = nullptr;

static void shader_system_add_uniform(shader_config *out_config, dstring *name, shader_stage stage, shader_scope scope,
                                      u32 set, u32 binding, attribute_types type);
static void shader_system_add_attributes(shader_config *out_config, const char *name, u32 location,
                                         attribute_types type);

bool shader_system_startup(arena *arena, u64 *shader_system_mem_requirements, void *state)
{
    *shader_system_mem_requirements = sizeof(shader_system_state);
    if (!state)
    {
        return true;
    }
    DASSERT(arena);

    shader_sys_state_ptr        = static_cast<shader_system_state *>(state);
    shader_sys_state_ptr->arena = arena;

    shader_sys_state_ptr->shaders.c_init(arena, MAX_SHADER_COUNT);
    shader_sys_state_ptr->shader_configs.c_init(arena, MAX_SHADER_COUNT);
    shader_sys_state_ptr->loaded_shaders.reserve(arena, MAX_SHADER_COUNT);

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
    shader_sys_state_ptr->shaders.clear();
    shader_sys_state_ptr->shaders.~dhashtable();
    shader_sys_state_ptr->shader_configs.clear();
    shader_sys_state_ptr->shader_configs.~dhashtable();
    shader_sys_state_ptr->loaded_shaders.~darray();
    shader_sys_state_ptr = nullptr;
    return true;
}

static u64 _create_shader(shader_config *config, shader *out_shader, shader_type type)
{
    if (!config || !out_shader)
    {
        DERROR("The parameters provied to _create_shader() were nullptr.");
        return false;
    }

    bool result = vulkan_initialize_shader(config, out_shader);
    DASSERT(result);

    out_shader->type = type;
    u64 id           = shader_sys_state_ptr->shaders.insert(INVALID_ID_64, *out_shader);
    DASSERT(id != INVALID_ID_64);

    u64 config_id = shader_sys_state_ptr->shader_configs.insert(id, *config);

    return id;
}

bool shader_system_bind_shader(u64 shader_id)
{
    DASSERT(shader_id != INVALID_ID_64);
    DASSERT(shader_sys_state_ptr);
    // TODO: check if the shader_id is valid or not.
    shader_sys_state_ptr->bound_shader_id = shader_id;
    return true;
}

shader *shader_system_get_shader(u64 shader_id)
{
    DASSERT(shader_id != INVALID_ID_64);
    DASSERT(shader_sys_state_ptr);
    shader *shader = shader_sys_state_ptr->shaders.find(shader_id);
    return shader;
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
            offset = 0;
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
            offset = 0;
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

    arena *arena = shader_sys_state_ptr->arena;

    std::fstream file;
    bool         result = file_open(full_file_path, &file, false, false);
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
        else if (string_compare(str, "sampler_cube"))
        {
            return SAMPLER_CUBE;
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
            stages.c_init(arena);

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
            filepaths.c_init(arena);

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
            list.c_init(arena);

            const char *str = go_to_colon(line.c_str());
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
            list.c_init(arena);

            const char *str = go_to_colon(line.c_str());
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

bool shader_system_create_default_shaders(u64 *material_shader_id, u64 *skybox_shader_id, u64 *grid_shader_id,
                                          u64 *ui_shader_id)
{
    arena *arena = shader_sys_state_ptr->arena;

    shader_config material_shader_conf{};
    material_shader_conf.pipeline_configuration.mode                           = CULL_BACK_BIT;
    material_shader_conf.pipeline_configuration.color_blend.enable_color_blend = false;
    material_shader_conf.renderpass_types                                      = WORLD_RENDERPASS;
    material_shader_conf.init(arena);

    dstring conf_file = "material_shader.conf";
    shader_parse_configuration_file(conf_file, &material_shader_conf);

    shader material_shader{};
    *material_shader_id = _create_shader(&material_shader_conf, &material_shader, SHADER_TYPE_MATERIAL);
    shader_sys_state_ptr->default_material_shader_id = *material_shader_id;

    shader_config skybox_shader_conf{};
    skybox_shader_conf.pipeline_configuration.mode                           = CULL_NONE_BIT;
    skybox_shader_conf.pipeline_configuration.color_blend.enable_color_blend = false;
    skybox_shader_conf.renderpass_types                                      = WORLD_RENDERPASS;
    skybox_shader_conf.init(arena);

    conf_file.clear();
    conf_file = "skybox_shader.conf";
    shader_parse_configuration_file(conf_file, &skybox_shader_conf);

    shader skybox_shader{};
    *skybox_shader_id = _create_shader(&skybox_shader_conf, &skybox_shader, SHADER_TYPE_SKYBOX);
    shader_sys_state_ptr->default_skybox_shader_id = *skybox_shader_id;

    shader_config grid_shader_conf{};
    grid_shader_conf.pipeline_configuration.mode                               = CULL_NONE_BIT;
    grid_shader_conf.pipeline_configuration.color_blend.enable_color_blend     = true;
    grid_shader_conf.pipeline_configuration.color_blend.src_color_blend_factor = COLOR_BLEND_FACTOR_SRC_ALPHA;
    grid_shader_conf.pipeline_configuration.color_blend.dst_color_blend_factor = COLOR_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    grid_shader_conf.renderpass_types = WORLD_RENDERPASS;

    grid_shader_conf.pipeline_configuration.depth_state.enable_depth_blend = false;

    grid_shader_conf.init(arena);

    conf_file.clear();
    conf_file = "grid_shader.conf";
    shader_parse_configuration_file(conf_file, &grid_shader_conf);

    shader grid_shader{};
    *grid_shader_id                              = _create_shader(&grid_shader_conf, &grid_shader, SHADER_TYPE_GRID);
    shader_sys_state_ptr->default_grid_shader_id = *grid_shader_id;

    shader_config ui_shader_conf{};
    ui_shader_conf.pipeline_configuration.mode                               = CULL_BACK_BIT;
    ui_shader_conf.pipeline_configuration.color_blend.enable_color_blend     = false;
    //ui_shader_conf.pipeline_configuration.color_blend.src_color_blend_factor = COLOR_BLEND_FACTOR_SRC_ALPHA;
    //ui_shader_conf.pipeline_configuration.color_blend.dst_color_blend_factor = COLOR_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    ui_shader_conf.renderpass_types                                      = UI_RENDERPASS;
    ui_shader_conf.pipeline_configuration.depth_state.enable_depth_blend = false;

    ui_shader_conf.init(arena);

    conf_file.clear();
    conf_file = "ui_shader.conf";
    shader_parse_configuration_file(conf_file, &ui_shader_conf);

    shader ui_shader{};
    *ui_shader_id                              = _create_shader(&ui_shader_conf, &ui_shader, SHADER_TYPE_GRID);
    shader_sys_state_ptr->default_ui_shader_id = *ui_shader_id;

    return true;
}

// TODO:
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

u64 shader_system_get_default_material_shader_id()
{
    DASSERT(shader_sys_state_ptr);
    return shader_sys_state_ptr->default_material_shader_id;
}
u64 shader_system_get_default_skybox_shader_id()
{
    DASSERT(shader_sys_state_ptr);
    return shader_sys_state_ptr->default_skybox_shader_id;
}
u64 shader_system_get_default_grid_shader_id()
{
    DASSERT(shader_sys_state_ptr);
    return shader_sys_state_ptr->default_grid_shader_id;
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

bool shader_system_update_per_frame(scene_global_uniform_buffer_object *scene_global,
                                    light_global_uniform_buffer_object *light_global)
{
    u64 bound_shader_id = shader_sys_state_ptr->bound_shader_id;
    DASSERT_MSG(bound_shader_id != INVALID_ID_64, "Shader bound id is invalid. Call shader bind before updating.");

    shader *shader = shader_sys_state_ptr->shaders.find(bound_shader_id);
    DASSERT(shader);
    shader_config *config = shader_sys_state_ptr->shader_configs.find(bound_shader_id);
    DASSERT(config);

    if (shader->type == SHADER_TYPE_MATERIAL)
    {
        if (!scene_global)
        {
            DFATAL("scene_global_uniform_buffer_object was nullptr.Material shader requires it.");
            return false;
        }
        if (!light_global)
        {
            DFATAL("light_global_uniform_buffer_object was nullptr.Material shader requires it.");
            return false;
        }
        u32  scene_offset = 0;
        bool scene = renderer_update_global_data(shader, 0, sizeof(scene_global_uniform_buffer_object), scene_global);
        DASSERT(scene);

        u32  light_offset = config->per_frame_uniform_offsets[0];
        bool light =
            renderer_update_global_data(shader, light_offset, sizeof(light_global_uniform_buffer_object), light_global);
        DASSERT(light);
    }
    else if (shader->type == SHADER_TYPE_SKYBOX || shader->type == SHADER_TYPE_GRID)
    {
        if (!scene_global)
        {
            DFATAL("scene_global_uniform_buffer_object was nullptr. Skybox_shader requires it");
            return false;
        }
        u32 size   = config->per_frame_uniform_offsets[0];
        // this is because we are not sending the ambient color to the skybox shader;
        bool scene = renderer_update_global_data(shader, 0, size, scene_global);
        DASSERT(scene);
    }
    else
    {
        DERROR("Unknown shader type %d", shader->type);
    }

    bool result = renderer_update_globals(shader, config->per_frame_uniform_offsets);
    DASSERT(result);

    return result;
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
