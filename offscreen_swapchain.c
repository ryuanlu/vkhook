#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vulkan/vulkan.h>
#include "vkutils.h"

#ifndef SWAPINTERVAL_USEC
#define SWAPINTERVAL_USEC	(0)
#endif

struct offscreen_swapchain
{
	VkPhysicalDevice	phydevice;

	int		nr_images;
	VkImage*	images;
	VkDeviceMemory*	memories;

	VkFormat	format;
	int		width;
	int		height;

	int	index;
};


struct offscreen_swapchain* offscreen_swapchain_create(VkPhysicalDevice phydevice, VkDevice device, int nr_images, VkFormat format, int width, int height)
{
	int i;
	struct offscreen_swapchain* swapchain = NULL;

	swapchain = calloc(1, sizeof(struct offscreen_swapchain));

	swapchain->phydevice = phydevice;
	swapchain->nr_images = nr_images;
	swapchain->format = format;
	swapchain->width = width;
	swapchain->height = height;
	swapchain->images = calloc(nr_images, sizeof(VkImage));
	swapchain->memories = calloc(nr_images, sizeof(VkDeviceMemory));


	for(i = 0;i < nr_images;++i)
	{
		vkCreateImage
		(
			device,
			&(VkImageCreateInfo)
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.imageType = VK_IMAGE_TYPE_2D,
				.extent.width = width,
				.extent.height = height,
				.extent.depth = 1,
				.mipLevels = 1,
				.arrayLayers = 1,
				.format = format,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				.samples = VK_SAMPLE_COUNT_1_BIT,
			},
			NULL, &swapchain->images[i]
		);
		swapchain->memories[i] = vkimage_allocate_memory(swapchain->phydevice, device, swapchain->images[i], VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	return swapchain;
}


void offscreen_swapchain_destroy(VkDevice device, struct offscreen_swapchain* swapchain)
{
	int i;

	for(i = 0;i < swapchain->nr_images;++i)
	{
		vkFreeMemory(device, swapchain->memories[i], NULL);
		vkDestroyImage(device, swapchain->images[i], NULL);
	}

	free(swapchain->images);
	free(swapchain);
}


void offscreen_swapchain_get_images(struct offscreen_swapchain* swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages)
{
	*pSwapchainImageCount = swapchain->nr_images;
	if(pSwapchainImages)
		memcpy(pSwapchainImages, swapchain->images, sizeof(VkImage) * swapchain->nr_images);
}


int offscreen_swapchain_acquire_next_image(struct offscreen_swapchain* swapchain)
{
	return swapchain->index;
}


void offscreen_swapchain_present(struct offscreen_swapchain* swapchain)
{
	++swapchain->index;
	swapchain->index %= swapchain->nr_images;
	usleep(SWAPINTERVAL_USEC);
}