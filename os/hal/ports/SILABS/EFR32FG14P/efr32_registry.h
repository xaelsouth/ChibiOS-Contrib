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
 * @file    EFR32FG14P/efr32_registry.h
 * @brief   EFR32FG14P capabilities registry.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef EFR32_REGISTRY_H
#define EFR32_REGISTRY_H

/*===========================================================================*/
/* Defined device check.                                                     */
/*===========================================================================*/

#if !defined(EFR32FG14P231F128GM48) && !defined(EFR32FG14P231F256GM32) && \
    !defined(EFR32FG14P231F256GM48) && !defined(EFR32FG14P231F256IM32) && \
    !defined(EFR32FG14P231F256IM48) && !defined(EFR32FG14P232F128GM32) && \
    !defined(EFR32FG14P232F128GM48) && !defined(EFR32FG14P232F256GM32) && \
    !defined(EFR32FG14P232F256GM48) && !defined(EFR32FG14P233F128GM48) && \
    !defined(EFR32FG14P233F256GM48)
#error "No valid device defined."
#endif

/**
 * @brief   Sub-family identifier.
 */
#if !defined(EFR32FG14P23X) || defined(__DOXYGEN__)
#define EFR32FG14P23X
#endif


/*
 * Platform capabilities (from the device header of EFR32FG14P231F256GM48;
 * all FG14P variants share these blocks).
 *
 * HAL numbering is 1-based throughout: EFR32_HAS_USART1 refers to
 * hardware USART0, EFR32_HAS_TIMER1 to TIMER0, and so on.
 */

/*===========================================================================*/
/* Platform capabilities.                                                    */
/*===========================================================================*/

#define EFR32_HAS_GPIOA                     TRUE
#define EFR32_HAS_GPIOB                     TRUE
#define EFR32_HAS_GPIOC                     TRUE
#define EFR32_HAS_GPIOD                     TRUE

/* USART_COUNT = 2 (USART0, USART1), LEUART_COUNT = 1 (LEUART0). */
#define EFR32_HAS_USART1                    TRUE
#define EFR32_HAS_USART2                    TRUE
#define EFR32_HAS_LEUART1                   TRUE

/*
 * TIMER driver system settings: TIMER_COUNT = 2 (16-bit TIMER0/TIMER1),
 * WTIMER_COUNT = 1 (32-bit WTIMER0).
 */
#define EFR32_HAS_TIMER1                    TRUE
#define EFR32_HAS_TIMER2                    TRUE
#define EFR32_TIMER_IS_16BITS               TRUE
#define EFR32_HAS_WTIMER1                   TRUE
#define EFR32_WTIMER1_IS_32BITS             TRUE

/* I2C driver system settings (I2C_COUNT = 1: I2C0). */
#define EFR32_HAS_I2C1                      TRUE

/*
 * ADC driver system settings: series-1 ADC (ADC_COUNT = 1: ADC0).
 * There is no IADC on FG14P.
 */
#define EFR32_HAS_ADC0                      TRUE
#define EFR32_HAS_IADC                      FALSE

/* COMP driver system settings (ACMP_COUNT = 2: ACMP0, ACMP1). */
#define EFR32_HAS_ACMP1                     TRUE
#define EFR32_HAS_ACMP2                     TRUE

/* LESENSE driver system settings (LESENSE_COUNT = 1, 16 channels). */
#define EFR32_HAS_LESENSE                   TRUE
#define EFR32_LESENSE_NUM_CHANNELS          16

/* QEI driver system settings (PCNT_COUNT = 1: PCNT0). */
#define EFR32_HAS_PCNT0                     TRUE

/*
 * SIGROUTE driver system settings (PRS_COUNT = 1, CH[12]).
 * The upper layer is called SIGROUTE because "PRS" is a Silicon Labs
 * trademark; the hardware peripheral is PRS.
 */
#define EFR32_HAS_PRS                       TRUE
#define EFR32_PRS_NUM_CHANNELS              12

/*
 * RTC driver system settings: RTCC (RTCC_COUNT = 1, 32-bit CNT,
 * subseconds via prescaler, alarms on CC0/CC1 of CC[3]) and the
 * PRORTC (PRORTC_COUNT = 1).
 */
#define EFR32_HAS_RTCC                      TRUE
#define EFR32_HAS_PRORTC                    TRUE
#define EFR32_HAS_RTC                       TRUE
#define EFR32_RTC_HAS_SUBSECONDS            TRUE
#define EFR32_RTC_NUM_ALARMS                2

/*
 * CRYPTO driver system settings (CRYPTO_COUNT = 1): hardware AES/SHA
 * engine. TRNG (TRNG_COUNT = 1) and CRYOTIMER (CRYOTIMER_COUNT = 1).
 */
#define EFR32_HAS_CRYPTO                    TRUE
#define EFR32_HAS_AES                       TRUE
#define EFR32_HAS_TRNG                      TRUE
#define EFR32_HAS_CRYOTIMER                 TRUE

/* IDAC (IDAC_COUNT = 1) — current DAC. */
#define EFR32_HAS_IDAC                      TRUE

/*
 * DAC driver system settings: VDAC (VDAC_COUNT = 1) with a single output
 * channel (CH0).
 */
#define EFR32_HAS_VDAC                      TRUE
#define EFR32_VDAC_NUM_CHANNELS             1
#define EFR32_HAS_DAC1_CH1                  TRUE
#define EFR32_HAS_DAC1_CH2                  FALSE

/* CRC driver system settings (GPCRC_COUNT = 1). */
#define EFR32_HAS_GPCRC                     TRUE

/*
 * WDG driver system settings (WDOG_COUNT = 2: WDOG0, WDOG1).
 */
#define EFR32_HAS_WDOG0                     TRUE
#define EFR32_HAS_WDOG1                     TRUE

/*
 * DMA driver system settings: FG14P has both the classic DMA
 * (DMA_CHAN_COUNT = 8, fixed peripheral request mapping) and an LDMA
 * (LDMA_PRESENT, CH[8]).
 */
#define EFR32_HAS_DMA                       TRUE
#define EFR32_DMA_NUM_CHANNELS              8
#define EFR32_HAS_LDMA                      TRUE
#define EFR32_LDMA_NUM_CHANNELS             8

/* BOOTLOADER (BOOTLOADER_COUNT = 1) — factory bootloader interface. */
#define EFR32_HAS_BOOTLOADER                TRUE

/* External interrupt lines (EXT_IRQ_COUNT = 42). */
#define EFR32_EXT_IRQ_COUNT                 42

/* LETIM attributes: LETIMER_COUNT = 1, 16-bit counter
 * (_LETIMER_CNT_MASK = 0x0000FFFF).*/
#define EFR32_LETIM_MAX_CHANNELS            2

#define EFR32_HAS_LETIM1                    TRUE
#define EFR32_LETIM1_IS_32BITS              FALSE
#define EFR32_LETIM1_IS_16BITS              TRUE
#define EFR32_LETIM1_CHANNELS               2

#define EFR32_HAS_LETIM2                    FALSE
#define EFR32_LETIM2_IS_32BITS              FALSE
#define EFR32_LETIM2_CHANNELS               2


/*===========================================================================*/
/* Common.                                                                   */
/*===========================================================================*/



/** @} */

#endif /* EFR32_REGISTRY_H */

/** @} */
