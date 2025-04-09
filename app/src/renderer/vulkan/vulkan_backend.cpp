#include "vulkan_backend.hpp"
#include "core/dstring.hpp"
#include "core/logger.hpp"
#include "defines.hpp"
#include "vulkan_platform.hpp"
#include "vulkan_types.hpp"
#include <vector>

static vulkan_context *vulkan_context_ptr;

VkBool32 vulkan_dbg_msg_rprt_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                      void *pUserData);

bool vulkan_check_validation_layer_support();
bool vulkan_create_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT *dbg_messenger_create_info);

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

    DDEBUG("Creating vulkan instance...");

    VkApplicationInfo app_info{};

    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext              = nullptr;
    app_info.pApplicationName   = app_config->application_name;
    app_info.applicationVersion = 0;
    app_info.pEngineName        = 0;
    app_info.engineVersion      = 0;
    app_info.apiVersion         = 0;

    bool enable_validation_layers = false;
    bool validation_layer_support = false;

#ifdef DEBUG

    enable_validation_layers = true;

    // vulkan debug messenger
    validation_layer_support = vulkan_check_validation_layer_support();
    if (!validation_layer_support && enable_validation_layers)
    {
        DERROR("Validation layer support requsted, but not available.");
        return false;
    }
#endif

    u32 enabled_layer_count = 0;
    const char *enabled_layer_names[4];

    u32 index = 0;
    if (enable_validation_layers && validation_layer_support)
    {
        enabled_layer_count++;
        enabled_layer_names[index++] = "VK_LAYER_KHRONOS_validation";
    }

    // INFO: get platform specifig extensions and extensions count
    u32 vulkan_extensions_count = 0;
    const char *vulkan_extensions[32];

    platform_get_required_vulkan_extensions(&vulkan_extensions_count, 0);
    platform_get_required_vulkan_extensions(&vulkan_extensions_count, vulkan_extensions);

#if DEBUG
    vulkan_extensions[vulkan_extensions_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#endif

    // Instance info
    VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info{};

    VkInstanceCreateInfo inst_create_info{};

    inst_create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_create_info.pNext                   = 0;
    inst_create_info.flags                   = 0;
    inst_create_info.enabledLayerCount       = enabled_layer_count;
    inst_create_info.ppEnabledLayerNames     = enabled_layer_names;
    inst_create_info.enabledExtensionCount   = vulkan_extensions_count;
    inst_create_info.ppEnabledExtensionNames = vulkan_extensions;
    inst_create_info.pApplicationInfo        = &app_info;

    VkResult result = vkCreateInstance(&inst_create_info, vulkan_context_ptr->vk_allocator, &vulkan_context_ptr->vk_instance);
    VK_CHECK(result);

    if (enable_validation_layers && validation_layer_support)
    {
        bool result = vulkan_create_debug_messenger(&dbg_messenger_create_info);
        if (!result)
        {
            DERROR("Couldn't setup Vulkan Create debug messenger.");
            return false;
        }
    }
    return true;
}

bool vulkan_create_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT *dbg_messenger_create_info)
{

    dbg_messenger_create_info->sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbg_messenger_create_info->pNext           = 0;
    dbg_messenger_create_info->flags           = 0;
    dbg_messenger_create_info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbg_messenger_create_info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
    dbg_messenger_create_info->pfnUserCallback = vulkan_dbg_msg_rprt_callback;
    dbg_messenger_create_info->pUserData       = nullptr;

    if (!vulkan_context_ptr->vk_instance)
    {
        DDEBUG("Populating vulkan_debug_utils_messenger struct");
        return true;
    }

    PFN_vkCreateDebugUtilsMessengerEXT func_create_dbg_utils_messenger_ext =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkan_context_ptr->vk_instance, "vkCreateDebugUtilsMessengerEXT");

    if (func_create_dbg_utils_messenger_ext == nullptr)
    {
        DERROR("Vulkan Create debug messenger ext called but fucn ptr to vkCreateDebugUtilsMessengerEXT is nullptr");
        return false;
    }
    if (vulkan_context_ptr->vk_instance)
    {
        VK_CHECK(func_create_dbg_utils_messenger_ext(vulkan_context_ptr->vk_instance, dbg_messenger_create_info, vulkan_context_ptr->vk_allocator, &vulkan_context_ptr->vk_dbg_messenger));
    }

    return true;
}

void vulkan_backend_shutdown()
{
    DDEBUG("Shutting down vulkan...");

#if DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT func_destroy_dbg_utils_messenger_ext =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkan_context_ptr->vk_instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func_destroy_dbg_utils_messenger_ext == nullptr)
    {
        DERROR("Vulkan destroy debug messenger ext called but fucn ptr to vkDestroyDebugUtilsMessengerEXT is nullptr");
    }
    else
    {
        func_destroy_dbg_utils_messenger_ext(vulkan_context_ptr->vk_instance, vulkan_context_ptr->vk_dbg_messenger, vulkan_context_ptr->vk_allocator);
    }
#endif
    vkDestroyInstance(vulkan_context_ptr->vk_instance, vulkan_context_ptr->vk_allocator);
}

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
        // DDEBUG("Layer Name: %s | Desc: %s", inst_layer_properties[i].layerName, inst_layer_properties[i].description);
    }
    return validation_layer_found;
}

VkBool32 vulkan_dbg_msg_rprt_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                      void *pUserData)
{
    DDEBUG("Vulkan validation layer: %s", pCallbackData->pMessage);
    return VK_TRUE;
}
