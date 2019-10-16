#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <pthread.h>

#define DEFAULT_WINDOW_TITLE	"Vulkan to OpenGL"

struct glwindow
{
	Display*		xdisplay;
	Window			window;
	EGLDisplay		egldisplay;
	EGLSurface		eglsurface;
	EGLConfig		eglconfig;
	EGLContext		eglcontext;
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
	fprintf(stderr, "0x%X\n", eglGetError());

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


void glwindow_run(struct glwindow* glwindow)
{
	static float l = 0.0f;
	eglMakeCurrent(glwindow->egldisplay, glwindow->eglsurface, glwindow->eglsurface, glwindow->eglcontext);

	glClearColor(l, l, l, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	eglSwapBuffers(glwindow->egldisplay, glwindow->eglsurface);
	l += 0.01f;
	if(l > 1.0f)
		l = 0.0f;
	eglMakeCurrent(glwindow->egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}