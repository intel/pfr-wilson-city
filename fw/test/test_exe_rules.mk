# Build module specific gtest
$(addprefix $($(MODULE_NAME)_WORK_DIR)/,$(GTEST_OBJ_LIST)) : MODULE_NAME := $(MODULE_NAME)
$(addprefix $($(MODULE_NAME)_WORK_DIR)/,$(GTEST_OBJ_LIST)) : TARGET_SPECIFIC_CPPFLAGS := $($(MODULE_NAME)_GTEST_CPPFLAGS)
$(addprefix $($(MODULE_NAME)_WORK_DIR)/,$(GTEST_OBJ_LIST)) : $($(MODULE_NAME)_WORK_DIR)/%.obj : $(WORK_DIR)/%.cc | $($(MODULE_NAME)_WORK_DIR)
	$(global_obj_from_cpp)

# Target to build all of the module
build-$(MODULE_NAME) :

##############################################################################
# Build module specific main
##############################################################################
TEST_EXE_NAMES += $(MODULE_NAME)_main
$(MODULE_NAME)_main : MODULE_NAME := $(MODULE_NAME)
$(MODULE_NAME)_main : $(addprefix $($(MODULE_NAME)_WORK_DIR)/,main.obj $(MAIN_SYSTEM_OBJS_LIST) $(MAIN_UNITTEST_OBJS_LIST) $(GTEST_OBJ_LIST))
	$(global_exe_link)

build-$(MODULE_NAME) : $(MODULE_NAME)_main

ifeq ($(ENABLE_RUN_RULES),1)
test-full-main : run_$(MODULE_NAME)_main
test-full : run_$(MODULE_NAME)_main
endif
run_$(MODULE_NAME)_main : MODULE_NAME := $(MODULE_NAME)
run_$(MODULE_NAME)_main : $(MODULE_NAME)_main
	$(info Preparing to run $@)
	# Clear the LD_LIBRARY_PATH before running
	LD_LIBRARY_PATH=$(GCC_ROOTDIR)/lib64:$(BOOST_ROOT_DIR)/lib ./$(MODULE_NAME)_main | tee $(MODULE_NAME)_main_out.txt


##############################################################################
# Build module specific main_no_bsp_mock
##############################################################################
TEST_EXE_NAMES += $(MODULE_NAME)_main_no_bsp_mock
$(MODULE_NAME)_main_no_bsp_mock : MODULE_NAME := $(MODULE_NAME)
$(MODULE_NAME)_main_no_bsp_mock : $(addprefix $($(MODULE_NAME)_WORK_DIR)/,$(MAIN_NO_BSP_MOCK_OBJS_LIST) $(GTEST_OBJ_LIST))
	$(global_exe_link)

build-$(MODULE_NAME) : $(MODULE_NAME)_main_no_bsp_mock

ifeq ($(ENABLE_RUN_RULES),1)
test-full : run_$(MODULE_NAME)_main_no_bsp_mock
endif
run_$(MODULE_NAME)_main_no_bsp_mock : MODULE_NAME := $(MODULE_NAME)
run_$(MODULE_NAME)_main_no_bsp_mock : $(MODULE_NAME)_main_no_bsp_mock
	$(info Preparing to run $@)
	# Clear the LD_LIBRARY_PATH before running
	LD_LIBRARY_PATH=$(GCC_ROOTDIR)/lib64:$(BOOST_ROOT_DIR)/lib ./$(MODULE_NAME)_main_no_bsp_mock | tee $(MODULE_NAME)_main_no_bsp_mock_out.txt


##############################################################################
# Build module specific main_t2
##############################################################################
TEST_EXE_NAMES += $(MODULE_NAME)_main_t2
$(MODULE_NAME)_main_t2 : MODULE_NAME := $(MODULE_NAME)
$(MODULE_NAME)_main_t2 : $(addprefix $($(MODULE_NAME)_WORK_DIR)/,main.obj $(MAIN_SYSTEM_OBJS_LIST) $(MAIN_T2_UNITTEST_OBJS_LIST) $(GTEST_OBJ_LIST))
	$(global_exe_link)

build-$(MODULE_NAME) : $(MODULE_NAME)_main_t2

ifeq ($(ENABLE_RUN_RULES),1)
test-full : run_$(MODULE_NAME)_main_t2
endif
run_$(MODULE_NAME)_main_t2 : MODULE_NAME := $(MODULE_NAME)
run_$(MODULE_NAME)_main_t2 : $(MODULE_NAME)_main_t2
	$(info Preparing to run $@)
	# Clear the LD_LIBRARY_PATH before running
	LD_LIBRARY_PATH=$(GCC_ROOTDIR)/lib64:$(BOOST_ROOT_DIR)/lib ./$(MODULE_NAME)_main_t2 | tee $(MODULE_NAME)_main_t2_out.txt

