# Available config:
#
#	CONFIG_USE_OFFSCREEN_SWAPCHAIN
#	CONFIG_USE_GL_DRAW_VKIMAGE_NV
#	CONFIG_USE_XPUTIMAGE
#	CONFIG_USE_XSHMPUTIMAGE
#

CONFIG:=CONFIG_USE_OFFSCREEN_SWAPCHAIN

include build.mk

.SUFFIXES:
.ONESHELL:

CONFIG_FLAGS:=$(foreach x, $(CONFIG), -D$(x))

VKHOOK_LIBRARY:=vkhook.so
VKHOOK_SOURCES:=vkhook.c vkutils.c capture.c offscreen_swapchain.c glwindow.c egl.c glx.c

DEBUG_FLAGS:=-g

all: $(VKHOOK_LIBRARY)

clean: $(VKHOOK_LIBRARY)_clean

$(VKHOOK_LIBRARY)_cflags:=-I./ -Wall -fPIC $(DEBUG_FLAGS) $(CONFIG_FLAGS)
$(VKHOOK_LIBRARY)_ldflags:=-lvulkan -lm -lEGL -lGL -lGLX -shared $(DEBUG_FLAGS)

$(eval $(call define_c_target,$(VKHOOK_LIBRARY),$(VKHOOK_SOURCES)))

run:
	LD_PRELOAD=./$(VKHOOK_LIBRARY) ../vkcube/vkcube
