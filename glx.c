#include <stdlib.h>
#include <X11/Xlib.h>
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/glxext.h>

#include "gl_context.h"

struct glx_context
{
	GLXContext	glxcontext;
	Display*	display;
	Window		window;
};


static struct glx_context* glx_context_create(Display* display, Window window)
{
	int fb_attr[] =
	{
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DOUBLEBUFFER, 1,
		0,
	};

	int version_attr[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 3,
		0,
	};

	GLXFBConfig* fbconfig = NULL;
	struct glx_context* glx = NULL;
	int nitems;

	glx = calloc(1, sizeof(struct glx_context));

	fbconfig = glXChooseFBConfig(display, DefaultScreen(display), fb_attr, &nitems);
	glx->glxcontext = glXCreateContextAttribsARB(display, *fbconfig, 0, 1, version_attr);

	glx->display = display;
	glx->window = window;

	return glx;
}


static void glx_context_destroy(struct glx_context* glx)
{
	glXDestroyContext(glx->display, glx->glxcontext);
	free(glx);
}


static void glx_context_make_current(struct glx_context* glx)
{
	glXMakeCurrent(glx->display, glx->window, glx->glxcontext);
}


static void glx_context_make_no_current(struct glx_context* glx)
{
	glXMakeCurrent(glx->display, 0, 0);
}


static void glx_context_swap_buffers(struct glx_context* glx)
{
	glXSwapBuffers(glx->display, glx->window);
}


void gl_context_set_glx(struct gl_context* context)
{
	context->create_context		= (PFN_create_context)glx_context_create;
	context->make_current		= (PFN_make_current)glx_context_make_current;
	context->make_no_current	= (PFN_make_no_current)glx_context_make_no_current;
	context->swap_buffers		= (PFN_swap_buffers)glx_context_swap_buffers;
	context->destroy_context	= (PFN_destroy_context)glx_context_destroy;
}
