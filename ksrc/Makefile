###############################################################################
##                                                                           ##
##    Makefile for Linux device driver for HD44780 compatible displays       ##
##                                                                           ##
##---------------------------------------------------------------------------##
## File:   Linux_HD44780_Display_Driver/ksrc/Makefile                        ##
## Author: Ulrich Becker www.INKATRON.de                                     ##
## Date:   17.02.2017                                                        ##
###############################################################################

# Name of the driver
DRIVER_NAME ?= anLcd

# Version of the driver
VERSION     ?= "0.1"

# Uncommend the following make-variable if your target-system doesn't 
# support device-trees.
#NO_DEVICE_TREE := 1

DEVTREE     ?= anLcd_raspi.dts
BLOB_EXT    ?= dtbo

# Common include directory for header files which are included in user and
# kernel-space.
USER_INCLUDE_DIR ?= ../include/linux/

COMMON_SRC_DIR   ?= ../src/

# Relative directory within the linux- source tree when a patch was made.
# See target "make patch"
REL_BUILD_IN_DIR := drivers/video

#==============================================================================
# If yo want to use a cross-compiler, so you need eventually to adapt the
# following variables on your tool chain.
# E.g.: your tool chain was made by Yocto instead of Buildroot.

ifdef KERNEL_SRC  # In the case of YOCTO recipe KERNEL_SRC is predefined.
   CROSS_KERNEL_SRC_DIR = $(KERNEL_SRC)
else
   TOOLCHAIN_BASE ?= ~/src/Linux_build/buildroot_pi/output/
   CROSS_KERNEL_SRC_DIR ?= $(shell echo $(TOOLCHAIN_BASE)/build/linux-[!h]* )
endif
TOOLCHAIN_PATH ?= $(TOOLCHAIN_BASE)host/usr/bin/
CROSS_COMPILE  ?= $(TOOLCHAIN_PATH)arm-buildroot-linux-uclibcgnueabihf-

#==============================================================================
# Compiler switches for the development phase or for building this module
# external. Otherwise the kernel-config (Kconfig) will accomplish this.
# See also in Kconfig.
ifndef EXTERN_DEFINES
  EXTERN_DEFINES += CONFIG_DEBUG_AN_LCD
  EXTERN_DEFINES += CONFIG_DEBUG_AN_LCD_FSM
  EXTERN_DEFINES += CONFIG_LCD_UNICODE
  EXTERN_DEFINES += CONFIG_AN_LCD_READBACK
  EXTERN_DEFINES += CONFIG_ST7066U
  EXTERN_DEFINES += CONFIG_LCD_USE_BACKSLASH
  EXTERN_DEFINES += CONFIG_LCD_USE_PARAGRAPH

  ifdef NO_DEVICE_TREE
    EXTERN_DEFINES += CONFIG_AN_LCD_NO_DEV_TREE

    # If your target-system doesn't support device-trees, so you need
    # eventually adapt the following defines of the display dimensions and
    # it's GPIO connections.
    # Please see also the specification of your display.
    EXTERN_DEFINES += CONFIG_AN_LCD_MAX_X=16
    EXTERN_DEFINES += CONFIG_AN_LCD_MAX_Y=2
    EXTERN_DEFINES += CONFIG_AN_LCD_GPIO_NUMBER_RS=16
    EXTERN_DEFINES += CONFIG_AN_LCD_GPIO_NUMBER_RW=20
    EXTERN_DEFINES += CONFIG_AN_LCD_GPIO_NUMBER_EN=21
    EXTERN_DEFINES += CONFIG_AN_LCD_GPIO_NUMBER_D4=5
    EXTERN_DEFINES += CONFIG_AN_LCD_GPIO_NUMBER_D5=6
    EXTERN_DEFINES += CONFIG_AN_LCD_GPIO_NUMBER_D6=13
    EXTERN_DEFINES += CONFIG_AN_LCD_GPIO_NUMBER_D7=26
  endif # ifdef NO_DEVICE_TREE
endif # ifndef EXTERN_DEFINES

#==============================================================================
# Optional variables for simplify the developing.
TARGET_DEVICE_USER     ?= root
TARGET_DEVICE_IP       ?= 10.0.0.1
TARGET_DEVICE_DIR      ?= /root
TARGET_DEVICE_BLOB_DIR ?= /boot/overlays
#==============================================================================

###############################################################################
##                 Do not edit the rest of this file!                        ##
###############################################################################
SOURCES := anLcd_drv.c
SOURCES += $(COMMON_SRC_DIR)hd44780Display.c
HEADERS := anLcd_dev_tree_names.h
HEADERS += $(USER_INCLUDE_DIR)an_disp_ioctl.h

