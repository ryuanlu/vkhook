#ifndef __GL_CONTEXT_H__
#define __GL_CONTEXT_H__


#include <X11/Xlib.h>

struct gl_context;

typedef void* (*PFN_create_context)(Display*, Window);
typedef void (*PFN_make_current)(void*);
typedef void (*PFN_make_no_current)(void*);
typedef void (*PFN_swap_buffers)(void*);
typedef void (*PFN_destroy_context)(void*);


struct gl_context
{
	PFN_create_context	create_context;
	PFN_make_current	make_current;
	PFN_make_no_current	make_no_current;
	PFN_swap_buffers	swap_buffers;
	PFN_destroy_context	destroy_context;
};

void gl_context_set_egl(struct gl_context* context);
void gl_context_set_glx(struct gl_context* context);


#endif /* __GL_CONTEXT_H__ */