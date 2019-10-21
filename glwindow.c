#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include <vulkan/vulkan.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <pthread.h>

#include "gl_context.h"

GLAPI void APIENTRY glDrawVkImageNV (GLuint64 vkImage, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1);

#define DEFAULT_WINDOW_TITLE	"Vulkan to OpenGL"

struct glwindow
{
	Display*	xdisplay;
	Window		window;

	void*		context;

	GLuint		texture;
	GLuint		fbo;
	int 		width;
	int		height;
};

struct gl_context gl = {0};

struct glwindow* glwindow_create(void)
{
	XTextProperty title;
	struct glwindow* glwindow = NULL;

	glwindow = calloc(1, sizeof(struct glwindow));
	XInitThreads();
	glwindow->xdisplay = XOpenDisplay(NULL);
	glwindow->window = XCreateWindow(glwindow->xdisplay, RootWindow(glwindow->xdisplay, 0), 0, 0, 1280, 720, 0,  CopyFromParent, CopyFromParent,  CopyFromParent, 0, NULL);

	XSetWindowBackground(glwindow->xdisplay, glwindow->window, WhitePixel(glwindow->xdisplay, 0));
	title.value = (unsigned char*)DEFAULT_WINDOW_TITLE;
	title.encoding = XA_STRING;
	title.format = 8;
	title.nitems = strlen(DEFAULT_WINDOW_TITLE);
	XSetWMProperties(glwindow->xdisplay, glwindow->window, &title, &title, NULL, 0, NULL, NULL, NULL);

	XSetWMProtocols
	(
		glwindow->xdisplay, glwindow->window,
		(Atom[])
		{
			XInternAtom(glwindow->xdisplay, "WM_DELETE_WINDOW", False),
		},
		1
	);

	XSelectInput(glwindow->xdisplay, glwindow->window, ExposureMask | KeyPressMask | StructureNotifyMask);
	XMapWindow(glwindow->xdisplay, glwindow->window);

#ifdef USE_EGL
	gl_context_set_egl(&gl);
#else
	gl_context_set_glx(&gl);
#endif

	glwindow->context = gl.create_context(glwindow->xdisplay, glwindow->window);

	gl.make_current(glwindow->context);
	gl.make_no_current(glwindow->context);

	return glwindow;
}


void glwindow_destroy(struct glwindow* glwindow)
{
	gl.destroy_context(glwindow->context);
	XUnmapWindow(glwindow->xdisplay, glwindow->window);
	XDestroyWindow(glwindow->xdisplay, glwindow->window);
	XCloseDisplay(glwindow->xdisplay);
	free(glwindow);
}


void glwindow_set_fbo_size(struct glwindow* glwindow, int width, int height)
{
	gl.make_current(glwindow->context);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	if(glwindow->texture)
		glDeleteTextures(1, &glwindow->texture);

	glwindow->width = width;
	glwindow->height = height;

	glGenTextures(1, &glwindow->texture);
	if(!glwindow->fbo)
		glCreateFramebuffers(1, &glwindow->fbo);

	glBindTexture(GL_TEXTURE_2D, glwindow->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, glwindow->fbo);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, glwindow->fbo);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glwindow->texture, 0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, glwindow->fbo);

	gl.make_no_current(glwindow->context);
}


void glwindow_blit(struct glwindow* glwindow, const char* pixels)
{
#ifdef NO_RGBA_TO_BGRA_CONVERSION
	GLuint format = GL_RGBA;
#else
	GLuint format = GL_BGRA;
#endif
	gl.make_current(glwindow->context);

	glBindTexture(GL_TEXTURE_2D, glwindow->texture);
	if(pixels)
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, glwindow->width, glwindow->height, format, GL_UNSIGNED_BYTE, pixels);

	glClear(GL_COLOR_BUFFER_BIT);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, glwindow->fbo);

	glBlitFramebuffer(0, glwindow->height, glwindow->width, 0, 0, 0, glwindow->width, glwindow->height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	gl.swap_buffers(glwindow->context);

	gl.make_no_current(glwindow->context);
}


void glwindow_vkimage_blit(struct glwindow* glwindow, VkImage image)
{
	gl.make_current(glwindow->context);
	glClear(GL_COLOR_BUFFER_BIT);

	glDrawVkImageNV((GLuint64)image, 0, 0, 0, glwindow->width, glwindow->height, 0, 0, 0, 1, 1);

	gl.swap_buffers(glwindow->context);
	gl.make_no_current(glwindow->context);
}
