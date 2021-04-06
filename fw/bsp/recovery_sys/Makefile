# Enable second expansion
.SECONDEXPANSION:

# Clear all built in suffixes
.SUFFIXES:

##############################################################################
# Set default goal before any targets. The default goal here is "test"
##############################################################################
DEFAULT_TARGET := all

.DEFAULT_GOAL := default
.PHONY: default
default: $(DEFAULT_TARGET)

##############################################################################
# Settings
##############################################################################

##############################################################################
# Rules
##############################################################################
.PHONY : clean
clean :
	-rm -rf bsp settings.bsp.tmp

bsp :
	mkdir bsp

settings.bsp.tmp : settings.bsp
	cp -f $^ $@
	chmod u+w $@

bsp/Makefile : settings.bsp.tmp ../../../src/recovery_sys/recovery_sys.sopcinfo | bsp
	nios2-bsp-generate-files --bsp-dir bsp --settings settings.bsp.tmp

bsp/libhal_bsp.a : bsp/Makefile
	$(info Building $@)
	$(MAKE) -C bsp

.PHONY : all
all: bsp/libhal_bsp.a

