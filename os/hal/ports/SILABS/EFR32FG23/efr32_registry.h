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
/* One alarm. Hardware-wise it lives on SYSRTC CMP1 (BURTC's single COMP
   is owned by the calendar tick) — see BURTCv1. Requires
   EFR32_SYSRTC_USE_SYSRTC=TRUE. */
#define EFR32_RTC_NUM_ALARMS                1

/*
 * LETIM driver system settings.
 * NOTE: LETIMER0 on FG23 is a 24-bit counter (_LETIMER_CNT_MASK = 0x00FFFFFF),
 * not 32-bit.
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

/*
 * BURTC driver system settings (BURTC_COUNT = 1: 32-bit backup RTC,
 * used as calendar/timekeeper only — the system tick is SYSRTC (SYSTICKv2)).
 */
#define EFR32_HAS_BURTC                     TRUE

/*
 * Additional hardware blocks (from the device header of
 * EFR32FG23B010F512IM48; all FG23 variants share these):
 *
 * HAL numbering is 1-based throughout: EFR32_HAS_EUSART1 refers to
 * hardware EUSART0, EFR32_HAS_TIMER1 to TIMER0, and so on.
 */

/* VDAC (DAC_COUNT = 1, channels CH0/CH1) — the hardware block behind
 * EFR32_HAS_DAC1_CH1/CH2 above. */
#define EFR32_HAS_VDAC                      TRUE
#define EFR32_VDAC_NUM_CHANNELS             2

/* LCDRF (LCDRF_COUNT = 1) — LCD charge-redistribution/waveform companion
 * of the LCD controller. */
#define EFR32_HAS_LCDRF                     TRUE

/* Radio mailboxes (HOSTMAILBOX_COUNT = 1, SEMAILBOX_COUNT = 1) — used by
 * the RAIL/radio glue to exchange commands with the modem. */
#define EFR32_HAS_HOSTMAILBOX               TRUE
#define EFR32_HAS_SEMAILBOX                 TRUE

/* DPLL (DPLL_COUNT = 1) — precision clock source for the radio.
 * ICACHE (ICACHE_COUNT = 1) — flash instruction cache. */
#define EFR32_HAS_DPLL                      TRUE
#define EFR32_HAS_ICACHE                    TRUE

/* LDMA (LDMA_COUNT = 1, CH[8]) — the DMA engine; EFR32_HAS_DMA above refers
 * to it. LDMAXBAR (LDMAXBAR_COUNT = 1) is the DMA request crossbar. */
#define EFR32_HAS_LDMA                      TRUE
#define EFR32_LDMA_NUM_CHANNELS             8
#define EFR32_HAS_LDMAXBAR                  TRUE

/* RADIOAES (RADIOAES_COUNT = 1) — AES engine inside the radio block;
 * EFR32_HAS_AES above refers to it. */
#define EFR32_HAS_RADIOAES                  TRUE

/* PRS has ASYNC_CH[12] + SYNC_CH[4]. */
#define EFR32_PRS_NUM_CHANNELS              12
#define EFR32_PRS_NUM_SYNC_CHANNELS         4

/* External interrupt lines (EXT_IRQ_COUNT = 76). */
#define EFR32_EXT_IRQ_COUNT                 76

/* GPIO pin counts per port (GPIO_Px_COUNT). */
#define EFR32_GPIO_PA_PINS                  11
#define EFR32_GPIO_PB_PINS                  4
#define EFR32_GPIO_PC_PINS                  10
#define EFR32_GPIO_PD_PINS                  6

/* All five TIMERs are 32-bit on FG23 (_TIMER_CNT_MASK = 0xFFFFFFFF). */
#define EFR32_TIMERS_ARE_32BITS             TRUE

/*===========================================================================*/
/* Common.                                                                   */
/*===========================================================================*/

/** @} */

#endif /* EFR32_REGISTRY_H */

/** @} */
