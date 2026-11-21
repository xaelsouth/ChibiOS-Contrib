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

ifeq ($(USE_SMART_BUILD),yes)

ifneq ($(findstring HAL_USE_SERIAL TRUE,$(HALCONF)),)
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/EUSARTv1/hal_serial_lld.c
endif

ifneq ($(findstring HAL_USE_UART TRUE,$(HALCONF)),)
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/EUSARTv1/hal_uart_lld.c
endif

ifneq ($(findstring HAL_USE_SIO TRUE,$(HALCONF)),)
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/EUSARTv1/hal_sio_lld.c
endif

ifneq ($(findstring HAL_USE_UART TRUE,$(HALCONF)),)
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/EUSARTv1/hal_eusart_irq.c
else
ifneq ($(findstring HAL_USE_SIO TRUE,$(HALCONF)),)
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/EUSARTv1/hal_eusart_irq.c
endif
endif

else
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/EUSARTv1/hal_serial_lld.c
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/EUSARTv1/hal_uart_lld.c
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/EUSARTv1/hal_sio_lld.c
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/EUSARTv1/hal_eusart_irq.c
endif

PLATFORMINC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/EUSARTv1
