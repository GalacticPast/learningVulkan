#include "material_system.hpp"
#include "texture_system.hpp"

#include "containers/darray.hpp"
#include "containers/dhashtable.hpp"

#include "core/dstring.hpp"

#include "resource_types.hpp"

#include "loaders/json_parser.hpp"

struct material_system_state
{
    darray<dstring>      loaded_materials;
    dhashtable<material> hashtable;
};

static material_system_state *mat_sys_state_ptr;

bool      material_system_create_material(material_config *config);
bool      material_system_create_default_material();
material *material_system_get_default_material();
bool      material_system_initialize(u64 *material_system_mem_requirements, void *state)

{
    *material_system_mem_requirements = sizeof(material_system_state);

    if (!state)
    {
        return true;
    }
    DINFO("Initializing material system...");
    mat_sys_state_ptr = (material_system_state *)state;

    {
        mat_sys_state_ptr->hashtable.table =
            (u64 *)dallocate(sizeof(material) * (MAX_MATERIALS_LOADED + 1), MEM_TAG_DHASHTABLE);
        mat_sys_state_ptr->hashtable.element_size          = sizeof(material);
        mat_sys_state_ptr->hashtable.max_length            = MAX_MATERIALS_LOADED;
        mat_sys_state_ptr->hashtable.num_elements_in_table = 0;
    }
    {
        darray<dstring> strings;
        mat_sys_state_ptr->loaded_materials = strings;
    }
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

    material *out_mat = nullptr;
    out_mat           = mat_sys_state_ptr->hashtable.find(base.mat_name);

    if (out_mat == nullptr)
    {
        DTRACE("Material:%s not loaded yet. Loading it...", file_base_name->c_str());
        material_system_create_material(&base);
    }
    out_mat = mat_sys_state_ptr->hashtable.find(base.mat_name);
    return out_mat;
};

material *material_system_acquire_from_config(material_config *config)
{

    material *out_mat = mat_sys_state_ptr->hashtable.find(config->mat_name);
    if (out_mat == nullptr)
    {
        DERROR("No Material by the name of %s. Maybe you havenet loaded it yet. Returning default Material",
               config->mat_name);
        out_mat = material_system_get_default_material();
    }
    return out_mat;
};

bool material_system_create_material(material_config *config)
{

    material mat{};
    mat.name            = DEFAULT_MATERIAL_HANDLE;
    mat.id              = mat_sys_state_ptr->loaded_materials.size();
    mat.reference_count = 0;
    mat.map.diffuse_tex = texture_system_get_texture(config->diffuse_tex_name);
    mat.diffuse_color   = config->diffuse_color;

    mat_sys_state_ptr->hashtable.insert(config->mat_name, mat);
    mat_sys_state_ptr->loaded_materials.push_back(mat.name);

    return true;
}

bool material_system_create_default_material()
{
    material default_mat{};
    default_mat.name            = DEFAULT_MATERIAL_HANDLE;
    default_mat.id              = 0;
    default_mat.reference_count = 0;
    default_mat.map.diffuse_tex = texture_system_get_default_texture();
    default_mat.diffuse_color   = {1.0f, 1.0f, 1.0f, 1.0f};

    mat_sys_state_ptr->hashtable.insert(DEFAULT_MATERIAL_HANDLE, default_mat);
    mat_sys_state_ptr->loaded_materials.push_back(default_mat.name);
    return true;
}

material *material_system_get_default_material()
{
    material *default_mat = mat_sys_state_ptr->hashtable.find(DEFAULT_MATERIAL_HANDLE);
    default_mat->reference_count++;
    return default_mat;
}

bool material_system_create_release_materials(dstring *mat_name)
{
    const char *material_name = mat_name->c_str();
    bool        result        = mat_sys_state_ptr->hashtable.erase(material_name);
    if (!result)
    {
        DERROR("Couldn't release material %s", material_name);
        return false;
    }
    return true;
}
