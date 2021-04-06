  
THIS_MK_ABSPATH := $(abspath $(lastword $(MAKEFILE_LIST)))
THIS_MK_DIR := $(patsubst %/,%,$(dir $(THIS_MK_DIR)))
# Enable second expansion
.SECONDEXPANSION:

# Clear all built in suffixes
.SUFFIXES:

##############################################################################
# Set default goal before any targets. The default goal here is "compile"
##############################################################################
DEFAULT_TARGET := compile

.DEFAULT_GOAL := default
.PHONY: default
default: $(DEFAULT_TARGET)

##############################################################################
# Makefile starts here
##############################################################################

.PHONY: prep
prep: prep_done.txt

prep_done.txt:
	sh ./prep_revisions.sh

.PHONY: compile
compile: prep 
	sh ./compile_non_pfr.sh
	sh ./compile_recovery.sh
	sh ./compile_pfr.sh
# The blocksign tool is required to sign the capsules
ifeq ($(ENABLE_BLOCKSIGN),1)
# Create the signed/unsigned CPLD update capsule
	make -C bin/blocksign_tool/source 
	bin/blocksign_tool/source/blocksign -c bin/blocksign_tool/config_xmls/cpld_update_capsule.xml -o output_files/cpld_update_capsule.bin output_files/pfr_cfm1_auto.rpd
	mv output_files/pfr_cfm1_auto.rpd_aligned output_files/cpld_update_capsule_unsigned.bin
# This target generates the SHA256/SHA384 checksums of the CFM image inside the update capsule. 
# Note that the update capsule contains a SVN field besides the CFM image. Hence, we cannot simply hash the update capsule protected content. 
	bin/blocksign_tool/source/blocksign -c bin/blocksign_tool/config_xmls/cpld_update_capsule_without_svn.xml -o tmp.bin output_files/pfr_cfm1_auto.rpd
	sha256sum output_files/pfr_cfm1_auto.rpd_aligned | head -c 64 > output_files/cpld_update_capsule_cfm_image_sha256_checksums.txt
	sha384sum output_files/pfr_cfm1_auto.rpd_aligned | head -c 128 > output_files/cpld_update_capsule_cfm_image_sha384_checksums.txt
	rm output_files/pfr_cfm1_auto.rpd_aligned tmp.bin
endif
        
 