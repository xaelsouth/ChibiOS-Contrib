##############################################################################
# Build settings
##############################################################################

# Project name.
PROJECT = ch

# Board selection: IMX23_OLIMEX_IMX233 (64MB) or IMX23_EVK (128MB).
# The board's board.h defines IMX23_RAM_SIZE, which sizes the memory map.
BOARD ?= IMX23_OLIMEX_IMX233

# Imported source files and paths.
CHIBIOS := ../../../../ChibiOS
CHIBIOS_CONTRIB := $(CHIBIOS)/../ChibiOS-Contrib

# Licensing files.
include $(CHIBIOS)/os/license/license.mk

# Startup files (i.MX23 device: armparams.h, __early_init, ld script).
include $(CHIBIOS)/os/common/startup/ARM/compilers/GCC/mk/startup_imx23.mk

# HAL-OSAL files (the Contrib hal.mk includes the main one).
include $(CHIBIOS_CONTRIB)/os/hal/hal.mk
include $(CHIBIOS)/os/hal/osal/rt-nil/osal.mk

# HAL platform (i.MX23) and board.
include $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/platform.mk
include $(CHIBIOS_CONTRIB)/os/hal/boards/$(BOARD)/board.mk

# RTOS files.
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/common/ports/ARM/compilers/GCC/mk/port_generic.mk

# Other files.
include $(CHIBIOS)/tools/mk/autobuild.mk

##############################################################################
# Project, sources and paths
##############################################################################

# Define linker script file here.
# IMX23.ld is preprocessed by the C preprocessor so that the board.h
# #defines (IMX23_RAM_SIZE etc.) configure the memory regions.
LDSCRIPT = $(BUILDDIR)/IMX23.ld

# C sources that can be compiled in ARM or THUMB mode.
CSRC = $(ALLCSRC) \
       main.c

# C++ sources.
CPPSRC = $(ALLCPPSRC)

# C sources compiled in ARM mode.
ACSRC =

# C++ sources compiled in ARM mode.
ACPPSRC =

# C sources compiled in THUMB mode.
TCSRC =

# C++ sources compiled in THUMB mode.
TCPPSRC =

# List ASM source files here.
ASMSRC = $(ALLASMSRC)

# List ASM with preprocessor source files here.
ASMXSRC = $(ALLXASMSRC)

# Inclusion directories.
INCDIR = $(CONFDIR) $(ALLINC)

##############################################################################
# Compiler settings
##############################################################################

# ARM926EJ-S: ARMv5TE, ARM mode, soft float (no FPU).
MCU  = arm926ej-s

TRGT = arm-none-eabi-
CC   = $(TRGT)gcc
CPPC = $(TRGT)g++
LD   = $(TRGT)gcc
CP   = $(TRGT)objcopy
AS   = $(TRGT)gcc -x assembler-with-cpp
AR   = $(TRGT)ar
OD   = $(TRGT)objdump
SZ   = $(TRGT)size
HEX  = $(CP) -O ihex
BIN  = $(CP) -O binary

# Build global options.
USE_OPT = -O2 -ggdb -fomit-frame-pointer -falign-functions=16 -mfloat-abi=soft
USE_COPT =
USE_CPPOPT = -fno-rtti
USE_LINK_GC = yes
USE_LDOPT =
USE_LTO = no
USE_THUMB = no
USE_VERBOSE_COMPILE = no
USE_SMART_BUILD = yes

# Stack sizes (bytes) for the ARM exception stacks.
USE_SYSTEM_STACKSIZE = 0x800
USE_PROCESS_STACKSIZE = 0x800
USE_IRQ_STACKSIZE = 0x400
USE_FIQ_STACKSIZE = 0x100
USE_ABT_STACKSIZE = 0x100
USE_UND_STACKSIZE = 0x100
USE_SVC_STACKSIZE = 0x100

# ARM-specific options here.
AOPT =
TOPT = -marm -DTHUMB
CWARN = -Wall -Wextra -Wundef -Wstrict-prototypes
CPPWARN = -Wall -Wextra -Wundef
UDEFS =
UADEFS =
UINCDIR =
ULIBDIR =
ULIBS =

RULESPATH = $(CHIBIOS)/os/common/startup/ARM/compilers/GCC

##############################################################################
# Build rules
##############################################################################

# BUILDDIR is normally set by rules.mk; define it early for LDSCRIPT below.
ifeq ($(BUILDDIR),)
  BUILDDIR = build/$(BOARD)
endif

include $(RULESPATH)/rules.mk

##############################################################################
# Linker script preprocessing: board.h #defines -> memory layout.
# NOTE: this rule must stay after the rules.mk include so that "all" remains
# the default goal.
##############################################################################

IMX23_DEV_DIR = $(CHIBIOS)/os/common/startup/ARM/devices/IMX23

$(BUILDDIR)/IMX23.ld: $(STARTUPLD)/IMX23.ld $(BOARDINC)/board.h
	@echo "Preprocessing linker script for board $(BOARD)"
	@mkdir -p $(BUILDDIR)
	$(TRGT)gcc -E -P -x c -D__IMX23_LDSCRIPT__ \
	  -I$(BOARDINC) -I$(IMX23_DEV_DIR) \
	  $(STARTUPLD)/IMX23.ld -o $@

##############################################################################
# Custom rules
##############################################################################

# Load with the bootstream (ROM USB boot): the .bin is written to
# 0x40000000 and the ROM CALLs it. Adjust to your bootstream tool.
upload: $(BUILDDIR)/$(PROJECT).bin
	@echo "Load $(BUILDDIR)/$(PROJECT).bin at 0x40000000 and jump (bootstream)"

.PHONY: upload
