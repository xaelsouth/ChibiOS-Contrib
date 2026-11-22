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

# MXS-family DCP (Data Co-Path) hardware crypto: AES-128 ECB/CBC,
# SHA-1/SHA-256, OTP/slot/device-unique keys (i.MX23 / i.MX28 / STMP37xx).
MXSDCPCSRC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/crypto/mxs_dcp.c
MXSDCPINC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/crypto

ALLCSRC += $(MXSDCPCSRC)
ALLINC += $(MXSDCPINC)
