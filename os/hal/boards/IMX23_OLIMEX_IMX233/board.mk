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

# List of all the board related files.
BOARDSRC = ${CHIBIOS_CONTRIB}/os/hal/boards/IMX23_OLIMEX_IMX233/board.c

# Required include directories
BOARDINC = ${CHIBIOS_CONTRIB}/os/hal/boards/IMX23_OLIMEX_IMX233

# Shared variables
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)