HEADERS += $(patsubst %.c, %.h, $(SOURCES))
BLOB_NAME := $(patsubst %.dts, %.$(BLOB_EXT), $(DEVTREE))

ifdef KERNELRELEASE
   # This code-segment becomes invoked from the kernels master-makefile.
   DEFINES += VERSION=$(VERSION)
   ifdef EXTERN
      CONFIG_ALPHANUMERIC_DISPLAY = m
      EXTRA_CFLAGS += $(addprefix -I, $(PWD)/$(USER_INCLUDE_DIR))
      EXTRA_CFLAGS += $(addprefix -I, $(PWD)/$(COMMON_SRC_DIR))
      EXTRA_CFLAGS += $(addprefix -I, $(PWD) )
      DEFINES     += $(EXTERN_DEFINES)
      __SOURCES__ = $(SOURCES)
   else
      __SOURCES__ = $(notdir $(SOURCES))
   endif
   EXTRA_CFLAGS += $(addprefix -D, $(DEFINES))
   obj-$(CONFIG_ALPHANUMERIC_DISPLAY) += $(DRIVER_NAME).o
   $(DRIVER_NAME)-objs := $(patsubst %.c, %.o, $(__SOURCES__))
   dtb-y += $(BLOB_NAME)
else
   # This code-segment becomes invoked immediately.
   ifdef CROSS_COMPILE
      DTC ?= $(TOOLCHAIN_PATH)dtc
      # Symbolic link or path to the embedded kernel-source-tree of the target-platform.
      KERNEL_SRC_DIR = $(CROSS_KERNEL_SRC_DIR)
      ARCH ?= arm
   else
      # By default: (native compiling) symbolic link to kernel-source-tree of the host-OS.
      KERNEL_SRC_DIR ?= /lib/modules/$(shell uname -r)/build
   endif

   PWD  := $(shell pwd)

   ABS_BUILD_IN_DIR := $(KERNEL_SRC_DIR)/$(REL_BUILD_IN_DIR)/$(DRIVER_NAME)/
   BASE_MAKEFILE = $(ABS_BUILD_IN_DIR)../Makefile
   BASE_KCONFIG  = $(ABS_BUILD_IN_DIR)../Kconfig
   LINK_LIST := $(SOURCES) $(HEADERS) Makefile Kconfig

   BASE_ORIGIN_MAKEFILE = $(BASE_MAKEFILE).$(DRIVER_NAME).origin
   BASE_ORIGIN_KCONFIG  = $(BASE_KCONFIG).$(DRIVER_NAME).origin

.PHONY: all checkKernelDir clean status modules_install

# Invoke of the kernels master-makefile.
all: checkKernelDir status # $(BLOB_NAME)
ifdef CROSS_COMPILE
	@$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC_DIR) M=$(PWD) EXTERN=1 modules
else
	@$(MAKE) -C $(KERNEL_SRC_DIR) M=$(PWD) EXTERN=1 modules
endif

modules_install: all
ifdef CROSS_COMPILE
	@$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC_DIR) M=$(PWD) EXTERN=1 modules_install
else
	@$(MAKE) -C $(KERNEL_SRC_DIR) M=$(PWD) EXTERN=1 modules_install
endif


.PHONY: blob
blob: $(BLOB_NAME)

$(DEVTREE).tmp: $(DEVTREE) anLcd_dev_tree_names.h Makefile
	$(CROSS_COMPILE)cpp -P  -DNODE_NAME=$(DRIVER_NAME) -o $(DEVTREE).tmp $(DEVTREE)
	
$(BLOB_NAME): $(DEVTREE).tmp
	$(DTC) -@ -I dts -O dtb -o $(BLOB_NAME) $(DEVTREE).tmp

checkKernelDir:
	@if ! [ -d $(KERNEL_SRC_DIR) ]; then \
		echo "ERROR: Can not find the directory or symbolic link \"$(KERNEL_SRC_DIR)\" to the linux-source-tree!" 1>&2; \
		exit 1; \
	fi; \
	if  ! [ -f $(KERNEL_SRC_DIR)/Makefile ]; then \
		echo "ERROR: Source-tree \"$$(readlink -m $(KERNEL_SRC_DIR))\" is invalid!" 1>&2; \
		echo "       Can not find the kernels master-makefile!" 1>&2; \
		exit 1; \
	fi

