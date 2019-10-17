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

GLAPI void APIENTRY glDrawVkImageNV (GLuint64 vkImage, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1);

#define DEFAULT_WINDOW_TITLE	"Vulkan to OpenGL"

struct glwindow
{
	Display*		xdisplay;
	Window			window;
	EGLDisplay		egldisplay;
	EGLSurface		eglsurface;
	EGLConfig		eglconfig;
	EGLContext		eglcontext;

	GLuint			texture;
	GLuint			fbo;
	int 			width;
	int			height;
};



struct glwindow* glwindow_create(void)
{
	int nr_configs;
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

	glwindow->egldisplay = eglGetPlatformDisplay(EGL_PLATFORM_X11_EXT, glwindow->xdisplay, NULL);
	eglInitialize(glwindow->egldisplay, NULL, NULL);
	eglBindAPI(EGL_OPENGL_API);

	eglChooseConfig
	(
		glwindow->egldisplay,
		(EGLint[])
		{
			EGL_RENDERABLE_TYPE,	EGL_OPENGL_BIT,
			EGL_SURFACE_TYPE,	EGL_WINDOW_BIT,
			EGL_NONE,
		},
		&glwindow->eglconfig, 1, &nr_configs
	);

	glwindow->eglsurface = eglCreatePlatformWindowSurface(glwindow->egldisplay, glwindow->eglconfig, &glwindow->window, NULL);

	glwindow->eglcontext = eglCreateContext
	(
		glwindow->egldisplay, glwindow->eglconfig, EGL_NO_CONTEXT,
		(EGLint[])
		{
			EGL_CONTEXT_MAJOR_VERSION_KHR, 4,
			EGL_CONTEXT_MINOR_VERSION_KHR, 5,
			EGL_NONE
		}
	);

	eglMakeCurrent(glwindow->egldisplay, glwindow->eglsurface, glwindow->eglsurface, glwindow->eglcontext);

	eglMakeCurrent(glwindow->egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	return glwindow;
}


void glwindow_destroy(struct glwindow* glwindow)
{
	eglMakeCurrent(glwindow->egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(glwindow->egldisplay, glwindow->eglconfig);
	eglDestroySurface(glwindow->egldisplay, glwindow->eglsurface);
	eglTerminate(glwindow->egldisplay);
	XUnmapWindow(glwindow->xdisplay, glwindow->window);
	XDestroyWindow(glwindow->xdisplay, glwindow->window);
	XCloseDisplay(glwindow->xdisplay);
	free(glwindow);
}


void glwindow_set_fbo_size(struct glwindow* glwindow, int width, int height)
{
	eglMakeCurrent(glwindow->egldisplay, glwindow->eglsurface, glwindow->eglsurface, glwindow->eglcontext);

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


	eglMakeCurrent(glwindow->egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

}


void glwindow_blit(struct glwindow* glwindow, const char* pixels)
{
#ifdef NO_RGBA_TO_BGRA_CONVERSION
	GLuint format = GL_RGBA;
#else
	GLuint format = GL_BGRA;
#endif
	eglMakeCurrent(glwindow->egldisplay, glwindow->eglsurface, glwindow->eglsurface, glwindow->eglcontext);

	glBindTexture(GL_TEXTURE_2D, glwindow->texture);
	if(pixels)
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, glwindow->width, glwindow->height, format, GL_UNSIGNED_BYTE, pixels);

	glClear(GL_COLOR_BUFFER_BIT);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, glwindow->fbo);

	glBlitFramebuffer(0, glwindow->height, glwindow->width, 0, 0, 0, glwindow->width, glwindow->height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	eglSwapBuffers(glwindow->egldisplay, glwindow->eglsurface);

	eglMakeCurrent(glwindow->egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}


void glwindow_vkimage_blit(struct glwindow* glwindow, VkImage image)
{
	eglMakeCurrent(glwindow->egldisplay, glwindow->eglsurface, glwindow->eglsurface, glwindow->eglcontext);
	glClear(GL_COLOR_BUFFER_BIT);

	glDrawVkImageNV((GLuint64)image, 0, 0, 0, glwindow->width, glwindow->height, 0, 0, 0, 1, 1);

	eglSwapBuffers(glwindow->egldisplay, glwindow->eglsurface);
	eglMakeCurrent(glwindow->egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}
