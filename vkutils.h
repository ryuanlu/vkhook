#ifndef __VKUTILS_H__
#define __VKUTILS_H__

#include <vulkan/vulkan.h>

VkDeviceMemory vkimage_allocate_memory(VkPhysicalDevice phydevice, VkDevice device, VkImage image, VkMemoryPropertyFlags flags);

#endif /* __VKUTILS_H__ */