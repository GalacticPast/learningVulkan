#include "material_system.hpp"
#include "containers/dhashtable.hpp"
#include "core/dstring.hpp"
#include "renderer/vulkan/vulkan_backend.hpp"
#include "resource_types.hpp"

#include "loaders/json_parser.hpp"

struct material_system_state
{
    darray<dstring>      loaded_materials;
    dhashtable<material> hashtable;
};

static material_system_state *mat_sys_state_ptr;

bool material_system_create_default_material();
bool material_system_initialize(u64 *material_system_mem_requirements, void *state)
{
    *material_system_mem_requirements = sizeof(material_system_state) + MAX_MATERIALS_LOADED * sizeof(material);

    if (!state)
    {
        return true;
    }
    DINFO("Initializing material system...");
    mat_sys_state_ptr = (material_system_state *)state;

    {
        mat_sys_state_ptr->hashtable.table                 = (u64 *)((u8 *)state + sizeof(material_system_state));
        mat_sys_state_ptr->hashtable.element_size          = sizeof(material);
        mat_sys_state_ptr->hashtable.max_length            = MAX_TEXTURES_LOADED;
        mat_sys_state_ptr->hashtable.num_elements_in_table = 0;
    }
    {
        darray<dstring> strings;
        mat_sys_state_ptr->loaded_materials = strings;
    }
    dstring name;
    name = DEFAULT_MATERIAL_HANDLE;

    material *mat = material_system_acquire_from_file(&name);
    return true;
}
bool material_system_shutdown(void *state)
{
    u64 loaded_materials_count = mat_sys_state_ptr->loaded_materials.size();
    for (u64 i = 0; i < loaded_materials_count; i++)
    {
        dstring *mat_name = &mat_sys_state_ptr->loaded_materials[i];
        // material_system_release_materials(tex_name);
    }

    mat_sys_state_ptr->loaded_materials.~darray();
    mat_sys_state_ptr = nullptr;

    return true;
}

material *material_system_acquire_from_file(dstring *file_base_name)
{
    material_config base{};
    const char     *prefix = "../assets/materials/";
    const char     *suffix = ".json";

    char path[128] = {};
    string_copy_format(path, "%s%s%s", 0, prefix, file_base_name->c_str(), suffix);
    dstring name;
    name = path;
    json_parse_material(&name, &base);

    return nullptr;
};

material *material_system_acquire_from_config(material_config *config)
{

    return nullptr;
};

material *material_system_acquire_by_id(u32 id)
{
    return nullptr;
}
