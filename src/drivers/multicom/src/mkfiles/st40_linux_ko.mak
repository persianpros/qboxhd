#
# mkfiles/st40_linux_ko.mak
#
# Copyright (C) STMicroelectronics Ltd. 2001
#
# Setup macros for SH4 Linux kernel modules
#

# Allow either KERNELDIR_ST40_LINUX_KO or O to be used
# to specify where to find the Linux kernel image
KERNELDIR_ST40_LINUX_KO ?= $(O)
ifeq ($(KERNELDIR),)
KERNELDIR = $(KERNELDIR_ST40_LINUX_KO)
else
ifeq ($(KERNELDIR_ST40_LINUX_KO),)
KERNELDIR_ST40_LINUX_KO = $(KERNELDIR)
endif
endif

ifndef DISABLE_ST40_LINUX_KO
  ifneq (,$(KERNELDIR_ST40_LINUX_KO))
    ENABLE_ST40_LINUX_KO = 1
  endif
endif

# Always use GPL version for Havana
ENABLE_GPL=1

OBJDIR_ST40_LINUX_KO   = obj/linux/st40
ifdef ENABLE_PLATFORM_SPECIFIC_OBJECTS
OBJDIR_ST40_LINUX_KO  := $(OBJDIR_ST40_LINUX_KO)/$(PLATFORM)
endif

ifdef ENABLE_ST40_LINUX_KO
DIST_ST40_LINUX_GCC=$(call FINDEXEC,sh4-linux-gcc)

# switch automatically between versions of the distribution
DIST_ST40_LINUX_1_0 = /opt/STM/ST40Linux-1.0
DIST_ST40_LINUX_2_0 = /opt/STM/STLinux-2.0
DIST_ST40_LINUX_2_2 = /opt/STM/STLinux-2.2
DIST_ST40_LINUX_2_3 = /opt/STM/STLinux-2.3
ifeq ($(DIST_ST40_LINUX_1_0),$(findstring $(DIST_ST40_LINUX_1_0),$(DIST_ST40_LINUX_GCC)))
DIST_ST40_LINUX ?= $(DIST_ST40_LINUX_1_0)
else
ifeq ($(DIST_ST40_LINUX_2_0),$(findstring $(DIST_ST40_LINUX_2_0),$(DIST_ST40_LINUX_GCC)))
DIST_ST40_LINUX ?= $(DIST_ST40_LINUX_2_0)
else
ifeq ($(DIST_ST40_LINUX_2_2),$(findstring $(DIST_ST40_LINUX_2_2),$(DIST_ST40_LINUX_GCC)))
DIST_ST40_LINUX ?= $(DIST_ST40_LINUX_2_2)
else
DIST_ST40_LINUX ?= $(DIST_ST40_LINUX_2_3)
endif
endif
endif

