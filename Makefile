# Program wide settings
EXE       := protura
VERSION   := 0
SUBLEVEL  := 6
PATCH     := 0

ARCH   := x86
BITS   := 32

PROTURA_DIR := $(PWD)

# TARGET := i686-elf-
TARGET := i686-protura
TOOLCHAIN_DIR := $(PROTURA_DIR)/toolchain

# Compiler settings
CC      := $(TARGET)-gcc
CPP     := $(TARGET)-cpp
LD      := $(TARGET)-ld
AS      := $(TARGET)-gas
PERL    := perl -w -Mdiagnostics
MKDIR   := mkdir
OBJCOPY := $(TARGET)-objcopy

EXTRA := $(shell ./scripts/version_tag.sh)

VERSION_FULL := $(VERSION).$(SUBLEVEL).$(PATCH)$(EXTRA)

$(info Building: $(EXE))
$(info Arch:     $(ARCH))
$(info Version:  $(VERSION_FULL))

CPPFLAGS  = -DPROTURA_VERSION=$(VERSION)              \
            -DPROTURA_SUBLEVEL=$(SUBLEVEL)            \
            -DPROTURA_PATCH=$(PATCH)                  \
            -DPROTURA_EXTRA="$(EXTRA)"                \
            -DPROTURA_VERSION_FULL="$(VERSION_FULL)"  \
            -DPROTURA_ARCH="$(ARCH)"                  \
            -DPROTURA_BITS=$(BITS)                    \
			-D__KERNEL__                              \
            -I'./include' -I'./arch/$(ARCH)/include'

CFLAGS  := -Wall -O2 -std=gnu99 -ffreestanding \
           -fno-strict-aliasing -nostdlib -fno-builtin -nostdinc

LDFLAGS := -nostdlib -O2 -ffreestanding -lgcc -static-libgcc
ASFLAGS := -DASM -Wall -ffreestanding -nostdlib

# Configuration -- Uncomment lines to enable option
# Or specify on the commandline

# Enable debugging
PROTURA_DEBUG := y

# Show all commands executed by the Makefile
# V := y

# Tree is the directory currently being processed
# This changes when decending into directories
tree := .

# Define 'conf' if you want to use a conf file other then
# $(tree)/protura.conf
ifdef conf
	CONFIG_FILE := $(conf)
else
	CONFIG_FILE := $(tree)/protura.conf
	conf := $(CONFIG_FILE)
endif

EXE_OBJ := $(tree)/$(EXE).o

# This is our default target - The default is the first target in the file so
# we need to define this fairly high-up.
all: real-all

PHONY += all install clean dist qemu-test real-all

# This variable defines any extra targets that should be build by 'all'
EXTRA_TARGETS :=

# This is the internal list of objects to compile into the file .o
REAL_OBJS_y :=

# Predefine this variable. It contains a list of extra files to clean. Ex.
CLEAN_LIST :=

# Set configuration options
ifdef V
	Q :=
else
	Q := @
endif

ifdef PROTURA_DEBUG
	CPPFLAGS += -DPROTURA_DEBUG
	CFLAGS +=  -ggdb -gdwarf-2
	ASFLAGS += -ggdb -gdwarf-2
	LDFLAGS +=
endif

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),clean-configure)
ifneq ($(wildcard $(CONFIG_FILE)),$(CONFIG_FILE))
$(error Configuration file $(tree)/protura.conf does not exist. Please create this file before running make or specify different config file via conf variable on commandline)
endif
endif
endif

# We don't include config.mk if we're just going to delete it anyway
ifneq ($(MAKECMDGOALS),clean-configure)
ifneq ($(MAKECMDGOALS),clean-full)
_tmp := $(shell mkdir -p $(tree)/include/protura/config)
# Note - Including a file that doesn't exist provokes make to check for a rule
# This line actually runs the $(tree)/config.mk rule and generates config.mk
# before including it and continuing.
-include $(tree)/config.mk
endif
endif

ifeq ($(CONFIG_FRAME_POINTER),y)
CFLAGS += -fno-omit-frame-pointer
endif

# This includes everything in the 'include' folder of the $(tree)
# This is so that the code can reference generated include files
CPPFLAGS += -I'$(tree)/include/'

make_name = $(subst /,_,$(basename $(tree)/$1))

define add_dep
$(1): $(2)
endef

