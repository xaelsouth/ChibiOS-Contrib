##############################################################################
# testhal/SILABS/EFR32FG23x0xx/SIO
##############################################################################

ifeq ($(USE_OPT),)
  USE_OPT = -O2 -ggdb -fomit-frame-pointer -falign-functions=16
endif
ifeq ($(USE_COPT),)
  USE_COPT =
endif
ifeq ($(USE_CPPOPT),)
  USE_CPPOPT = -fno-rtti
endif
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC = yes
endif
ifeq ($(USE_LDOPT),)
  USE_LDOPT =
endif
ifeq ($(USE_LTO),)
  USE_LTO = yes
endif
ifeq ($(USE_VERBOSE_COMPILE),)
  USE_VERBOSE_COMPILE = no
endif
ifeq ($(USE_SMART_BUILD),)
  USE_SMART_BUILD = yes
endif

ifeq ($(USE_PROCESS_STACKSIZE),)
  USE_PROCESS_STACKSIZE = 0x1000
endif
ifeq ($(USE_EXCEPTIONS_STACKSIZE),)
  USE_EXCEPTIONS_STACKSIZE = 0x1000
endif
ifeq ($(USE_FPU),)
  USE_FPU = no
endif
ifeq ($(USE_FPU_OPT),)
  USE_FPU_OPT = -mfloat-abi=$(USE_FPU) -mfpu=fpv5-sp-d16
endif

BUILDDIR := ./build
DEPDIR   := ./.dep

PROJECT = ch

# Imported source files and paths
CHIBIOS = ../../../../../ChibiOS
CHIBIOS_CONTRIB = $(CHIBIOS)/../ChibiOS-Contrib

include $(CHIBIOS)/os/license/license.mk
include $(CHIBIOS_CONTRIB)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_efr32fg23x.mk
include $(CHIBIOS_CONTRIB)/os/hal/hal.mk
include $(CHIBIOS)/os/hal/osal/rt-nil/osal.mk
include $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/EFR32FG23/platform.mk
include $(CHIBIOS_CONTRIB)/os/hal/boards/SILABS_EFR32FG23_DK2600A/board.mk
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/common/ports/ARMv8-M-ML/compilers/GCC/mk/port.mk
include $(CHIBIOS)/os/hal/lib/streams/streams.mk

# Define linker script file here
LDSCRIPT = $(STARTUPLD_CONTRIB)/EFR32FG23XF512.ld

GECKO_SDK = $(CHIBIOS_CONTRIB)/ext/gecko_sdk
RAILEMLIBINC = \
       $(GECKO_SDK)/platform/emlib/inc \
       $(GECKO_SDK)/platform/common/inc

CSRC = $(ALLCSRC) \
       $(CHIBIOS)/os/various/newlib_bindings/syscalls.c \
       main.c

CPPSRC = $(ALLCPPSRC)
ASMSRC = $(ALLASMSRC)
ASMXSRC = $(ALLXASMSRC)

INCDIR = . $(ALLINC) $(CHIBIOS)/os/hal/lib/streams $(CHIBIOS_CONTRIB)/os/various $(RAILEMLIBINC)

CWARN = -Wall -Wextra -Wundef -Wstrict-prototypes -Wcast-align=strict
CPPWARN = -Wall -Wextra -Wundef

UDEFS = -DEFR32FG23B010F512IM48=1 \
        -DCORTEX_ENABLE_WFI_IDLE \
        -DESCAPE_HATCH_ENABLE=TRUE \
        -DSL_TRUSTZONE_SECURE \
        -DSYSTEM_NO_STATIC_MEMORY \
        -D_SILICON_LABS_32B_SERIES_1_CONFIG=0 \
        -DSL_RAIL_UTIL_PROTOCOL_IEEE802154_GB868_ENABLE=0 \
        -DFEAT_RF_2G4=0 \
        -DFEAT_RF_SUBG=1

UADEFS =

UINCDIR = $(CHIBIOS_CONTRIB)/ext/gecko_sdk/platform/radio/rail_lib/common \
          $(CHIBIOS_CONTRIB)/ext/gecko_sdk/platform/radio/rail_lib/chip/efr32/efr32xg2x \
          $(GECKO_SDK)/app/common/util/app_assert \
          $(GECKO_SDK)/app/common/util/app_assert/config

ULIBDIR = $(CHIBIOS_CONTRIB)/ext/gecko_sdk/platform/radio/rail_lib/autogen/librail_release
ULIBS = -l:librail_efr32xg23_gcc_release.a

MCU  = cortex-m33

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
SREC = $(CP) -O srec

RULESPATH = $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk
include $(RULESPATH)/arm-none-eabi.mk
include $(RULESPATH)/rules.mk

# Flash with J-Link: make flash
flash: $(BUILDDIR)/$(PROJECT).bin
	@printf 'device EFR32FG23B010F512IM48\nif SWD\nspeed 4000\nr\nloadbin $(BUILDDIR)/$(PROJECT).bin, 0x08000000\nr\ng\nq\n' > $(BUILDDIR)/flash.jlink
	JLinkExe -CommanderScript $(BUILDDIR)/flash.jlink