CLEAN_DIRS             += $(OBJDIR_ST40_LINUX_KO)/*
MAKE_DIRS              += $(OBJDIR_ST40_LINUX_KO)
LIBDIR_ST40_LINUX       = $(RPC_ROOT)/lib/linux/st40

CC_ST40_LINUX_KO        = sh4-linux-gcc
CPP_ST40_LINUX_KO       = sh4-linux-gcc -E
WARNINGS_ST40_LINUX_KO  = -Wall
ifndef ENABLE_DEBUG
# At present gcc is unable to study control flow sufficiently accurately
# for this warning to be useful. Most people fix it by simply initializing
# with a 'broken' value which hides the bug (if it existed) but will continue
# to suppress the warning even if the compiler is improved.
# http://kerneltrap.org/node/6591
#WARNINGS_ST40_LINUX_KO += -Wno-uninitialized

# This warning cannot be suppressed without patching the kernel since it
# is present in various kernel macros (notably memory barriers). This is
# outsides multicom's domain so we simply band-aid it to meet the build
# criteria that have been imposed.
#WARNINGS_ST40_LINUX_KO += -Wno-unused-value
endif
# Sync up Warnings with Linux 2.6.23 kernel build
WARNINGS_ST40_LINUX_KO += -Wundef -Wstrict-prototypes -Wno-trigraphs -Werror-implicit-function-declaration
WARNINGS_ST40_LINUX_KO += -Wdeclaration-after-statement -Wno-pointer-sign

CFLAGS_ST40_LINUX_KO    = $(WARNINGS_ST40_LINUX_KO) -ml -Wa,-isa=sh4 -m4 -m4-nofpu -pipe \
		         $(DEFS_ST40_LINUX_KO) $(OPT_ST40_LINUX_KO) $(DEBUG_ST40_LINUX_KO) \
		         $(LOCAL_CFLAGS) $(LOCAL_CFLAGS_ST40_LINUX_KO) $(DEBUG_CFLAGS) $(INCS_ST40_LINUX_KO)
DEFS_ST40_LINUX_KO      = -D__LINUX__ -D__SH4__ \
                          -D__KERNEL__ -DMODULE -DEXPORT_SYMTAB
# Sync up options with Linux 2.6.23 kernel build
DEFS_ST40_LINUX_KO     += -fno-common -ffreestanding -fno-omit-frame-pointer -fno-optimize-sibling-calls
DEFS_ST40_LINUX_KO     += -fno-strict-aliasing -fno-stack-protector

INCS_ST40_LINUX_KO      = -I$(RPC_ROOT)/include -Wp,-isystem,$(KERNELDIR_ST40_LINUX_KO)/include -Wp,-isystem,$(KERNELDIR_ST40_LINUX_KO)/include2 -Wp,-isystem,$(KERNELDIR)/include
# This is needed for Linux 2.6.23 and later
INCS_ST40_LINUX_KO     += -include $(KERNELDIR_ST40_LINUX_KO)/include/linux/autoconf.h

# Conditionally enable Linux GPL support
ifdef ENABLE_GPL
DEFS_ST40_LINUX_KO     += -DMULTICOM_GPL
endif

ifdef ENABLE_RPCCC
CC_ST40_LINUX_KO       := rpccc $(CC_ST40_LINUX_KO)
endif
ifdef ENABLE_DEBUG
OPT_ST40_LINUX_KO       = -O2 
DEBUG_ST40_LINUX_KO     = -g
else
OPT_ST40_LINUX_KO       = -O2
DEBUG_ST40_LINUX_KO     =
endif

OBJ_ST40_LINUX_KO       = lo

ifndef OBJDIR_ST40_LINUX_MKDIR_RULE_ALREADY_DONE
OBJDIR_ST40_LINUX_MKDIR_RULE_ALREADY_DONE = 1
$(OBJDIR_ST40_LINUX_KO) :
	-$(MKDIR) $(call DOSCMD,$@)
endif

LIB_ST40_LINUX_KO       = sh4-linux-ld
LIBFLAGS_ST40_LINUX_KO  = -r -EL $(LOCAL_LIBFLAGS_ST40_LINUX_KO) -o

LD_ST40_LINUX_KO        = sh4-linux-ld
LDFLAGS_ST40_LINUX_KO   = -r -EL $(LOCAL_LDFLAGS_ST40_LINUX_KO) -o

ifneq ($(DIST_ST40_LINUX_1_0),$(DIST_ST40_LINUX))
MODPOST_ST40_LINUX_KO   = $(KERNELDIR_ST40_LINUX_KO)/scripts/mod/modpost 
MODPOSTFLAGS_ST40_LINUX_KO = -i $(KERNELDIR_ST40_LINUX_KO)/Module.symvers
endif


# TODO: the use of global macros in this function will prevent us from testing
#       multiple Linux on a single SoC.
#
ifdef ENABLE_STMC2
RTLD_ST40_LINUX_KO    = env RPC_ROOT=$(RPC_ROOT) \
			st40load_gdb -c sh4tp -t $2 -b $(RTLD_KERNEL_ST40_LINUX)\
		       -z $(RTLD_RAMDISKADDR_ST40_LINUX),$(OBJDIR_ST40_LINUX)/$(TEST)_ko.img\
		       -- $(RTLD_COMMANDLINE_ST40_LINUX)
else
RTLD_ST40_LINUX_KO    = env RPC_ROOT=$(RPC_ROOT) \
			st40load_gdb -c $1 -t $2 -b $(RTLD_KERNEL_ST40_LINUX)\
		       -z $(RTLD_RAMDISKADDR_ST40_LINUX),$(OBJDIR_ST40_LINUX)/$(TEST)_ko.img\
		       -- $(RTLD_COMMANDLINE_ST40_LINUX)
endif

ifndef RTLD_KERNEL_ST40_LINUX
RTLD_KERNEL_ST40_LINUX = $(error RTLD_KERNEL_ST40_LINUX is not set)
endif
ifndef RTLD_RAMDISKADDR_ST40_LINUX
RTLD_RAMDISKADDR_ST40_LINUX = $(error RTLD_RAMDISKADDR_ST40_LINUX is not set)
endif
ifndef RTLD_COMMANDLINE_ST40_LINUX
RTLD_COMMANDLINE_ST40_LINUX = $(error RTLD_COMMANDLINE_ST40_LINUX is not set)
endif

.SUFFIXES: .$(OBJ_ST40_LINUX_KO) .c

$(OBJDIR_ST40_LINUX_KO)/%.$(OBJ_ST40_LINUX_KO) : %.c
ifdef ENABLE_COMENTARY
	@$(ECHO) +++ Compile ST40/Linux kernel module source [$<]. +++
endif
	$(CC_ST40_LINUX_KO) -c $(CFLAGS_ST40_LINUX_KO) $< -o $@ 

ifdef OBJS_ST40_LINUX_KO

# Linux kernel modules treat applications and libraries identically
ifdef APPLICATION
LIBRARY=$(APPLICATION)
endif

ifdef KBUILD_WRAPPER
TARGET_FOR_ST40_LINUX_KO = $(OBJDIR_ST40_LINUX_KO)/$(LIBRARY).o_shipped
else
TARGET_FOR_ST40_LINUX_KO = $(OBJDIR_ST40_LINUX_KO)/$(LIBRARY).ko
endif
DIST_ST40_LINUX_LIB     += $(TARGET_FOR_ST40_LINUX_KO)

ifeq ($(DIST_ST40_LINUX_1_0),$(DIST_ST40_LINUX))

$(TARGET_FOR_ST40_LINUX_KO) : $(OBJS_ST40_LINUX_KO)
	$(LIB_ST40_LINUX_KO) $(LIBFLAGS_ST40_LINUX_KO) $@ $(OBJS_ST40_LINUX_KO)

else # STLinux-2.0/STLinux-2.2

ifeq ($(DIST_ST40_LINUX_2_0),$(DIST_ST40_LINUX))
LOCAL_CFLAGS_ST40_LINUX_KO += \
	-DKBUILD_MODNAME=$(LIBRARY) \
	-DKBUILD_BASENAME=$(subst -,_,$(*F))
else
LOCAL_CFLAGS_ST40_LINUX_KO += \
	-D"KBUILD_STR(s)=\#s" \
	-D"KBUILD_MODNAME=KBUILD_STR($(LIBRARY))" \
	-D"KBUILD_BASENAME=KBUILD_STR($(*F))"
endif

$(OBJDIR_ST40_LINUX_KO)/$(LIBRARY).o : $(OBJS_ST40_LINUX_KO)
	$(LIB_ST40_LINUX_KO) $(LIBFLAGS_ST40_LINUX_KO) $@ $(OBJS_ST40_LINUX_KO)

$(OBJDIR_ST40_LINUX_KO)/$(LIBRARY).o_shipped : $(OBJS_ST40_LINUX_KO)
	$(LIB_ST40_LINUX_KO) $(LIBFLAGS_ST40_LINUX_KO) $@ $(OBJS_ST40_LINUX_KO)

$(OBJDIR_ST40_LINUX_KO)/$(LIBRARY).mod.c : $(OBJDIR_ST40_LINUX_KO)/$(LIBRARY).o
	-$(MODPOST_ST40_LINUX_KO) $(MODPOSTFLAGS_ST40_LINUX_KO) $<

$(OBJDIR_ST40_LINUX_KO)/$(LIBRARY).mod.$(OBJ_ST40_LINUX_KO) : $(OBJDIR_ST40_LINUX_KO)/$(LIBRARY).mod.c
	$(CC_ST40_LINUX_KO) -c $(CFLAGS_ST40_LINUX_KO) $< -o $@

$(OBJDIR_ST40_LINUX_KO)/$(LIBRARY).ko : $(OBJDIR_ST40_LINUX_KO)/$(LIBRARY).o \
                              $(OBJDIR_ST40_LINUX_KO)/$(LIBRARY).mod.$(OBJ_ST40_LINUX_KO)
	$(LIB_ST40_LINUX_KO) $(LIBFLAGS_ST40_LINUX_KO) $@ $^

endif 

endif # OBJS_ST40_LINUX_KO

MAKE_TARGETS            += $(TARGET_FOR_ST40_LINUX_KO)

endif # ENABLE_ST40_LINUX_KO

