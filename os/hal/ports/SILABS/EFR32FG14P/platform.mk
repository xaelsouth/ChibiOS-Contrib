# Required platform files.
PLATFORMSRC_CONTRIB := \
               $(CHIBIOS)/os/hal/ports/common/ARMCMx/nvic.c \
               $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/EFR32FG14P/hal_lld.c \
               $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/EFR32FG14P/efr32_clocks.c

# Required include directories.
PLATFORMINC_CONTRIB := \
               $(CHIBIOS)/os/hal/ports/common/ARMCMx \
               $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/EFR32FG14P \
               ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG14P \
               $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/common

# Optional platform files.
ifeq ($(USE_SMART_BUILD),yes)

# Configuration files directory
ifeq ($(HALCONFDIR),)
  ifeq ($(CONFDIR),)
    HALCONFDIR = .
  else
    HALCONFDIR := $(CONFDIR)
  endif
endif

HALCONF := $(strip $(shell cat $(CONFDIR)/halconf.h $(CONFDIR)/halconf_community.h | egrep -e "\#define"))

else
endif

# Drivers compatible with the platform.
# NOTE: SYSTICKv1 (Cortex-M SysTick) is NOT included here — FG14P uses its
# own SYSTICKv3 (LETIMER-based, supports EM2 low-power).
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG14P/SYSTICKv3/driver.mk

# Shared variables
ALLCSRC += $(PLATFORMSRC_CONTRIB)
ALLINC  += $(PLATFORMINC_CONTRIB)
