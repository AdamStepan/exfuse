#!/usr/bin/make -f
.DEFAULT_GOAL = all
.PHONY: clean mount umount deps

# where .d .o .so and generated .hpp are stored
BUILDDIR := ../build
# where .so will be placed
TARGETDIR := ..
# note: shouldn't end with '/'
SRCDIR := ../src

CC := gcc
CDEPS := $(CC)
LD := $(CC)
CP := cp
RM := rm

# find files with c extension within current directory and it's subdirectories
SRC := $(addprefix $(SRCDIR)/, $(filter-out , $(wildcard *.c)))
DEPS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.d,$(SRC))
OBJ := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%,$(SRC))
BIN := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.test,$(SRC))

# don't create dependencies when we're cleaning or creating deps
ifeq (0, $(words $(findstring $(MAKECMDGOALS), clean mount umount deps)))
	# include .cpp files dependencies
	-include $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.d,$(SRC))
endif

-include verbosity.mk

DEPFLAGS = -MT $(@:.d=) -MM -MP $< -MF $(@:.o=.d)
CFLAGS := -Wall -I$(SRCDIR) -ggdb -std=c11 -D_POSIX_C_SOURCE=200809L
LDFLAGS := -L$(BUILDDIR) -lexfuse

CFLAGS += $(shell pkg-config --cflags fuse)
LDFLAGS += $(shell pkg-config --libs fuse)

all: $(BIN)

test:
	@for testfile in $(BIN); do \
			/bin/echo -en "$$testfile ("; \
			LD_LIBRARY_PATH=$(BUILDDIR) ./$$testfile >> log; \
			/bin/echo ") --> $$?"; \
	done

deps: $(DEPS)

$(BUILDDIR)::
	@mkdir -p $(BUILDDIR)

$(TARGETDIR)::
	@mkdir -p $(TARGETDIR)

# create a build directory, generate java headers and protobuffer C++ files
gen: | $(BUILDDIR) $(BUILDDIR)/libexfuse.so

$(BUILDDIR)/%.d: %.c | gen
	$(CDEPS) $(DEPFLAGS) $(CFLAGS)

$(BUILDDIR)/%.test: %.c | gen
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

$(BUILDDIR)/libexfuse.so:
	$(MAKE) --directory $(SRCDIR) $@

clean:
	$(RM) -rf $(BUILDDIR)
	$(RM) -rf log exdev
