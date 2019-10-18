#include <vulkan/vulkan.h>

VkDeviceMemory vkimage_allocate_memory(VkPhysicalDevice phydevice, VkDevice device, VkImage image, VkMemoryPropertyFlags flags)
{
	int i, memtype;
	VkDeviceMemory	memory;

	VkMemoryRequirements			memreq;
	VkPhysicalDeviceMemoryProperties	memprop;

	vkGetImageMemoryRequirements(device, image, &memreq);
	vkGetPhysicalDeviceMemoryProperties(phydevice, &memprop);

	for(i = 0;i < memprop.memoryTypeCount;i++)
	{
		if(memreq.memoryTypeBits & (1 << i) && (memprop.memoryTypes[i].propertyFlags & flags) == flags)
		{
			memtype = i;
			break;
		}
	}

	vkAllocateMemory
	(
		device,
		&(VkMemoryAllocateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memreq.size,
			.memoryTypeIndex = memtype,
		},
		NULL, &memory
	);

	vkBindImageMemory(device, image, memory, 0);

	return memory;
}
