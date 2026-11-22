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

# Generic NAND support (devices_lib/nand):
#  - nand_chips.c: vendor/JEDEC chip database (controller-agnostic)
#  - nand_lfs.c:   LittleFS over any MTD-style NAND logical block device
# The NAND controller driver itself (e.g. GPMI+BCH on i.MX23) lives with
# the port it belongs to.
NANDCSRC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/nand/nand_chips.c \
           $(CHIBIOS_CONTRIB)/os/various/devices_lib/nand/nand_lfs.c
NANDINC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/nand

ALLCSRC += $(NANDCSRC)
ALLINC += $(NANDINC)
