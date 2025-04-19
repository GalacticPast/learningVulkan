#include "vulkan_image.hpp"
#include "vulkan_device.hpp"

bool vulkan_create_image(vulkan_context *vk_context, vulkan_image *out_image, u32 img_width, u32 img_height,
                         VkFormat img_format, VkMemoryPropertyFlags mem_properties_flags, VkImageUsageFlags img_usage,
                         VkImageTiling img_tiling)
{
    VkDevice &device = vk_context->vk_device.logical;

    VkImageCreateInfo image_create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image_create_info.imageType         = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width      = img_width;
    image_create_info.extent.height     = img_height;
    image_create_info.extent.depth      = 1; // TODO: Support configurable depth.
    image_create_info.mipLevels         = 4; // TODO: Support mip mapping
    image_create_info.arrayLayers       = 1; // TODO: Support number of layers in the image.
    image_create_info.format            = img_format;
    image_create_info.tiling            = img_tiling;
    image_create_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage             = img_usage;
    image_create_info.samples           = VK_SAMPLE_COUNT_1_BIT;     // TODO: Configurable sample count.
    image_create_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE; // TODO: Configurable sharing mode.

    VkResult result = vkCreateImage(device, &image_create_info, vk_context->vk_allocator, &out_image->handle);
    VK_CHECK(result);

    VkMemoryRequirements img_mem_requirements;
    vkGetImageMemoryRequirements(device, out_image->handle, &img_mem_requirements);

    VkMemoryAllocateInfo mem_alloc_info{};
    mem_alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc_info.allocationSize = img_mem_requirements.size;
    mem_alloc_info.memoryTypeIndex =
        vulkan_find_memory_type_index(vk_context, img_mem_requirements.memoryTypeBits, mem_properties_flags);

    result = vkAllocateMemory(device, &mem_alloc_info, nullptr, &out_image->memory);
    VK_CHECK(result);

    vkBindImageMemory(device, out_image->handle, out_image->memory, 0);
    return true;
}

bool vulkan_create_image_view(vulkan_context *vk_context, VkImage image, VkFormat format, VkImageView *out_view,
                              VkImageAspectFlags img_aspect_flags)
{
    VkImageViewCreateInfo image_view_create_info{};

    image_view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext                           = 0;
    image_view_create_info.flags                           = 0;
    image_view_create_info.image                           = image;
    image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format                          = format;
    image_view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask     = img_aspect_flags;
    image_view_create_info.subresourceRange.baseMipLevel   = 0;
    image_view_create_info.subresourceRange.levelCount     = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount     = 1;

    VkResult result =
        vkCreateImageView(vk_context->vk_device.logical, &image_view_create_info, vk_context->vk_allocator, out_view);
    VK_CHECK(result);
    return true;
}
bool vulkan_destroy_image(vulkan_context *vk_context, vulkan_image *image)
{
    VkDevice              &device    = vk_context->vk_device.logical;
    VkAllocationCallbacks *allocator = vk_context->vk_allocator;

    vkDestroyImage(device, image->handle, allocator);
    vkDestroyImageView(device, image->view, allocator);
    vkFreeMemory(device, image->memory, allocator);

    image->handle = 0;
    image->view   = 0;
    image->memory = 0;
    return true;
}
