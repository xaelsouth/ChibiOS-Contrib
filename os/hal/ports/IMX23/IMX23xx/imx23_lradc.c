/*
    ChibiOS - Copyright (C) 2026 Xael South.

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

/**
 * @file    IMX23/IMX23xx/imx23_lradc.c
 * @brief   i.MX23 LRADC driver (polled channel reads + 4-wire touch).
 *
 * @addtogroup IMX23
 * @{
 */

#include "hal.h"

#include "imx23_lradc.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

#define IMX23_REG32(addr)   (*(volatile uint32_t *)(addr))

/* Conversion scheduling: logical channel 0 via DELAY0. */
#define TS_DELAY_US         100U        /* DELAY0 delay field (us-ish)      */

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/* Run one conversion of logical channel 0 (source selected in CTRL4)
   and return the 18-bit result. */
static uint32_t lradc_convert(uint8_t pad, sysinterval_t timeout) {
  uint32_t v;
  systime_t t0 = chVTGetSystemTimeX();

  /* Route the requested pad to logical channel 0. */
  IMX23_REG32(IMX23_LRADC_CTRL4) = IMX23_LRADC_CTRL4_SELECT(0U, pad);
  /* Clear any pending channel-0 IRQ, schedule channel 0, kick DELAY0. */
  IMX23_REG32(IMX23_LRADC_CTRL1) = IMX23_LRADC_CTRL1_LRADC_IRQ(0U);
  IMX23_REG32(IMX23_LRADC_CTRL0) = (IMX23_REG32(IMX23_LRADC_CTRL0) &
                                     ~IMX23_LRADC_CTRL0_SCHEDULE_MASK) |
                                    0x1U;
  IMX23_REG32(IMX23_LRADC_DELAY(0U)) = TS_DELAY_US |
                                        IMX23_LRADC_DELAY_TRIGGER(0U) |
                                        IMX23_LRADC_DELAY_KICK;
  /* Poll the channel-0 conversion-done IRQ flag. */
  do {
    if ((IMX23_REG32(IMX23_LRADC_CTRL1) & IMX23_LRADC_CTRL1_LRADC_IRQ(0U)) != 0U) {
      break;
    }
    if (chVTTimeElapsedSinceX(t0) > timeout) {
      return 0U;
    }
    chThdSleepMicroseconds(50U);
  } while (true);

  v = IMX23_REG32(IMX23_LRADC_CH(0U)) & IMX23_LRADC_CH_VALUE_MASK;
  IMX23_REG32(IMX23_LRADC_CTRL1) = IMX23_LRADC_CTRL1_LRADC_IRQ(0U);  /* ack */
  return v;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void imx23_lradc_init(void) {

  /* Release reset, ungate the clock. */
  IMX23_REG32(IMX23_LRADC_CTRL0) = IMX23_LRADC_CTRL0_SFTRST |
                                    IMX23_LRADC_CTRL0_CLKGATE;
  IMX23_REG32(IMX23_LRADC_CTRL0) = 0U;
  /* 4-wire touch screen type + touch detect enable. */
  IMX23_REG32(IMX23_LRADC_CTRL0) = IMX23_LRADC_CTRL0_TOUCH_SCREEN_TYPE |
                                   IMX23_LRADC_CTRL0_TOUCH_DETECT_EN;
  /* Route logical channel 0 to pad 0 by default. */
  IMX23_REG32(IMX23_LRADC_CTRL4) = IMX23_LRADC_CTRL4_SELECT(0U, 0U);
}

uint32_t imx23_lradc_read(uint8_t pad, sysinterval_t timeout) {

  if (pad > 7U) {
    return 0U;
  }
  return lradc_convert(pad, timeout);
}

bool imx23_lradc_touch_detect(void) {

  return ((IMX23_REG32(IMX23_LRADC_STATUS) &
           (IMX23_LRADC_STATUS_TOUCH_PANEL | IMX23_LRADC_STATUS_TOUCH_DETECT_RAW))
          != 0U);
}

int imx23_lradc_touch_read(const imx23_lradc_ts_t *ts, uint16_t *x_raw,
                           uint16_t *y_raw) {
  uint32_t ctrl0_base, x, y;

  if (!imx23_lradc_touch_detect()) {
    return -1;
  }
  ctrl0_base = IMX23_REG32(IMX23_LRADC_CTRL0) &
                ~(IMX23_LRADC_CTRL0_XPULSW | IMX23_LRADC_CTRL0_XNURSW_MASK |
                  IMX23_LRADC_CTRL0_YPLLSW_MASK | IMX23_LRADC_CTRL0_YNLRSW);

  /* X coordinate: drive X+ high, X- low, sense on the Y plate pad. */
  IMX23_REG32(IMX23_LRADC_CTRL0) = ctrl0_base | IMX23_LRADC_CTRL0_XPULSW |
                                   (1U << 17);   /* XNURSW = pull down */
  x = lradc_convert(ts->y_sense, TIME_MS2I(10));

  /* Y coordinate: drive Y+ high, Y- low, sense on the X plate pad. */
  IMX23_REG32(IMX23_LRADC_CTRL0) = ctrl0_base | (1U << 19) |   /* YPLLSW  */
                                   IMX23_LRADC_CTRL0_YNLRSW;
  y = lradc_convert(ts->x_sense, TIME_MS2I(10));

  /* Return the switches to the idle (detect) state. */
  IMX23_REG32(IMX23_LRADC_CTRL0) = ctrl0_base;

  *x_raw = (uint16_t)x;
  *y_raw = (uint16_t)y;
  return 0;
}

/** @} */
