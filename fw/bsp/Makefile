#//START_MODULE_HEADER/////////////////////////////////////////////////////////
#
# Description: This makefile fragment support firmware BSP build
#
# Authors:     Alex Teoh
#
#              Copyright (c)  Intel Corporation 2019
#              All rights reserved.
#
#//END_MODULE_HEADER///////////////////////////////////////////////////////////

BSP_MK_ABSPATH := $(abspath $(lastword $(MAKEFILE_LIST)))
BSP_MK_DIR := $(notdir $(patsubst %/,%,$(dir $(BSP_MK_ABSPATH))))

SHELL := /bin/bash

# Enable second expansion
.SECONDEXPANSION:
# Clear all built in suffixes
.SUFFIXES:
.DELETE_ON_ERROR:

##############################################################################
# Set default goal before any targets. The default goal here is "all"
##############################################################################
DEFAULT_TARGET := all

.DEFAULT_GOAL := default
.PHONY: default
default: $(DEFAULT_TARGET)

##############################################################################
# BSPs
##############################################################################

ALL_BSP := pfr_sys pfr_sys_sim

CREATE_BSP_TARGETS := $(patsubst %,create-%,$(ALL_BSP))
CLEAN_BSP_TARGETS := $(patsubst %,clean-%,$(ALL_BSP))

##############################################################################
# Makefile starts here
##############################################################################

.PHONY: all
all: $(CREATE_BSP_TARGETS)

.PHONY: $(CREATE_BSP_TARGETS)
$(CREATE_BSP_TARGETS): create-% : %
	$(MAKE) -C $<

.PHONY: clean
clean: $(CLEAN_BSP_TARGETS)


# Make clean in sub-directories if their makefiles exist
.PHONY: $(CLEAN_BSP_TARGETS)
$(CLEAN_BSP_TARGETS): clean-%: %
	@if [ "$(wildcard $</Makefile)" != "" ]; then \
		$(MAKE) -C $< clean; \
	fi

