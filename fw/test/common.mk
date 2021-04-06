# Enable second expansion
.SECONDEXPANSION:

# Clear all built in suffixes
.SUFFIXES:

##############################################################################
# Special use variables
##############################################################################
NULL :=
SPACE := $(NULL) $(NULL)
INFO_INDENT := $(SPACE)$(SPACE)$(SPACE)

##############################################################################
# Current directory. This is the directory Make was run in
##############################################################################
MAKE_CUR_DIR := $(CURDIR)

# The bcommon dir is defined as the directory of this file
BCOMMON_ROOT := $(abspath $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST)))))

# Work root is a subdir of this dir
WORK_DIR := $(abspath $(BCOMMON_ROOT)/work)

##############################################################################
# Standard Tools
##############################################################################
MKDIR := mkdir -p
RM := rm -f
RMDIR := rm -rf
COPY := cp -f --remove-destination
COPY_FILE_PRESERVE_LINK := cp -pf --remove-destination --no-preserve=ownership --no-dereference
COPYDIR := cp -rf

##############################################################################
# Set default goal before any targets. The default goal here is "all"
##############################################################################
DEFAULT_TARGET := all

.DEFAULT_GOAL := default
.PHONY: default
default: $(DEFAULT_TARGET)

##############################################################################
# Standard targets
##############################################################################
# build
.PHONY: build
build:

# test
.PHONY: test
test: build

# clean
.PHONY: clean
clean:

$(WORK_DIR) :
	$(MKDIR) $@

