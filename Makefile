include build.mk

.SUFFIXES:
.ONESHELL:

VKHOOK_LIBRARY:=vkhook.so
VKHOOK_SOURCES:=vkhook.c

DEBUG_FLAGS:=-g

all: $(VKHOOK_LIBRARY)

clean: $(VKHOOK_LIBRARY)_clean

$(VKHOOK_LIBRARY)_cflags:=-I./ -Wall -fPIC $(DEBUG_FLAGS)
$(VKHOOK_LIBRARY)_ldflags:=-lvulkan -shared $(DEBUG_FLAGS)

$(eval $(call define_c_target,$(VKHOOK_LIBRARY),$(VKHOOK_SOURCES)))

run:
	LD_PRELOAD=./$(VKHOOK_LIBRARY) ../vkcube/vkcube