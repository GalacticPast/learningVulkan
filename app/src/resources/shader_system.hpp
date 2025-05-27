#include "core/dstring.hpp"
#include "resources/resource_types.hpp"

bool shader_system_startup(u64* shader_system_mem_requirements, void* state);
bool shader_system_shutdown(void* state);

bool shader_system_create_shader(dstring *shader_config_file_path, shader* out_shader, u64* out_shader_id);


