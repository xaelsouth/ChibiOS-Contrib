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
               $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/EFR32FG23/hal_lld.c \
               $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/EFR32FG23/efr32_clocks.c

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
# System tick driver selection:
#   default                  -> SYSTICKv2 (SYSRTC0-based, native 32-bit counter,
#                               LF-clocked so it keeps running in EM2/EM3, two
#                               compare channels, own SEQ vector — no conflict
#                               with the BURTC calendar)
#   EFR32_USE_SYSTICKV1=yes  -> SYSTICKv1 (ARM core SysTick, 24-bit, stops in
#                               EM2/EM3, supports PERIODIC and FREERUNNING)
# LETIMER0 is NOT used as systick (24-bit counter needs SW extension and it is
# better kept free for low-power one-shot timing); BURTC is NOT used as
# systick either (single COMP channel + shared vector with the BURTCv1 RTC
# calendar) — BURTC is the backup-domain timekeeper.
ifeq ($(EFR32_USE_SYSTICKV1),yes)
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/SYSTICKv1/driver.mk   # core SysTick (optional)
else
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/SYSTICKv2/driver.mk   # SYSRTC0-based system tick (default)
endif
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
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/TEMPv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/KEYSCANv1/driver.mk
include ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/SIGROUTEv1/driver.mk
# emlib binding (optional, for apps that use emlib directly without RAIL).
# When EFR32_RAIL_USE_RAIL=TRUE, RAILv1 already provides the same shims.

# Shared variables
ALLCSRC += $(PLATFORMSRC_CONTRIB)
ALLINC  += $(PLATFORMINC_CONTRIB)
