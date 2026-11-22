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

# MXS-family audioout (mxs-builtin DAC, playback via APBX DMA):
# i.MX23 / i.MX28 / STMP37xx. The DMA engine itself stays port-level
# (picked up via __has_include("imx23_dma.h") on the i.MX23 port).
MXSAUDIOCSRC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/audio/mxs_audioout.c
MXSAUDIOINC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/audio

ALLCSRC += $(MXSAUDIOCSRC)
ALLINC += $(MXSAUDIOINC)