define create_rule
$(1): $(2)
	@echo " $(3)$$@"
	$(4)
endef

# Traverse into tree
define subdir_inc
tree := $$(tree)/$(1)

pfix := $$(subst /,_,$$(tree))_
subdir-y :=
objs-y :=
clean-list-y :=

_tmp := $$(shell mkdir -p $$(tree))
include $$(tree)/Makefile

REAL_OBJS_y += $$(patsubst %,$$(tree)/%,$$(objs-y))
CLEAN_LIST += $$(patsubst %,$$(tree)/%,$$(clean-list-y))

$$(foreach subdir,$$(subdir-y),$$(eval $$(call subdir_inc,$$(subdir))))

tree := $$(patsubst %/$(1),%,$$(tree))
pfix := $$(subst /,_,$$(tree))_
endef


# Include the base directories for source files - That is, the generic 'src'
# directory as well as the 'arch/$(ARCH)' directory
$(eval $(call subdir_inc,src))
$(eval $(call subdir_inc,arch/$(ARCH)))

# This is a list of targets to create bootable images from
# Ex. For x86, there is a multiboot compliant image, also a full realmode image
# that can boot from a floppy.
BOOT_TARGETS :=

imgs_pfix := $(tree)/imgs/$(EXE)_$(ARCH)_

$(eval $(call subdir_inc,arch/$(ARCH)/boot))

_tmp := $(shell mkdir -p $(tree)/imgs)

define compile_file

_tmp := $$(subst /,_,$$(basename $(1)))_y
ifdef $$(_tmp)

CLEAN_LIST += $$($$(_tmp))

$(1): $$($(_tmp))
	@echo " LD      $$@"
	$$(Q)$$(LD) -r -o $$@ $$($$(_tmp))

endif

endef

REAL_BOOT_TARGETS :=
DEP_LIST :=

define create_boot_target

_expand := $$(tree)/imgs/$$(EXE)_$$(ARCH)_$(1)
REAL_BOOT_TARGETS += $$(_expand)

CLEAN_LIST += $$(OBJS_$(1))
DEP_LIST += $$(EXTRAS_$(1))

$$(_expand): $$(EXE_OBJ) $$(OBJS_$(1)) $$(EXTRAS_$(1))
	@echo " CCLD    $$@"
	$$(Q)$$(CC) $$(CPPFLAGS) -o $$@ $$(OBJS_$(1)) $$(EXE_OBJ) $$(LDFLAGS) $$(LDFLAGS_$(1)) 
	@echo " COPY    $$@.full"
	$$(Q)cp $$@ $$@.full
	@echo " OBJCOPY $$@.sym"
	$$(Q)$$(OBJCOPY) --only-keep-debug $$@ $$@.sym
	@echo " OBJCOPY $$@"
	$$(Q)$$(OBJCOPY) --strip-unneeded $$@

endef

$(foreach file,$(REAL_OBJS_y),$(eval $(call compile_file,$(file))))

DEP_LIST += $(REAL_OBJS_y)
DEP_LIST := $(foreach file,$(DEP_LIST),$(dir $(file)).$(notdir $(file)))
DEP_LIST := $(DEP_LIST:.o=.d)
DEP_LIST := $(DEP_LIST:.ld=.d)

ifneq (,$(filter $(MAKECMDGOALS),kernel disk check))
-include $(DEP_LIST)
endif

CLEAN_LIST += $(DEP_LIST)

$(foreach btarget,$(BOOT_TARGETS),$(eval $(call create_boot_target,$(btarget))))

# Actual default entry
real-all:
	@echo "Please run make with one of 'configure', 'kernel', 'toolchain', or 'disk'"
	@echo "See README.md for more information"

PHONY += kernel
kernel: configure $(REAL_BOOT_TARGETS) $(EXTRA_TARGETS)

PHONY += configure
configure: $(tree)/config.mk $(tree)/include/protura/config/autoconf.h

PHONY += clean-configure
clean-configure:
	@echo " RM      $(tree)/config.mk"
	$(Q)$(RM) -fr $(tree)/config.mk
	@echo " RM      $(tree)/include/protura/config/autoconf.h"
	$(Q)$(RM) -fr $(tree)/include/protura/config/autoconf.h

$(tree)/config.mk: $(CONFIG_FILE) $(tree)/scripts/genconfig.pl
	@echo " PERL    $@"
	$(Q)$(PERL) $(tree)/scripts/genconfig.pl make < $< > $@