$(ABS_BUILD_IN_DIR):
	@mkdir $(ABS_BUILD_IN_DIR); \
	if [ "$$?" != "0" ]; then \
		echo "ERROR: Unable to create directory: \"$(ABS_BUILD_IN_DIR)\"" 1>&2; \
		exit 1; \
	else \
		echo "INFO: Creating directory: \"$(ABS_BUILD_IN_DIR)\""; \
	fi
	
.PHONY: $(LINK_LIST)
$(LINK_LIST): $(ABS_BUILD_IN_DIR)
	@if [ -f "$@" ]; then \
		if [ ! -L "$(ABS_BUILD_IN_DIR)/$(notdir $@)" ]; then \
			ln -sf $$(readlink -m $@) $(ABS_BUILD_IN_DIR) && \
			echo "INFO: Making symbolic link: $(notdir $@) --> $(ABS_BUILD_IN_DIR)"; \
		else \
			echo "INFO: Link for \"$(notdir $@)\" already made."; \
		fi; \
	else \
		echo "ERROR: File \"$@\" not found!" 1>&2; \
		exit 1; \
	fi

$(BASE_ORIGIN_KCONFIG): $(ABS_BUILD_IN_DIR)
	@if [ ! -n "$$(grep "$(DRIVER_NAME)" $(BASE_KCONFIG) )" ]; then \
		cp -f $(BASE_KCONFIG) $(BASE_ORIGIN_KCONFIG); \
		echo "INFO: Saving $(BASE_KCONFIG) in $(BASE_ORIGIN_KCONFIG)"; \
		echo "INFO: Preparing file: $(BASE_KCONFIG)"; \
		if [ -n "$$(grep "endmenu" $(BASE_KCONFIG) )" ]; then \
			sed -e "s/endmenu/source \"$(subst /,\/,$(REL_BUILD_IN_DIR)/$(DRIVER_NAME)/Kconfig)\"\nendmenu/g" \
			$(BASE_ORIGIN_KCONFIG) > $(BASE_KCONFIG); \
		elif [ -n "$$(grep "RTC_CLASS" $(BASE_KCONFIG) )" ]; then \
			sed -e "/RTC_CLASS/s/endif/source \"$(subst /,\/,$(REL_BUILD_IN_DIR)/$(DRIVER_NAME)/Kconfig)\"\nendif/" \
			$(BASE_ORIGIN_KCONFIG) > $(BASE_KCONFIG); \
		else \
			echo "source \"$(REL_BUILD_IN_DIR)/$(DRIVER_NAME)/Kconfig\"" >> $(BASE_KCONFIG); \
		fi; \
	else \
		echo "INFO: File: $(BASE_KCONFIG) already prepared."; \
	fi

$(BASE_ORIGIN_MAKEFILE): $(ABS_BUILD_IN_DIR)
	@if [ ! -n "$$(grep "$(DRIVER_NAME)" $(BASE_MAKEFILE) )" ]; then \
		cp -f $(BASE_MAKEFILE) $(BASE_ORIGIN_MAKEFILE); \
		echo "INFO: Saving $(BASE_MAKEFILE) in $(BASE_ORIGIN_MAKEFILE)"; \
		echo "INFO: Preparing file: $(BASE_MAKEFILE)"; \
		echo 'obj-$$(CONFIG_ALPHANUMERIC_DISPLAY)	+= $(DRIVER_NAME)/' >> $(BASE_MAKEFILE); \
	else \
		echo "INFO: File: $(BASE_MAKEFILE) already prepared."; \
	fi

.PHONY: patch
patch: checkKernelDir $(LINK_LIST) $(BASE_ORIGIN_KCONFIG) $(BASE_ORIGIN_MAKEFILE)
	@echo "INFO: Patch to source-tree \"$$(readlink -m $(KERNEL_SRC_DIR))\" for module \"$(DRIVER_NAME)\" accomplished!"

.PHONY: unpatch
unpatch: checkKernelDir
	@if [ -f $(BASE_ORIGIN_MAKEFILE) ]; then \
		echo "INFO: Rebuilding old origin file: $(BASE_MAKEFILE)"; \
		mv -f $(BASE_ORIGIN_MAKEFILE) $(BASE_MAKEFILE); \
	fi
	@if [ -f $(BASE_ORIGIN_KCONFIG) ]; then \
		echo "INFO: Rebuilding old origin file: $(BASE_KCONFIG)"; \
		mv -f $(BASE_ORIGIN_KCONFIG)  $(BASE_KCONFIG); \
	fi
	@if [ -d $(ABS_BUILD_IN_DIR) ]; then \
		echo "INFO: Removing directory: $(ABS_BUILD_IN_DIR)"; \
		rm -fr $(ABS_BUILD_IN_DIR); \
	fi
	@echo "INFO: Patch to source-tree \"$$(readlink -m $(KERNEL_SRC_DIR))\" revoked!"

