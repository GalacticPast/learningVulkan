#include "containers/dhashtable.hpp"

#include "renderer/vulkan/vulkan_backend.hpp"

#include "shader_system.hpp"

struct shader_system_state
{
    darray<dstring> loaded_shaders;
    dhashtable<shader> hashtable;
};
static shader_system_state* shader_sys_state = nullptr;

bool shader_system_startup(u64* shader_system_mem_requirements, void* state)
{
    *shader_system_mem_requirements = sizeof(shader_system_state);
    if(!state)
    {
        return true;
    }

    shader_sys_state = static_cast<shader_system_state *>(state);

    shader_sys_state->hashtable.c_init(MAX_SHADER_COUNT);
    shader_sys_state->loaded_shaders.c_init(MAX_SHADER_COUNT);

    return true;
}


bool shader_system_shutdown(void* state)
{
    if(!state)
    {
        DERROR("Provided shader system state ptr is nullptr");
        return false;
    }
    if(state != shader_sys_state)
    {
        DFATAL("Provided state ptr is not equal to the ptr which was provided when creating the shader system. Provide the correct ptr");
    }
    // TODO: release the shaders
    shader_sys_state->hashtable.clear();
    shader_sys_state->hashtable.~dhashtable();
    shader_sys_state->loaded_shaders.~darray();
    shader_sys_state = nullptr;
    return true;
}

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

    bool result = vulkan_create_shader(shader_config_file_path, out_shader);

    return true;
}
