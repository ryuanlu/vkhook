#ifndef __CAPTURE_H__
#define __CAPTURE_H__

#include <vulkan/vulkan.h>

typedef struct capture_context capture_context;

struct capture_context*	capture_context_init	(VkPhysicalDevice phydevice, VkDevice device, int queuefamily);
void			capture_context_deinit	(capture_context* capture_context);

void	capture_context_init_image	(capture_context* capture_context, VkFormat format, int width, int height);
void	capture_context_destroy_image	(capture_context* capture_context);

void	capture_context_capture		(capture_context* capture_context, VkImage source);
void	capture_context_read_pixles	(capture_context* capture_context, char* pixels);

const char*	capture_context_map_image	(capture_context* capture_context);
void		capture_context_unmap_image	(capture_context* capture_context);

VkImage	capture_context_get_vkimage	(capture_context* capture_context);

#endif /* __CAPTURE_H__ */
