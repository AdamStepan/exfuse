#!/usr/bin/make -f
.DEFAULT_GOAL = all
.PHONY: clean mount umount deps

# where .d .o .so and are stored
BUILDDIR := ../build
# where .so will be placed
TARGETDIR := ..
# note: shouldn't end with '/'
SRCDIR := .

CC := gcc
CDEPS := $(CC)
LD := $(CC)
CP := cp
RM := rm

# find files with c extension within current directory and it's subdirectories
SRC := $(addprefix $(SRCDIR)/, $(filter-out exdbg.c exmkfs.c, $(wildcard *.c)))
DEPS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.d,$(SRC))
OBJ := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRC))

# don't create dependencies when we're cleaning or creating deps
ifeq (0, $(words $(findstring $(MAKECMDGOALS), clean mount umount deps)))
	# include .c files dependencies
	-include $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.d,$(SRC))
endif

include verbosity.mk

DEPFLAGS = -MT $(@:.d=.o) -MM -MP $< -MF $(@:.o=.d)
CFLAGS := -fPIC -Wall -I. -ggdb -std=c11 -D_GNU_SOURCE -Wextra -fmax-errors=2
LDFLAGS := -lm

CFLAGS += $(shell pkg-config --cflags fuse)
LDFLAGS += $(shell pkg-config --libs fuse)

all: $(BUILDDIR)/exfuse $(BUILDDIR)/exdbg $(BUILDDIR)/libexfuse.so \
		$(BUILDDIR)/exmkfs

deps: $(DEPS)

$(BUILDDIR)::
	@mkdir -p $(BUILDDIR)

$(TARGETDIR)::
	@mkdir -p $(TARGETDIR)

gen: | $(BUILDDIR)

$(BUILDDIR)/%.d: %.c | gen
	$(CDEPS) $(DEPFLAGS) $(CFLAGS)

$(BUILDDIR)/%.o: %.c | gen
	$(CC) $(CFLAGS) $< -c -o $@

$(BUILDDIR)/exfuse: $(OBJ) | $(TARGETDIR)
	$(LD) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $@
	$(CP) $@ $(TARGETDIR)/$(notdir $@)

$(BUILDDIR)/exdbg: exdbg.c $(OBJ) $(BUILDDIR)/libexfuse.so
	$(CC) $(CFLAGS) $< -c -o $(BUILDDIR)/exdbg.o
	$(LD) $(filter-out $(BUILDDIR)/wrapper.o, $(OBJ)) $(BUILDDIR)/exdbg.o $(LDFLAGS) -o $@
	$(CP) $@ $(TARGETDIR)/$(notdir $@)

$(BUILDDIR)/exmkfs: exmkfs.c $(OBJ) $(BUILDDIR)/libexfuse.so
	$(CC) $(CFLAGS) $< -c -o $(BUILDDIR)/exmkfs.o
	$(LD) $(filter-out $(BUILDDIR)/wrapper.o, $(OBJ)) $(BUILDDIR)/exmkfs.o $(LDFLAGS) -o $@
	$(CP) $@ $(TARGETDIR)/$(notdir $@)

$(BUILDDIR)/libexfuse.so: $(filter-out $(BUILDDIR)/wrapper.o, $(OBJ)) | $(TARGETDIR)
	$(LD) $(CFLAGS) $? $(LDFLAGS) -shared -o $@
	$(CP) $@ $(TARGETDIR)/$(notdir $@)

clean:
	$(RM) -rf $(BUILDDIR) $(TARGETDIR)/exfuse $(TARGETDIR)/libexfuse.so \
			$(TARGETDIR)/exdbg
