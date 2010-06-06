# Makefile for MenuGen
#
# Copyright 2010, Stephen Fryatt
#
# This file really needs to be run by GNUMake.
# It is intended for native compilation on Linux (for use in a GCCSDK
# environment) or cross-compilation under the GCCSDK.

.PHONY: all clean documentation release install


# The archive to assemble the release files in.  If $(RELEASE) is set, then the file can be given
# a standard version number suffix.

ZIPFILE := menugen$(RELEASE).zip


# The build date.

BUILD_DATE := $(shell date "+%d %b %Y")


# Build Tools

ifeq ($(TARGET),riscos)
  CC := $(wildcard $(GCCSDK_INSTALL_CROSSBIN)/*gcc)
else
  CC := gcc
endif

RM := rm -rf
CP := cp

ZIP := /home/steve/GCCSDK/env/bin/zip

SFBIN := /home/steve/GCCSDK/sfbin

TEXTMAN := $(SFBIN)/textman
STRONGMAN := $(SFBIN)/strongman
HTMLMAN := $(SFBIN)/htmlman
DDFMAN := $(SFBIN)/ddfman
BINDHELP := $(SFBIN)/bindhelp
TEXTMERGE := $(SFBIN)/textmerge
MENUGEN := $(SFBIN)/menugen


# Build Flags

ifeq ($(TARGET),riscos)
  CCFLAGS := -mlibscl -mhard-float -static -mthrowback -Wall -O2 -D'BUILD_DATE="$(BUILD_DATE)"' -fno-strict-aliasing -mpoke-function-name
else
  CCFLAGS := -Wall -O2 -fno-strict-aliasing -D'BUILD_DATE="$(BUILD_DATE)"'
endif
ZIPFLAGS := -x "*/.svn/*" -r -, -9
BINDHELPFLAGS := -f -r -v
MENUGENFLAGS := -d


# Includes and libraries.

INCLUDES := -I$(GCCSDK_INSTALL_ENV)/include
LINKS := -L$(GCCSDK_INSTALL_ENV)/lib


# Set up the various build directories.

SRCDIR := src
MANUAL := manual
OBJDIR := obj
OUTDIR := build


# Set up the named target files.

RUNIMAGE := menugen
README := ReadMe


# Set up the source files.

MANSRC := Source
MANSPR := ManSprite

OBJS := data.o menugen.o parse.o stack.o


# Build everything, but don't package it for release.

all: documentation $(OUTDIR)/$(RUNIMAGE)


# Build the complete !RunImage from the object files.

OBJS := $(addprefix $(OBJDIR)/, $(OBJS))

$(OUTDIR)/$(RUNIMAGE): $(OBJS)
	$(CC) $(CCFLAGS) $(LINKS) -o $(OUTDIR)/$(RUNIMAGE) $(OBJS)


# Build the object files, and identify their dependencies.

-include $(OBJS:.o=.d)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $(CCFLAGS) $(INCLUDES) $< -o $@
	@$(CC) -MM $(CCFLAGS) $(INCLUDES) $< > $(@:.o=.d)
	@mv -f $(@:.o=.d) $(@:.o=.d).tmp
	@sed -e 's|.*:|$@:|' < $(@:.o=.d).tmp > $(@:.o=.d)
	@sed -e 's/.*://' -e 's/\\$$//' < $(@:.o=.d).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(@:.o=.d)
	@rm -f $(@:.o=.d).tmp


# Build the documentation

documentation: $(OUTDIR)/$(README)

$(OUTDIR)/$(README): $(MANUAL)/$(MANSRC)
	$(TEXTMAN) $(MANUAL)/$(MANSRC) $(OUTDIR)/$(README)


# Build the release Zip file.

release: clean all
	$(RM) $(ZIPFILE)
	(cd $(OUTDIR) ; $(ZIP) $(ZIPFLAGS) ../$(ZIPFILE) $(RUNIMAGE))
	(cd $(OUTDIR) ; $(ZIP) $(ZIPFLAGS) ../$(ZIPFILE) $(README))


# Install the finished version in the GCCSDK, ready for use.

install: clean all
	$(CP) -r $(OUTDIR)/$(RUMIMAGE) $(SFBIN)


# Clean targets

clean:
	$(RM) $(OBJDIR)/*
	$(RM) $(OUTDIR)/$(RUNIMAGE)
	$(RM) $(OUTDIR)/$(README)

