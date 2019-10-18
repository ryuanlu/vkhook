#ifndef __OFFSCREEN_SWAPCHAIN_H__
#define __OFFSCREEN_SWAPCHAIN_H__

#define OFFSCREEN_SWAPCHAIN_VKSURFACE_MAGIC	(0xFFFFFFFF)


typedef struct offscreen_swapchain offscreen_swapchain;

offscreen_swapchain*	offscreen_swapchain_create	(VkPhysicalDevice phydevice, VkDevice device, int nr_images, VkFormat format, int width, int height);
void			offscreen_swapchain_destroy	(VkDevice device, offscreen_swapchain* swapchain);

void	offscreen_swapchain_get_images		(offscreen_swapchain* swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages);
int	offscreen_swapchain_acquire_next_image	(offscreen_swapchain* swapchain);
void	offscreen_swapchain_present		(offscreen_swapchain* swapchain);

#endif /* __OFFSCREEN_SWAPCHAIN_H__ */