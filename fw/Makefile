# Enable second expansion
.SECONDEXPANSION:

# Clear all built in suffixes
.SUFFIXES:

##############################################################################
# Set default goal before any targets. The default goal here is "all"
##############################################################################
DEFAULT_TARGET := all

.DEFAULT_GOAL := default
.PHONY: default
default: $(DEFAULT_TARGET)

##############################################################################
# Include bcommon makefiles 
##############################################################################

include bcommon/coverity.mk

##############################################################################
# Sub-directories
##############################################################################

BSP_DIR = bsp
SRC_DIR = code
TEST_DIR = test

##############################################################################
# Makefile starts here
##############################################################################

.PHONY: all
all:
	$(MAKE) -C $(BSP_DIR) all
	$(MAKE) -C $(SRC_DIR) all
	$(MAKE) -C $(TEST_DIR) all

.PHONY: coverity-check
coverity-check: 
	$(MAKE) clean
	$(MAKE) -C $(BSP_DIR) all
	arc shell coverity -- $(MAKE) cov-pass

.PHONY: coverity-scan
coverity-scan: 
	$(MAKE) clean
	$(MAKE) -C $(BSP_DIR) all
	arc shell coverity -- $(MAKE) cov-scan

.PHONY: clean
clean:
	$(MAKE) -C $(SRC_DIR) clean
	$(MAKE) -C $(TEST_DIR) clean
	$(MAKE) -C $(BSP_DIR) clean
	-rm -rf coverity*
	
.PHONY : eclipse
eclipse :
	/nfs/site/disks/swbld_releases_1/acds/18.1std/625/linux64/nios2eds/bin/eclipse_nios2/eclipse -data eclipse-workspace
