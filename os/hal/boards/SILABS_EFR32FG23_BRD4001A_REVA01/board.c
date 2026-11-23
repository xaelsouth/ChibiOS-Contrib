/*
    ChibiOS - Copyright (C) 2023 Xael South

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "hal.h"

/**
 * @brief   PAL setup — the board's static default GPIO configuration.
 * @details halInit() calls palInit(&pal_default_config) (hal.c), and the PAL
 *          LLD applies this table once in _pal_lld_init(): each PAData..PDData
 *          entry programs the corresponding GPIO port registers
 *          (CTRL/MODEL/MODEH/DOUT), i.e. the reset-time mode and output level
 *          of every pin of that port.  Without it the pins would simply keep
 *          their hardware reset state.
 *          The values come from the PAL_EFR32_PORTx_{CTRL,MODEL,MODEH,DOUT}_DEFAULT
 *          macros in hal_pal_lld.h, which default every pin to
 *          PAL_EFR32_MODEL_DEFAULT (input, output driver disabled).  To give
 *          a board pin a different power-on state (e.g. LED pin as push-pull
 *          output, button pin with pull-up), redefine the matching macro in
 *          board.h — this table picks it up automatically.
 *          Runtime changes afterwards are done per pin with palSetPadMode() /
 *          palSetPad() / palClearPad(), not through this const table.
 */
#if HAL_USE_PAL || defined(__DOXYGEN__)
const PALConfig pal_default_config =
{
  .PAData = {
    .ctrlr =  PAL_EFR32_PORTA_CTRL_DEFAULT,
    .modelr = PAL_EFR32_PORTA_MODEL_DEFAULT,
    .modehr = PAL_EFR32_PORTA_MODEH_DEFAULT,
    .doutr =  PAL_EFR32_PORTA_DOUT_DEFAULT
  },

  .PBData = {
    .ctrlr =  PAL_EFR32_PORTB_CTRL_DEFAULT,
    .modelr = PAL_EFR32_PORTB_MODEL_DEFAULT,
    .modehr = PAL_EFR32_PORTB_MODEH_DEFAULT,
    .doutr =  PAL_EFR32_PORTB_DOUT_DEFAULT
  },

  .PCData = {
    .ctrlr =  PAL_EFR32_PORTC_CTRL_DEFAULT,
    .modelr = PAL_EFR32_PORTC_MODEL_DEFAULT,
    .modehr = PAL_EFR32_PORTC_MODEH_DEFAULT,
    .doutr =  PAL_EFR32_PORTC_DOUT_DEFAULT
  },

  .PDData = {
    .ctrlr =  PAL_EFR32_PORTD_CTRL_DEFAULT,
    .modelr = PAL_EFR32_PORTD_MODEL_DEFAULT,
    .modehr = PAL_EFR32_PORTD_MODEH_DEFAULT,
    .doutr =  PAL_EFR32_PORTD_DOUT_DEFAULT
  },
};
#endif

/*
 * Early initialization code.
 * This initialization must be performed just after stack setup and before
 * any other initialization.
 */
void __early_init(void) {

  efr32_escape_hatch();
  efr32_chip_init();
  efr32_clock_init();
}

/*
 * Board-specific initialization code.
 */
void boardInit(void) {

}
