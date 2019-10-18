#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "vkutils.h"

struct capture_context
{
	VkPhysicalDevice	phydevice;
	int			queuefamily;
	VkDevice		device;
	VkQueue			queue;
	VkCommandPool		cmdpool;
	VkCommandBuffer		cmdbuf;

	int		width;
	int		height;
	size_t		image_size;
	VkImage		image;
	VkFormat	format;
	VkDeviceMemory	memory;

};


struct capture_context* capture_context_init(VkPhysicalDevice phydevice, VkDevice device, int queuefamily)
{
	struct capture_context* capture = NULL;

	capture = calloc(1, sizeof(struct capture_context));

	capture->phydevice = phydevice;
	capture->device = device;
	capture->queuefamily = queuefamily;

	vkGetDeviceQueue(capture->device, capture->queuefamily, 0, &capture->queue);

	vkCreateCommandPool
	(
		capture->device,
		&(VkCommandPoolCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.queueFamilyIndex = capture->queuefamily,
		},
		NULL, &capture->cmdpool
	);

	vkAllocateCommandBuffers
	(
		capture->device,
		&(VkCommandBufferAllocateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandPool = capture->cmdpool,
			.commandBufferCount = 1,
		},
		&capture->cmdbuf
	);

	return capture;
}


void capture_context_deinit(struct capture_context* capture_context)
{
	vkFreeCommandBuffers(capture_context->device, capture_context->cmdpool, 1, &capture_context->cmdbuf);
	vkDestroyCommandPool(capture_context->device, capture_context->cmdpool, NULL);
	free(capture_context);
}


void capture_context_init_image(struct capture_context* capture_context, VkFormat format, int width, int height)
{
	vkCreateImage
	(
		capture_context->device,
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
			.tiling = VK_IMAGE_TILING_LINEAR,
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.samples = VK_SAMPLE_COUNT_1_BIT,
		},
		NULL, &capture_context->image
	);

	capture_context->width = width;
	capture_context->height = height;
	capture_context->format = format;
	capture_context->image_size = width * height * 4;

	capture_context->memory = vkimage_allocate_memory(capture_context->phydevice, capture_context->device, capture_context->image, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}


void capture_context_destroy_image(struct capture_context* capture_context)
{
	vkFreeMemory(capture_context->device, capture_context->memory, NULL);
	vkDestroyImage(capture_context->device, capture_context->image, NULL);
}


static void vkimage_pipeline_barrier(VkCommandBuffer cmdbuf, VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	vkCmdPipelineBarrier
	(
		cmdbuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
		&(VkImageMemoryBarrier)
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.image = image,
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.levelCount = 1,
			.subresourceRange.layerCount = 1,
			.srcAccessMask = srcAccessMask,
			.dstAccessMask = dstAccessMask,
			.oldLayout = oldLayout,
			.newLayout = newLayout,
			.subresourceRange = (VkImageSubresourceRange)
			{
				VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1,
			}
		}
	);
}


void capture_context_capture(struct capture_context* capture_context, VkImage source)
{
	vkBeginCommandBuffer
	(
		capture_context->cmdbuf,
		&(VkCommandBufferBeginInfo)
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		}
	);

	vkimage_pipeline_barrier
	(
		capture_context->cmdbuf, source,
		VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
	);

	vkimage_pipeline_barrier
	(
		capture_context->cmdbuf, capture_context->image,
		0, VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	vkCmdCopyImage
	(
		capture_context->cmdbuf, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, capture_context->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
		&(VkImageCopy)
		{
			.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.srcSubresource.layerCount = 1,
			.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.dstSubresource.layerCount = 1,
			.extent.width = capture_context->width,
			.extent.height = capture_context->height,
			.extent.depth = 1,
		}
	);

	vkimage_pipeline_barrier
	(
		capture_context->cmdbuf, source,
		VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	);

	vkimage_pipeline_barrier
	(
		capture_context->cmdbuf, capture_context->image,
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL
	);

	vkEndCommandBuffer(capture_context->cmdbuf);

	vkQueueSubmit
	(
		capture_context->queue, 1,
		&(VkSubmitInfo)
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pCommandBuffers = &capture_context->cmdbuf,
			.commandBufferCount = 1,
		},
		VK_NULL_HANDLE
	);

	vkQueueWaitIdle(capture_context->queue);
}


void capture_context_read_pixles(struct capture_context* capture_context, char* pixels)
{
	const char* ptr = NULL;
	VkSubresourceLayout layout = {0};

	vkGetImageSubresourceLayout
	(
		capture_context->device, capture_context->image,
		&(VkImageSubresource)
		{
			VK_IMAGE_ASPECT_COLOR_BIT, 0, 0
		},
		&layout
	);

	vkMapMemory(capture_context->device, capture_context->memory, 0, VK_WHOLE_SIZE, 0, (void**)&ptr);

	if(ptr && pixels)
		memcpy(pixels, ptr, capture_context->image_size);

	vkUnmapMemory(capture_context->device, capture_context->memory);
}


const char* capture_context_map_image(struct capture_context* capture_context)
{
	const char* ptr = NULL;
	vkMapMemory(capture_context->device, capture_context->memory, 0, VK_WHOLE_SIZE, 0, (void**)&ptr);
	return ptr;
}


void capture_context_unmap_image(struct capture_context* capture_context)
{
	vkUnmapMemory(capture_context->device, capture_context->memory);
}


VkImage	capture_context_get_vkimage(struct capture_context* capture_context)
{
	return capture_context->image;
}


VkPhysicalDevice capture_context_get_physical_device(struct capture_context* capture_context)
{
	return capture_context->phydevice;
}
