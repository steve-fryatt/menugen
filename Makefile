# Makefile for MenuGen
#
# Copyright 2010, Stephen Fryatt
#
# This file really needs to be run by GNUMake.

.PHONY:	all

BUILD_DATE := $(shell date "+%-d %b %Y")

ifeq ($(TARGET),riscos)
  CC := $(wildcard $(GCCSDK_INSTALL_CROSSBIN)/*gcc)

  # Compilation flags

  CCFLAGS := -mlibscl -mhard-float -static -mthrowback -Wall -O2 -fno-strict-aliasing -mpoke-function-name
else
  CC := gcc

  # Compilation flags

  CCFLAGS := -Wall -O2 -fno-strict-aliasing -D'BUILD_DATE="$(BUILD_DATE)"'
endif

# Set up source and object files.

INCLUDES := -I$(GCCSDK_INSTALL_ENV)/include

LINKS := -L$(GCCSDK_INSTALL_ENV)/lib

OBJS := data.o menugen.o parse.o stack.o

# Start to define the targets.

all:	menugen


# Build MenuGen itself.

menugen: $(OBJS)
	$(CC) $(CCFLAGS) $(LINKS) -o menugen $(OBJS)

-include $(OBJS:.o=.d)

%.o: %.c
	$(CC) -c $(CCFLAGS) $(INCLUDES) $*.c -o $*.o
	$(CC) -MM $(CCFLAGS) $(INCLUDES) $*.c > $*.d
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

