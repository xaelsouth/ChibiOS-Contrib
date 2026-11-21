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
 * @file    IMX23/IMX23xx/imx23_lradc.h
 * @brief   i.MX23 LRADC (low-resolution ADC, 8 channels) + 4-wire touch.
 *
 * @details Register map per the i.MX23 reference manual (matches the
 *          bootlets regs-lradc.h): CTRL0 0x00, CTRL1 0x10, CTRL2 0x20,
 *          CTRL3 0x30, STATUS 0x40, CHn 0x50+0x10n, DELAYn 0xD0+0x10n,
 *          CTRL4 0x140. Conversions are scheduled through DELAY0 and
 *          channel 0; results are read polled from CTRL1/CH0.
 *
 *          The EVK's 4.3" LCD has a 4-wire resistive touch panel on the
 *          LRADC (see imx23-evk.dts: fsl,lradc-touchscreen-wires = <4>);
 *          imx23_lradc_touch_read() drives the plates via the CTRL0
 *          switches and senses through the channels selected in CTRL4.
 *
 * @addtogroup IMX23
 * @{
 */

#ifndef IMX23_LRADC_H
#define IMX23_LRADC_H

#include "imx23.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/* Register offsets. */
#define IMX23_LRADC_CTRL0           (IMX23_LRADC_BASE + 0x00U)
#define IMX23_LRADC_CTRL1           (IMX23_LRADC_BASE + 0x10U)
#define IMX23_LRADC_CTRL2           (IMX23_LRADC_BASE + 0x20U)
#define IMX23_LRADC_CTRL3           (IMX23_LRADC_BASE + 0x30U)
#define IMX23_LRADC_STATUS          (IMX23_LRADC_BASE + 0x40U)
#define IMX23_LRADC_CH(n)           (IMX23_LRADC_BASE + 0x50U + (uint32_t)(n) * 0x10U)
#define IMX23_LRADC_DELAY(n)        (IMX23_LRADC_BASE + 0xD0U + (uint32_t)(n) * 0x10U)
#define IMX23_LRADC_CTRL4           (IMX23_LRADC_BASE + 0x140U)

/* CTRL0 bits (i.MX23 RM / U-Boot regs-lradc.h). */
#define IMX23_LRADC_CTRL0_SFTRST            (1U << 31)
#define IMX23_LRADC_CTRL0_CLKGATE           (1U << 30)
#define IMX23_LRADC_CTRL0_ONCHIP_GROUNDREF  (1U << 26)
#define IMX23_LRADC_CTRL0_TOUCH_DETECT_EN   (1U << 23)
#define IMX23_LRADC_CTRL0_TOUCH_SCREEN_TYPE (1U << 22)  /* 1 = 4-wire */
#define IMX23_LRADC_CTRL0_YNLRSW            (1U << 21)
#define IMX23_LRADC_CTRL0_YPLLSW_MASK       (0x3U << 19)
#define IMX23_LRADC_CTRL0_XNURSW_MASK       (0x3U << 17)
#define IMX23_LRADC_CTRL0_XPULSW            (1U << 16)
#define IMX23_LRADC_CTRL0_SCHEDULE_MASK     0xFFU

/* CTRL1 bits: LRADCx_IRQ status/enable at bit 16+x, TOUCH_DETECT_IRQ bit 8. */
#define IMX23_LRADC_CTRL1_LRADC_IRQ(ch)     (1U << (16U + (ch)))
#define IMX23_LRADC_CTRL1_TOUCH_IRQ         (1U << 8)

/* STATUS bits. */
#define IMX23_LRADC_STATUS_TOUCH_PANEL      (1U << 24)
#define IMX23_LRADC_STATUS_TOUCH_DETECT_RAW (1U << 0)

/* CHn fields. */
#define IMX23_LRADC_CH_VALUE_MASK           0x3FFFFU    /* 18-bit result       */

/* DELAYn fields. */
#define IMX23_LRADC_DELAY_KICK              (1U << 20)
#define IMX23_LRADC_DELAY_TRIGGER(ch)       (1U << (24U + (ch)))

/* CTRL4: LRADCnSELECT at bits 4*n (source pad 0..15 for logical channel n). */
#define IMX23_LRADC_CTRL4_SELECT(n, pad)    ((uint32_t)(pad) << (4U * (n)))

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   4-wire touch panel wiring (which LRADC pads sense the plates).
 * @note    Defaults target the i.MX23 EVK 4.3" LCD; adjust per board.
 */
typedef struct {
  uint8_t x_sense;      /**< LRADC pad sensing the X plate (Y read)        */
  uint8_t y_sense;      /**< LRADC pad sensing the Y plate (X read)        */
  uint16_t raw_min;     /**< raw value at screen origin (calibration)      */
  uint16_t raw_max;     /**< raw value at far edge (calibration)           */
} imx23_lradc_ts_t;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void     imx23_lradc_init(void);
  uint32_t imx23_lradc_read(uint8_t pad, sysinterval_t timeout);
  bool     imx23_lradc_touch_detect(void);
  int      imx23_lradc_touch_read(const imx23_lradc_ts_t *ts,
                                  uint16_t *x_raw, uint16_t *y_raw);
#ifdef __cplusplus
}
#endif

#endif /* IMX23_LRADC_H */

/** @} */
