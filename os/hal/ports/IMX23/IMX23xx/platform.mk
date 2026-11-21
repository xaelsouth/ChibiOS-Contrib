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

# List of all the i.MX23 platform files.
PLATFORMSRC_CONTRIB := \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/hal_lld.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/imx23_dma.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/imx23_dcp.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/imx23_clock.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/imx23_lcdif.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/imx23_pxp.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/imx23_audioout.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/hal_spi_lld.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/hal_i2c_lld.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/icoll.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/hal_st_lld.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/hal_sio_lld.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/hal_pal_lld.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/hal_pwm_lld.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/hal_usb_lld.c

PLATFORMASM_CONTRIB := \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/icoll_tramp.S

# Required include directories.
PLATFORMINC_CONTRIB := \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx

# Shared variables
ALLCSRC   += $(PLATFORMSRC_CONTRIB)
ALLXASMSRC += $(PLATFORMASM_CONTRIB)
ALLINC    += $(PLATFORMINC_CONTRIB)
