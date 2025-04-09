#include "vulkan_backend.hpp"
#include "core/dstring.hpp"
#include "core/logger.hpp"
#include "defines.hpp"
#include "vulkan_platform.hpp"
#include "vulkan_types.hpp"
#include <vector>

static vulkan_context *vulkan_context_ptr;

// VkBool32 vulkan_backend_debug_report_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char *pLayerPrefix,
//                                              const char *pMessage, void *pUserData);

bool vulkan_check_validation_layer_support();

bool vulkan_backend_initialize(u64 *vulkan_backend_memory_requirements, application_config *app_config, void *state)
{
    *vulkan_backend_memory_requirements = sizeof(vulkan_context);
    if (!state)
    {
        return true;
    }
    DDEBUG("Initializing Vulkan backend...");
    vulkan_context_ptr               = (vulkan_context *)state;
    vulkan_context_ptr->vk_allocator = nullptr;

    bool enable_validation_layers = false;
#ifdef DEBUG
    enable_validation_layers = true;
#endif

    DDEBUG("Creating vulkan instance...");

    VkApplicationInfo app_info{};

    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext              = nullptr;
    app_info.pApplicationName   = app_config->application_name;
    app_info.applicationVersion = 0;
    app_info.pEngineName        = 0;
    app_info.engineVersion      = 0;
    app_info.apiVersion         = 0;

    // INFO: callback info structure for vulkan.
    VkDebugReportCallbackCreateInfoEXT app_dbg_rprt_cb_create_info{};
    app_dbg_rprt_cb_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    app_dbg_rprt_cb_create_info.pNext = 0;
    app_dbg_rprt_cb_create_info.flags =
        VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
    // app_dbg_rprt_cb_create_info.pfnCallback = vulkan_backend_debug_report_callback;
    app_dbg_rprt_cb_create_info.pUserData = nullptr;

    VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info{};
    dbg_messenger_create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbg_messenger_create_info.pNext           = 0;
    dbg_messenger_create_info.flags           = 0;
    dbg_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbg_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                                            VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
    dbg_messenger_create_info.pfnUserCallback = 0;
    dbg_messenger_create_info.pUserData       = 0;

#ifdef DEBUG

    u32 extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

    // for (u32 i = 0; i < extension_count; i++)
    //{
    //     DDEBUG("Extension Name: %s,Extension Version :%u", extensions[i].extensionName, extensions[i].specVersion);
    // }

#endif
    bool validation_layer_support = vulkan_check_validation_layer_support();
    if (!validation_layer_support && enable_validation_layers)
    {
        DERROR("Validation layer support requsted, but not available.");
        return false;
    }
    u32 enabled_layer_count = 0;
    const char *enabled_layer_names[4];

    u32 index = 0;
    if (validation_layer_support)
    {
        enabled_layer_count++;
        enabled_layer_names[index++] = "VK_LAYER_KHRONOS_validation";
    }

    // INFO: get platform specifig extensions and extensions count
    u32 vulkan_extensions_count = 0;
    const char *vulkan_extensions[32];

    platform_get_required_vulkan_extensions(&vulkan_extensions_count, 0);
    platform_get_required_vulkan_extensions(&vulkan_extensions_count, vulkan_extensions);

    vulkan_extensions[vulkan_extensions_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    VkInstanceCreateInfo inst_create_info{};
    inst_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    // TODO: debug msg
    inst_create_info.pNext                   = 0;
    inst_create_info.flags                   = 0;
    inst_create_info.enabledLayerCount       = enabled_layer_count;
    inst_create_info.ppEnabledLayerNames     = enabled_layer_names;
    inst_create_info.enabledExtensionCount   = vulkan_extensions_count;
    inst_create_info.ppEnabledExtensionNames = vulkan_extensions;
    // TODO: end
    inst_create_info.pApplicationInfo = &app_info;

    VK_CHECK(vkCreateInstance(&inst_create_info, 0, &vulkan_context_ptr->vk_instance));

    return true;
}
void vulkan_backend_shutdown()
{
    DDEBUG("Shutting down vulkan...");
    vkDestroyInstance(vulkan_context_ptr->vk_instance, vulkan_context_ptr->vk_allocator);
}

// VkBool32 vulkan_backend_debug_report_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char *pLayerPrefix,
//                                             const char *pMessage, void *pUserData);

bool vulkan_check_validation_layer_support()
{
    u32 inst_layer_properties_count = 0;
    vkEnumerateInstanceLayerProperties(&inst_layer_properties_count, 0);

    std::vector<VkLayerProperties> inst_layer_properties(inst_layer_properties_count);
    vkEnumerateInstanceLayerProperties(&inst_layer_properties_count, inst_layer_properties.data());

    const char *validation_layer_name = "VK_LAYER_KHRONOS_validation";

    bool validation_layer_found = false;

    for (u32 i = 0; i < inst_layer_properties_count; i++)
    {
        if (string_compare(inst_layer_properties[i].layerName, validation_layer_name))
        {
            validation_layer_found = true;
            break;
        }
        DDEBUG("Layer Name: %s | Desc: %s", inst_layer_properties[i].layerName, inst_layer_properties[i].description);
    }
    return validation_layer_found;
}
