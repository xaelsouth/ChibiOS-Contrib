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

# MXS-family display pipeline (i.MX23 / i.MX28 / STMP37xx):
#  - mxs_lcdif.c: LCDIF dotclock/RGB-mode framebuffer driver
#  - mxs_pxp.c:   PXP pixel pipeline (e.g. RGB565 -> XRGB8888 mem2mem)
MXSDISPLAYCSRC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/display/mxs_lcdif.c \
                 $(CHIBIOS_CONTRIB)/os/various/devices_lib/display/mxs_pxp.c
MXSDISPLAYINC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/display

ALLCSRC += $(MXSDISPLAYCSRC)
ALLINC += $(MXSDISPLAYINC)
