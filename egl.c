#include <stdlib.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "gl_context.h"

struct egl_context
{
	EGLDisplay	egldisplay;
	EGLSurface	eglsurface;
	EGLConfig	eglconfig;
	EGLContext	eglcontext;
};


static struct egl_context* egl_context_create(Display* display, Window window)
{
	int nr_configs;
	struct egl_context* egl = NULL;

	egl = calloc(1, sizeof(struct egl_context));

	egl->egldisplay = eglGetPlatformDisplay(EGL_PLATFORM_X11_EXT, display, NULL);
	eglInitialize(egl->egldisplay, NULL, NULL);
	eglBindAPI(EGL_OPENGL_API);

	eglChooseConfig
	(
		egl->egldisplay,
		(EGLint[])
		{
			EGL_RENDERABLE_TYPE,	EGL_OPENGL_BIT,
			EGL_SURFACE_TYPE,	EGL_WINDOW_BIT,
			EGL_NONE,
		},
		&egl->eglconfig, 1, &nr_configs
	);

	egl->eglsurface = eglCreatePlatformWindowSurface(egl->egldisplay, egl->eglconfig, &window, NULL);

	egl->eglcontext = eglCreateContext
	(
		egl->egldisplay, egl->eglconfig, EGL_NO_CONTEXT,
		(EGLint[])
		{
			EGL_CONTEXT_MAJOR_VERSION_KHR, 4,
			EGL_CONTEXT_MINOR_VERSION_KHR, 5,
			EGL_NONE
		}
	);



	return egl;
}


static void egl_context_destroy(struct egl_context* egl)
{
	eglMakeCurrent(egl->egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(egl->egldisplay, egl->eglconfig);
	eglDestroySurface(egl->egldisplay, egl->eglsurface);
	eglTerminate(egl->egldisplay);
	free(egl);
}


static void egl_context_make_current(struct egl_context* egl)
{
	eglMakeCurrent(egl->egldisplay, egl->eglsurface, egl->eglsurface, egl->eglcontext);
}


static void egl_context_make_no_current(struct egl_context* egl)
{
	eglMakeCurrent(egl->egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}


static void egl_context_swap_buffers(struct egl_context* egl)
{
	eglSwapBuffers(egl->egldisplay, egl->eglsurface);
}


void gl_context_set_egl(struct gl_context* context)
{
	context->create_context		= (PFN_create_context)egl_context_create;
	context->make_current		= (PFN_make_current)egl_context_make_current;
	context->make_no_current	= (PFN_make_no_current)egl_context_make_no_current;
	context->swap_buffers		= (PFN_swap_buffers)egl_context_swap_buffers;
	context->destroy_context	= (PFN_destroy_context)egl_context_destroy;
}
