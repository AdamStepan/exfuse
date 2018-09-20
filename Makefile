#!/usr/bin/make -f
.DEFAULT_GOAL = all
.PHONY: clean mount umount

# where .d .o .so and generated .hpp are stored
BUILDDIR := ./build
# where .so will be placed
TARGETDIR := .
# note: shouldn't end with '/'
SRCDIR := .

CC := gcc
CDEPS := $(CC)
LD := $(CC)
CP := cp
RM := rm

# find files with c extension within current directory and it's subdirectories
SRC := $(addprefix $(SRCDIR)/, $(filter-out , $(wildcard *.c)))
DEPS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.d,$(SRC))
OBJ := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRC))

-include verbosity.mk

DEPFLAGS = -MT $(@:.d=.o) -MM -MP $< -MF $(@:.o=.d)
CFLAGS := -Wall -I. -ggdb -std=c11
LDFLAGS :=

CFLAGS += $(shell pkg-config --cflags fuse)
LDFLAGS += $(shell pkg-config --libs fuse)

all: $(BUILDDIR)/exfuse

$(BUILDDIR)::
	@mkdir -p $(BUILDDIR)

$(TARGETDIR)::
	@mkdir -p $(TARGETDIR)

# create a build directory, generate java headers and protobuffer C++ files
gen: | $(BUILDDIR)

$(BUILDDIR)/%.d: %.c | gen
	$(CDEPS) $(DEPFLAGS) $(CFLAGS)

$(BUILDDIR)/%.o: %.c | gen
	$(CC) $(CFLAGS) $< -c -o $@

$(BUILDDIR)/exfuse: $(OBJ) | $(TARGETDIR)
	$(LD) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $@
	$(CP) $@ $(TARGETDIR)/$(notdir $@)

mount: $(BUILDDIR)/exfuse
	if [ ! -d mp ]; then\
		mkdir mp;\
	fi
	./exfuse -f mp

umount:
	fusermount -u mp

clean:
	$(RM) -rf $(BUILDDIR) $(TARGETDIR)/exfuse
