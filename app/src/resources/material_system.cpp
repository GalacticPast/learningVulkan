#include "material_system.hpp"
#include "containers/darray.hpp"
#include "containers/dhashtable.hpp"
#include "core/dfile_system.hpp"
#include "core/dmemory.hpp"
#include "core/logger.hpp"
#include "defines.hpp"

#include "texture_system.hpp"

#include "core/dstring.hpp"

#include "resource_types.hpp"

#include "loaders/json_parser.hpp"

struct material_system_state
{
    darray<dstring>      loaded_materials;
    dhashtable<material> hashtable;
    // material material_table[MAX_MATERIALS_LOADED];
};

static material_system_state *mat_sys_state_ptr;

bool material_system_create_material(material_config *config);
bool material_system_create_default_material();
bool material_system_initialize(u64 *material_system_mem_requirements, void *state)

{
    *material_system_mem_requirements = sizeof(material_system_state);

    if (!state)
    {
        return true;
    }
    DINFO("Initializing material system...");

    mat_sys_state_ptr = (material_system_state *)state;

    mat_sys_state_ptr->loaded_materials.c_init();
    mat_sys_state_ptr->hashtable.c_init(MAX_MATERIALS_LOADED);

    material_system_create_default_material();

    return true;
}
bool material_system_shutdown(void *state)
{
    u64 loaded_materials_count = MAX_MATERIALS_LOADED;

    mat_sys_state_ptr = nullptr;
    return true;
}

//// this will return nullptr;
// material *_get_material(const char *material_name)
//{
//     material *out_mat = nullptr;
//
//     for (u32 i = 0; i < MAX_MATERIALS_LOADED; i++)
//     {
//         const char *mat_name = (const char *)mat_sys_state_ptr->material_table[i].name.c_str();
//         bool        result   = string_compare(material_name, mat_name);
//         if (result)
//         {
//             out_mat = &mat_sys_state_ptr->material_table[i];
//         }
//     }
//     return out_mat;
// }
//  this will return nullptr;

// u32 _get_empty_id()
//{
//     for (u32 i = 0; i < MAX_MATERIALS_LOADED; i++)
//     {
//         if (mat_sys_state_ptr->material_table[i].id == INVALID_ID)
//         {
//             return i;
//         }
//     }
//     return INVALID_ID;
// }

material *material_system_acquire_from_config_file(dstring *file_base_name)
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
    mat.name            = config->mat_name;
    mat.id              = mat_sys_state_ptr->hashtable.size();
    mat.reference_count = 0;
    mat.map.diffuse_tex = texture_system_get_texture(config->diffuse_tex_name);
    mat.diffuse_color   = config->diffuse_color;

    mat_sys_state_ptr->hashtable.insert(config->mat_name, mat);
    mat_sys_state_ptr->loaded_materials.push_back(config->mat_name);

    DTRACE("Material %s created", config->mat_name);

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
    mat_sys_state_ptr->loaded_materials.push_back(DEFAULT_MATERIAL_HANDLE);

    return true;
}

material *material_system_get_default_material()
{
    material *default_mat = mat_sys_state_ptr->hashtable.find(DEFAULT_MATERIAL_HANDLE);
    default_mat->reference_count++;
    return default_mat;
}

bool material_system_release_materials(dstring *mat_name)
{
    const char *material_name = mat_name->c_str();

    bool found = mat_sys_state_ptr->hashtable.erase(material_name);

    if (!found)
    {
        DERROR("Couldn't find and release material %s. Have you made sure that you have loaded it correctly?",
               material_name);
        return false;
    }
    return true;
}
material *material_system_acquire_from_name(dstring *material_name)
{

    material *out_mat = mat_sys_state_ptr->hashtable.find(material_name->c_str());
    if (out_mat == nullptr)
    {
        DERROR("No Material by the name of %s. Maybe you havenet loaded it yet. Returning default Material",
               material_name->c_str());
        out_mat = material_system_get_default_material();
    }
    return out_mat;
}

bool material_system_parse_mtl_file(const char *mtl_file_name)
{
    material_config *configs = nullptr;

    char *file                         = nullptr;
    u64   file_buffer_mem_requirements = INVALID_ID_64;
    file_open_and_read(mtl_file_name, &file_buffer_mem_requirements, 0, 0);
    file = (char *)dallocate(file_buffer_mem_requirements + 1, MEM_TAG_RENDERER);
    file_open_and_read(mtl_file_name, &file_buffer_mem_requirements, file, 0);

    s32 num_materials = string_num_of_substring_occurence(file, "newmtl");
    if (num_materials < 0)
    {
        DERROR("No newmtl defined in %s file", mtl_file_name);
        return false;
    }
    configs = (material_config *)dallocate(sizeof(material_config) * (num_materials + 1), MEM_TAG_RENDERER);

    char *mtl_ptr                    = file;
    char  newmat_sub_string[7]       = "newmtl";
    u32   newmat_substring_size      = 7;
    char  diffuse_map_sub_string[7]  = "map_Kd";
    u32   diffuse_map_substring_size = 7;

    for (s32 i = 0; i < num_materials; i++)
    {
        u32 advance_ptr_by  = string_first_string_occurence(mtl_ptr, newmat_sub_string);
        mtl_ptr            += advance_ptr_by + newmat_substring_size;
        u32 next_material   = string_first_string_occurence(mtl_ptr, newmat_sub_string);

        u32 material_name_length = string_first_char_occurence(mtl_ptr, '\n');
        while (*mtl_ptr == ' ')
        {
            mtl_ptr++;
        }
        string_ncopy(configs[i].mat_name, mtl_ptr, material_name_length);

        u32 diffuse_tex_occurence = string_first_string_occurence(mtl_ptr, diffuse_map_sub_string);

        if (diffuse_tex_occurence < next_material)
        {
            mtl_ptr += diffuse_tex_occurence + diffuse_map_substring_size;

            while (*mtl_ptr == ' ')
            {
                mtl_ptr++;
            }
            u32   diffuse_map_name_size = string_first_char_occurence(mtl_ptr, '\n');
            char *diffuse_map_name      = mtl_ptr + diffuse_map_name_size - 1;
            diffuse_map_name_size       = 0;
            // TODO: will this suffice?
            while (*diffuse_map_name != '/' && *diffuse_map_name != ' ' && *diffuse_map_name != '\n')
            {
                diffuse_map_name--;
                diffuse_map_name_size++;
            }
            // it will break when its encouters a '/', ' ', or '\n'. So to not copy this part we increment the pointer
            // by one;
            diffuse_map_name++;

            if (diffuse_map_name_size == INVALID_ID)
            {
                DERROR("Coulndt get diffuse map name for material %s. Using default", configs[i].mat_name);
                continue;
            }
            string_ncopy(configs[i].diffuse_tex_name, diffuse_map_name, diffuse_map_name_size);
        }
        // TODO: Diffuse color and everything else
    }

    for (s32 i = 0; i < num_materials; i++)
    {
        material_system_create_material(&configs[i]);
    }
    return true;
}
