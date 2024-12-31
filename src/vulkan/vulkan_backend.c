#include "vulkan_backend.h"
#include "containers/array.h"
#include "core/asserts.h"
#include "core/logger.h"
#include "core/strings.h"

VkBool32 debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_types,
                                  const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data);
void     debug_messenger_destroy(vulkan_context *context);

b8 initialize_vulkan(vulkan_context *context, const char *application_name)
{

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = 0;
    app_info.pApplicationName = application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No engine";
    app_info.engineVersion = VK_MAKE_VERSION(0, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;

    // INFO: vulkan layers
    char **required_layer_names = array_create(const char *);

    char *validation_layer_name = "VK_LAYER_KHRONOS_validation";
    required_layer_names = array_push_value(required_layer_names, &validation_layer_name);

    u32 layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, 0));
    VkLayerProperties *layer_properties = array_create_with_capacity(VkLayerProperties, layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, layer_properties));

#if _DEBUG
    INFO("Avaliable layers: %d", layer_count);
    for (u32 i = 0; i < layer_count; i++)
    {
        INFO("Layer Name:%s | Spec Version:%d | implementation version:%d | description:%s", layer_properties[i].layerName,
             layer_properties[i].specVersion, layer_properties[i].implementationVersion, layer_properties[i].description);
    }
#endif
    // INFO: to check if we have the required layer present
    u32 required_layer_count = (u32)array_get_length(required_layer_names);
    for (u32 i = 0; i < required_layer_count; i++)
    {
        b8 found = false;
        for (u32 j = 0; j < layer_count; j++)
        {
            if (string_compare(required_layer_names[i], layer_properties[j].layerName))
            {
                found = true;
            }
        }
        if (!found)
        {
            ERROR("Layer: %s required but not found.", layer_properties[i].layerName);
            return false;
        }
    }
    INFO("Required vulkan layers found.");
    //

    // INFO: vulkan extensions
    u32 extension_count = 0;

    char **required_extensions_name = array_create(const char *);
    char  *debug_utils = "VK_EXT_debug_utils";

    required_extensions_name = array_push_value(required_extensions_name, &debug_utils);
    u32 required_extensions_count = (u32)array_get_length(required_extensions_name);

    for (u32 i = 0; i < required_layer_count; i++)
    {
        VK_CHECK(vkEnumerateInstanceExtensionProperties(required_layer_names[i], &extension_count, 0));
        VkExtensionProperties *extension_properties = array_create_with_capacity(VkExtensionProperties, extension_count);
        VK_CHECK(vkEnumerateInstanceExtensionProperties(required_layer_names[i], &extension_count, extension_properties));
#if _DEBUG
        INFO("Layer Name:%s ", required_layer_names[i]);
        INFO("Avaliable extension layers: %d", extension_count);
        for (u32 j = 0; j < extension_count; j++)
        {
            INFO("Extension Name:%s | spec version:%d", extension_properties[j].extensionName, extension_properties[j].specVersion);
        }
#endif
        for (u32 j = 0; j < required_extensions_count; j++)
        {
            b8 found = false;
            for (u32 k = 0; k < extension_count; k++)
            {
                if (string_compare(required_extensions_name[j], extension_properties[k].extensionName))
                {
                    found = true;
                }
            }
            if (!found)
            {
                ERROR("Extension: %s required but not found.", required_extensions_name[i]);
                return false;
            }
        }
    }
    INFO("Required vulkan layer extensions found.");

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = 0;
    instance_create_info.flags = 0;
    instance_create_info.pApplicationInfo = &app_info;
#ifdef _DEBUG
    instance_create_info.enabledLayerCount = 1;
    instance_create_info.ppEnabledLayerNames = (const char *const *)required_layer_names;
    instance_create_info.enabledExtensionCount = required_extensions_count;
    instance_create_info.ppEnabledExtensionNames = (const char *const *)required_extensions_name;
#endif

    VK_CHECK(vkCreateInstance(&instance_create_info, 0, &context->instance));
    INFO("Vulkan instance created");
    // INFO: create debug messenger
#ifdef _DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = {};
    debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_messenger_create_info.pNext = 0;
    debug_messenger_create_info.flags = 0;
    debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_messenger_create_info.pfnUserCallback = debug_messenger_callback;
    debug_messenger_create_info.pUserData = 0;

    // INFO: get the fucntion address because its a extension function
    PFN_vkCreateDebugUtilsMessengerEXT create_debug_utils_messenger =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance, "vkCreateDebugUtilsMessengerEXT");

    if (!create_debug_utils_messenger)
    {
        ERROR("Unable to get the function pointer to PFN_vkCreateDebugUtilsMessengerEXT");
        return false;
    }
    VK_CHECK(create_debug_utils_messenger(context->instance, &debug_messenger_create_info, 0, &context->debug_messenger));
#endif

    return true;
}

VkBool32 debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_types,
                                  const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data)
{
    INFO("message recived");
    return VK_TRUE;
}

void debug_messenger_destroy(vulkan_context *context)
{
    PFN_vkDestroyDebugUtilsMessengerEXT destroy_debug_utils_messenger =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance, "vkDestroyDebugUtilsMessengerEXT");
    if (!destroy_debug_utils_messenger)
    {
        ERROR("Unable to get the function pointer to PFN_vkDestroyDebugUtilsMessengerEXT");
        return;
    }
    destroy_debug_utils_messenger(context->instance, context->debug_messenger, 0);
}

b8 shutdown_vulkan(vulkan_context *context)
{

    INFO("Destroying debug messenger...");
    // debug_messenger_destroy(context);
    INFO("Destroying vulkan instance...");
    // vkDestroyInstance(context->instance, 0);
    INFO("Destroyed vulkan instance");
    return true;
}
