#ifndef	__GLWINDOW_H__
#define	__GLWINDOW_H__

#include <vulkan/vulkan.h>

typedef struct glwindow	glwindow;

glwindow*	glwindow_create		(void);
void		glwindow_destroy	(glwindow* glwindow);
void		glwindow_set_fbo_size	(glwindow* glwindow, int width, int height);
void		glwindow_blit		(glwindow* glwindow, const char* pixels);
void		glwindow_vkimage_blit	(glwindow* glwindow, VkImage image);
void		glwindow_xputimage	(glwindow* glwindow, char* pixels);




#endif	/* __GLWINDOW_H__ */