clean:
	rm -f *.mod.c *.o *.ko modules.order Module.symvers .*.cmd .tmp_versions/*
	rm -f *.dtb* $(COMMON_SRC_DIR)*.o $(COMMON_SRC_DIR).*.cmd
	rm -f *.tmp
	rmdir .tmp_versions

status:
	@printf "Driver:   %s\n" $(DRIVER_NAME)
	@printf "Version:  %s\n" $(VERSION)
	@printf "Target:   "
ifdef CROSS_COMPILE
	@printf $(ARCH)"\n"
else
	@uname -m
endif
	@printf "Compiler: "
	@$(CROSS_COMPILE)gcc --version | awk -F'\n' '{print $1; exit}'
	@printf "Prefix:   %s\n" $(CROSS_COMPILE)
	@printf "sysroot:  %s\n" $$($(CROSS_COMPILE)gcc -print-sysroot)
	@printf "Kernel:   %s\n" $$(readlink -m $(KERNEL_SRC_DIR))


#========== Following code sequence is for developing purposes only ===========
ifdef CROSS_COMPILE

$(DRIVER_NAME).ko: all

.PHONY: scp
scp: $(DRIVER_NAME).ko
	scp $(DRIVER_NAME).ko $(TARGET_DEVICE_USER)@$(TARGET_DEVICE_IP):$(TARGET_DEVICE_DIR)

.PHONY: scpblob
scpblob: $(BLOB_NAME)
	scp $(BLOB_NAME) $(TARGET_DEVICE_USER)@$(TARGET_DEVICE_IP):$(TARGET_DEVICE_BLOB_DIR)

SSH := ssh $(TARGET_DEVICE_USER)@$(TARGET_DEVICE_IP)
else
SSH :=
endif # ifdef CROSS_COMPILE

.PHONY: rmmod
rmmod:
	$(SSH) rmmod $(DRIVER_NAME).ko

.PHONY: insmod
insmod: scp
	$(SSH) insmod $(DRIVER_NAME).ko

.PHONY: showproc
showproc:
	$(SSH) cat /proc/driver/$(DRIVER_NAME)

.PHONY: reboot
reboot:
	$(SSH) reboot

#==============================================================================

.PHONY: help
help:
	@echo
	@echo "Valid targets for this Makefile:"
	@echo "================================="
	@echo
	@echo "help                        Print this help."
	@echo
	@echo "all                         Building $(DRIVER_NAME).ko"
	@echo
	@echo "scp                         Building $(DRIVER_NAME).ko and copy it via scp to"
	@echo "                            the remote test target device."
	@echo "                            NOTE: In this case the definitions of TARGET_DEVICE_DIR,"
	@echo "                                  TARGET_DEVICE_USER and TARGET_DEVICE_IP are assumed."
	@echo
	@echo "status                      Print some informations about the tool-chain, compiler "
	@echo "                            and so on..."
	@echo
	@echo "clean                       Deletes all via \"make all\" built files."
	@echo
	@echo "cleandir                    Same like clean."
	@echo
	@echo "patch                       Prepares a given linux source-tree (defined in KERNEL_SRC_DIR)"
	@echo "                            to compile the \"$(DRIVER_NAME)\" as static build-in module for"
	@echo "                            a monolithic linux-kernel."
	@echo
	@echo "unpatch                     Counterpart to \"make patch\". It sets the given linux source-tree"
	@echo "                            (defined in KERNEL_SRC_DIR) back in its origin state."
	@echo
	@echo "Optional environment variables for developing and using scp to the test-target:"
	@echo "==============================================================================="
	@echo
	@echo "TARGET_DEVICE_DIR           Target-directory of the remote target device."
	@echo "                            Default: /root"
	@echo
	@echo "TARGET_DEVICE_USER          User for the remote target device."
	@echo "                            Necessary for a SSH-connection."
	@echo "                            Default: root."
	@echo
	@echo "TARGET_DEVICE_IP            IP-address of the remote target device."
	@echo "                            Necessary for a SSH-connection."
	@echo "                            Default is the content of ./target.ip."

endif # /ifdef KERNELRELEASE
#=================================== EOF ======================================
