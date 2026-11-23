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

/**
 * @file    EFR32FG23/efr32_registry.h
 * @brief   EFR32FG23 capabilities registry.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef EFR32_REGISTRY_H
#define EFR32_REGISTRY_H

/*===========================================================================*/
/* Defined device check.                                                     */
/*===========================================================================*/

#if !defined(EFR32FG23A010F128GM40) && !defined(EFR32FG23A010F256GM40) && \
    !defined(EFR32FG23A010F256GM48) && !defined(EFR32FG23A010F512GM40) && \
    !defined(EFR32FG23A010F512GM48) && !defined(EFR32FG23A011F512GM40) && \
    !defined(EFR32FG23A020F128GM40) && !defined(EFR32FG23A020F256GM40) && \
    !defined(EFR32FG23A020F256GM48) && !defined(EFR32FG23A020F512GM40) && \
    !defined(EFR32FG23A020F512GM48) && !defined(EFR32FG23A021F512GM40) && \
    !defined(EFR32FG23B010F128GM40) && !defined(EFR32FG23B010F512GM48) && \
    !defined(EFR32FG23B010F512IM40) && !defined(EFR32FG23B010F512IM48) && \
    !defined(EFR32FG23B020F128GM40) && !defined(EFR32FG23B020F512IM40) && \
    !defined(EFR32FG23B020F512IM48) && !defined(EFR32FG23B021F512IM40) && \
    !defined(EFR32FG23B021F512IM48)
#error "No valid device defined."
#endif

/**
 * @brief   Sub-family identifier.
 */
#if !defined(EFR32FG23X) || defined(__DOXYGEN__)
#define EFR32FG23X
#endif

/*===========================================================================*/
/* Platform capabilities.                                                    */
/*===========================================================================*/

#define EFR32_HAS_LETIM1                    TRUE

#define EFR32_HAS_GPIOA					            TRUE
#define EFR32_HAS_GPIOB					            TRUE
#define EFR32_HAS_GPIOC					            TRUE
#define EFR32_HAS_GPIOD					            TRUE

#define EFR32_HAS_EUSART1                   TRUE
#define EFR32_HAS_EUSART2                   TRUE
#define EFR32_HAS_EUSART3                   TRUE

#define EFR32_HAS_USART1                    TRUE

/*
 * RTC driver system settings.
 */
#define EFR32_HAS_RTC                       TRUE
#define EFR32_RTC_HAS_SUBSECONDS            TRUE
#define EFR32_RTC_NUM_ALARMS                1

/*
 * LETIM driver system settings.
 * NOTE: LETIMER0 on FG23 is a 24-bit counter (_LETIMER_CNT_MASK = 0x00FFFFFF),
 * not 32-bit — SYSTICKv3 models it as 24-bit (ST_INIT_VALUE 0x00FFFFFF).
 */
#define EFR32_LETIM1_IS_32BITS              FALSE
#define EFR32_LETIM1_IS_24BITS              TRUE
#define EFR32_LETIM_MAX_CHANNELS            2
#define EFR32_LETIM1_CHANNELS               2

/*
 * EUSART driver system settings.
 */
#define EFR32_EUSART1_LOW_POWER_ENABLED     TRUE
#define EFR32_EUSART2_LOW_POWER_ENABLED     FALSE
#define EFR32_EUSART3_LOW_POWER_ENABLED     FALSE

/*
 * DAC driver system settings.
 */
#define EFR32_HAS_DAC1_CH1                  TRUE
#define EFR32_HAS_DAC1_CH2                  TRUE

/*
 * DMA driver system settings.
 */
#define EFR32_HAS_DMA                       TRUE
#define EFR32_ADVANCED_DMA                  TRUE
#define EFR32_DMA_SUPPORTS_DMAMUX           TRUE

/*
 * WDG driver system settings.
 */
#define EFR32_HAS_WDOG0                     TRUE
#define EFR32_HAS_WDOG1                     TRUE

/*
 * LESENSE driver system settings.
 */
#define EFR32_HAS_LESENSE                   TRUE
#define EFR32_LESENSE_NUM_CHANNELS          16

/*
 * CRC driver system settings.
 */
#define EFR32_HAS_GPCRC                     TRUE

/*
 * TIMER driver system settings (TIMER_COUNT = 5: TIMER0..TIMER4,
 * exposed as EFR32_TIMER1..5).
 */
#define EFR32_HAS_TIMER1                    TRUE
#define EFR32_HAS_TIMER2                    TRUE
#define EFR32_HAS_TIMER3                    TRUE
#define EFR32_HAS_TIMER4                    TRUE
#define EFR32_HAS_TIMER5                    TRUE

/*
 * I2C driver system settings (I2C_COUNT = 2: I2C0, I2C1).
 */
#define EFR32_HAS_I2C1                      TRUE
#define EFR32_HAS_I2C2                      TRUE

/*
 * ADC driver system settings (IADC_COUNT = 1).
 */
#define EFR32_HAS_IADC                      TRUE

/*
 * COMP driver system settings (ACMP_COUNT = 2: ACMP0, ACMP1).
 */
#define EFR32_HAS_ACMP1                     TRUE
#define EFR32_HAS_ACMP2                     TRUE

/*
 * LCD driver system settings (LCD_COUNT = 1).
 */
#define EFR32_HAS_LCD                       TRUE

/*
 * KEYSCAN driver system settings (KEYSCAN_COUNT = 1).
 */
#define EFR32_HAS_KEYSCAN                   TRUE

/*
 * QEI driver system settings (PCNT_COUNT = 1: PCNT0).
 */
#define EFR32_HAS_PCNT0                     TRUE

/*
 * SIGROUTE driver system settings (PRS_COUNT = 1).
 * The upper layer is called SIGROUTE because "PRS" is a Silicon Labs
 * trademark; the hardware peripheral is PRS.
 */
#define EFR32_HAS_PRS                       TRUE

/*
 * SYSRTC driver system settings (SYSRTC_COUNT = 1).
 */
#define EFR32_HAS_SYSRTC                    TRUE

/*
 * AES driver system settings (AES_COUNT = 1).
 */
#define EFR32_HAS_AES                       TRUE

/*
 * BURAM driver system settings (backup RAM, keeps content in EM4).
 */
#define EFR32_HAS_BURAM                     TRUE

/*===========================================================================*/
/* Common.                                                                   */
/*===========================================================================*/

/** @} */

#endif /* EFR32_REGISTRY_H */

/** @} */
