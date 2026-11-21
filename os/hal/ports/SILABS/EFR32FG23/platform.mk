#
#    ChibiOS - Copyright (C) 2026 Xael South.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

# Required platform files.
PLATFORMSRC_CONTRIB := \
               $(CHIBIOS)/os/hal/ports/common/ARMCMx/nvic.c \
               $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/EFR32FG23/hal_lld.c

# Required include directories.
PLATFORMINC_CONTRIB := \
               $(CHIBIOS)/os/hal/ports/common/ARMCMx \
               $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/EFR32FG23 \
               ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23 \
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
# System tick: SYSTICKv2 (SYSRTC-based, 32-bit, EM4-capable) is the default.
# Alternative: SYSTICKv3 (LETIMER0, 24-bit, EM2) — swap the include below.
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/SYSTICKv2/driver.mk
# include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/SYSTICKv3/driver.mk   # LETIMER0-based alternative
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/LETIMERv1/driver.mk   # LETIMER0 as GPT (HAL_USE_GPT + EFR32_LETIMER_GPT_USE_LETIMER0)
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/GPIOv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/EUSARTv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/BURTCv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/BURAMv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/AESv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/ADCv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/DACv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/DMAv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/TIMERv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/I2Cv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/SPIv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/SYSRTCv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/WDOGv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/LESENSEv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/CRCv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/COMPv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/QEIv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/RAILv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/LCDv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/EFLv1/driver.mk
# emlib binding (optional, for apps that use emlib directly without RAIL).
# When EFR32_RAIL_USE_RAIL=TRUE, RAILv1 already provides the same shims.

# Shared variables
ALLCSRC += $(PLATFORMSRC_CONTRIB)
ALLINC  += $(PLATFORMINC_CONTRIB)
