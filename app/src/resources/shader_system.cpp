#include "containers/dhashtable.hpp"

#include "defines.hpp"
#include "renderer/vulkan/vulkan_backend.hpp"

#include "shader_system.hpp"

struct shader_system_state
{
    darray<dstring> loaded_shaders;
    dhashtable<shader> hashtable;
    u64 default_shader_id  = INVALID_ID_64;
};
static shader_system_state* shader_sys_state_ptr = nullptr;

static bool shader_system_create_default_shader(shader* out_shader, u64* out_shader_id);

bool shader_system_startup(u64* shader_system_mem_requirements, void* state)
{
    *shader_system_mem_requirements = sizeof(shader_system_state);
    if(!state)
    {
        return true;
    }

    shader_sys_state_ptr = static_cast<shader_system_state *>(state);

    shader_sys_state_ptr->hashtable.c_init(MAX_SHADER_COUNT);
    shader_sys_state_ptr->loaded_shaders.c_init(MAX_SHADER_COUNT);

    shader default_shader{};
    u64 default_shader_id = INVALID_ID_64;
    shader_system_create_default_shader(&default_shader, &default_shader_id);

    return true;
}


bool shader_system_shutdown(void* state)
{
    if(!state)
    {
        DERROR("Provided shader system state ptr is nullptr");
        return false;
    }
    if(state != shader_sys_state_ptr)
    {
        DFATAL("Provided state ptr is not equal to the ptr which was provided when creating the shader system. Provide the correct ptr");
    }
    // TODO: release the shaders
    shader_sys_state_ptr->hashtable.clear();
    shader_sys_state_ptr->hashtable.~dhashtable();
    shader_sys_state_ptr->loaded_shaders.~darray();
    shader_sys_state_ptr = nullptr;
    return true;
}
static bool _create_shader(shader_config* config, shader* out_shader)
{
    if(!config || !out_shader)
    {
        DERROR("The parameters provied to _create_shader() were nullptr.");
        return false;
    }

    bool result = vulkan_create_shader(config, out_shader);
    DASSERT(result);

    u64 id = shader_sys_state_ptr->hashtable.insert(INVALID_ID_64, *out_shader);
    DASSERT(id != INVALID_ID_64);

    out_shader->id = id;

    return true;
}



static bool shader_system_create_default_shader(shader* out_shader, u64* out_shader_id)
{
    shader_config default_shader_conf{};
    default_shader_conf.frag_spv_full_path = "../assets/shaders/default_shader.frag.spv";
    default_shader_conf.vert_spv_full_path = "../assets/shaders/default_shader.vert.spv";

    *out_shader_id = _create_shader(&default_shader_conf, out_shader);
    shader_sys_state_ptr->default_shader_id = *out_shader_id;
    return true;
}

//TODO: finish this
bool shader_system_create_shader(dstring *shader_config_file_path, shader* out_shader, u64* out_shader_id)
{
    if(!shader_config_file_path || !out_shader || !out_shader_id)
    {
        DERROR("Provided shader parameters were nullptr.");
        return false;
    }
    //NOTE:  Maybe I should do some prelimenary file checking to see if the provided file path is correct and the file exists.
    const char* prefix = "../assets/shaders/";
    dstring config_file_full_path{};
    string_copy_format(config_file_full_path.string,"%s%s",0,prefix,shader_config_file_path->string);


    return true;
}

shader* shader_system_get_default_shader()
{
    DASSERT(shader_sys_state_ptr);
    shader* out_shader = shader_sys_state_ptr->hashtable.find(shader_sys_state_ptr->default_shader_id);
    if(!out_shader)
    {
        DFATAL("Default shader not found but have default_id %llu",shader_sys_state_ptr->default_shader_id);
        return nullptr;
    }
    return out_shader;
}