$(tree)/include/protura/config/autoconf.h: $(CONFIG_FILE) $(tree)/scripts/genconfig.pl
	@echo " PERL    $@"
	$(Q)$(PERL) $(tree)/scripts/genconfig.pl cpp < $< > $@

dist: clean-kernel clean-toolchain clean-configure clean-disk
	$(Q)mkdir -p $(EXE)-$(VERSION_N)
	$(Q)cp -R Makefile README.md config.mk LICENSE ./doc ./include ./src ./test $(EXE)-$(VERSION_N)
	$(Q)tar -cf $(EXE)-$(VERSION_N).tar $(EXE)-$(VERSION_N)
	$(Q)gzip $(EXE)-$(VERSION_N).tar
	$(Q)rm -fr $(EXE)-$(VERSION_N)
	@echo " Created $(EXE)-$(VERSION_N).tar.gz"

PHONY += clean-kernel
clean-kernel:
	$(Q)for file in $(REAL_OBJS_y) $(CLEAN_LIST) $(EXE_OBJ) $(tree)/imgs; do \
		if [ -e $$file ]; then \
		    echo " RM      $$file"; \
			rm -rf $$file; \
		fi \
	done

PHONY += clean-full
clean-full: clean-toolchain clean-configure clean-disk clean-kernel

full: configure install-kernel-headers toolchain kernel disk

clean:
	@echo " Please use one of 'clean-configure', 'clean-toolchain',"
	@echo " 'clean-kernel', 'clean-disk', or 'clean-full'"

$(EXE_OBJ): $(REAL_OBJS_y)
	@echo " LD      $@"
	$(Q)$(LD) -r $(REAL_OBJS_y) -o $@

$(tree)/%.o: $(tree)/%.c
	@echo " CC      $@"
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) $(CFLAGS_$(make_name $@)) -c $< -o $@

$(tree)/%.o: $(tree)/%.S
	@echo " CCAS    $@"
	$(Q)$(CC) $(CPPFLAGS) $(ASFLAGS) $(ASFLAGS_$(make_name $@)) -o $@ -c $<

$(tree)/%.ld: $(tree)/%.ldS
	@echo " CPP     $@"
	$(Q)$(CPP) -P $(CPPFLAGS) $(ASFLAGS) -o $@ -x c $<

$(tree)/.%.d: $(tree)/%.ldS
	@echo " CPPDEP   $@"
	$(Q)$(CPP) -MM -MP -MF $@ $(CPPFLAGS) $(ASFLAGS) $< -MT $(tree)/%.ld -MT $@

$(tree)/.%.d: $(tree)/%.c
	@echo " CCDEP   $@"
	$(Q)$(CC) -MM -MP -MF $@ $(CPPFLAGS) $< -MT $(tree)/$*.o -MT $@

$(tree)/.%.d: $(tree)/%.S
	@echo " CCDEP   $@"
	$(Q)$(CC) -MM -MP -MF $@ $(CPPFLAGS) $< -MT $(tree)/$*.o -MT $@

install-kernel-headers: | ./disk/root/usr/$(TARGET)/include
	@echo " CP      include"
	$(Q)cp -r ./include/* ./disk/root/usr/$(TARGET)/include/
	@echo " CP      arch/$(ARCH)/include"
	$(Q)cp -r ./arch/$(ARCH)/include/* ./disk/root/usr/$(TARGET)/include/
	@echo " LN      include"
	$(Q)ln -fs ./$(TARGET)/include ./disk/root/usr/include
	@echo " LN      lib"
	$(Q)ln -fs ./$(TARGET)/lib ./disk/root/usr/lib

clean-kernel-headers:
	@echo " RMDIR   include/protura"
	$(Q)rm -fr ./disk/root/usr/$(TARGET)/include/protura

PHONY += cscope
cscope:
	@echo " Generating cscope for arch $(ARCH)" 
	$(Q)find ./ \
		-path "./arch/*" ! -path "./arch/$(ARCH)/*" -prune -o \
		-path "./scripts/*" -prune -o \
		-path "./disk/*" -prune -o \
		-name "*.[chsS]" -print \
		> ./cscope.files
	$(Q)cscope -b -q -k

include ./disk/Makefile

.PHONY: $(PHONY)

