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
 * @file    hal_lld.h
 * @brief   PLATFORM HAL subsystem low level driver header.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef _HAL_LLD_H_
#define _HAL_LLD_H_

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

#include "efr32_registry.h"

/**
 * @name    Platform identification macros
 * @{
 */
#define PLATFORM_NAME           "EFR32FG23"
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*
 * Configuration-related checks.
 */
#if defined(EFR32FG23X) && !defined(EFR32FG23X_MCUCONF)
#error "Using a wrong mcuconf.h file, EFR32FG23X_MCUCONF not defined"
#endif

#if !defined(HFRCODPLL_MAX_FREQ)
#define HFRCODPLL_MAX_FREQ               80000000UL
#endif

#define VSCALE_1V0_MAX_FREQ              40000000UL
#define VSCALE_1V1_MAX_FREQ              HFRCODPLL_MAX_FREQ

#define EFR32_LFXOCLK                    EFR32_LFXO_FREQ
#define EFR32_HFXOCLK                    EFR32_HFXO_FREQ
#define EFR32_CLKIN0CLK                  EFR32_CLKIN0_FREQ
#define EFR32_FSRCOCLK                   20000000UL
#define EFR32_LFRCOCLK                   32768UL
#define EFR32_ULFRCOCLK                  1000UL
#define EFR32_HFRCODPLL_FREQ             19000000UL

#define EFR32_EM01_VSCALESEL_1V0         (1U << 0)
#define EFR32_EM01_VSCALESEL_1V1         (1U << 1)

#define EFR32_EM23_VSCALESEL_0V9         (0U << 0)
#define EFR32_EM23_VSCALESEL_1V0         (1U << 0)
#define EFR32_EM23_VSCALESEL_1V1         (1U << 1)

#define EFR32_EM01_VSCALE_1V0            (1U << 10) /**< EM0/EM1 operation up to 40 MHz */
#define EFR32_EM01_VSCALE_1V1            (1U << 11) /**< EM0/EM1 operation up to 80 MHz */

#define EFR32_EM23_VSCALE_0V9            (0U << 8)  /**< EM2/EM3 operation up to 40 MHz, slow startup */
#define EFR32_EM23_VSCALE_1V0            (1U << 8)  /**< EM2/EM3 operation up to 40 MHz */
#define EFR32_EM23_VSCALE_1V1            (2U << 8)  /**< EM2/EM3 operation up to 80 MHz */

/**
 * @name    CMU_SYSCLKCTRL register bits definitions
 * @{
 */
#define EFR32_SYSCLKSEL_FSRCO            (1U << 0)   /**< SYSCLK source is FSRCO.       */
#define EFR32_SYSCLKSEL_HFRCODPLL        (2U << 0)   /**< SYSCLK source is HFRCODPLL    */
#define EFR32_SYSCLKSEL_HFXO             (3U << 0)   /**< SYSCLK source is HFCO.        */
#define EFR32_SYSCLKSEL_CLKIN0           (4U << 0)   /**< SYSCLK source is CLKIN0.      */

#define EFR32_PPRE_DIV1                  (0U << 10)  /**< PCLK is HCLK divided by 1     */
#define EFR32_PPRE_DIV2                  (1U << 10)  /**< PCLK is HCLK divided by 2     */

#define EFR32_HPRE_DIV1                  (0U  << 12) /**< HCLK is SYSCLK divided by 1  */
#define EFR32_HPRE_DIV2                  (1U  << 12) /**< HCLK is SYSCLK divided by 2  */
#define EFR32_HPRE_DIV4                  (3U  << 12) /**< HCLK is SYSCLK divided by 4  */
#define EFR32_HPRE_DIV8                  (7U  << 12) /**< HCLK is SYSCLK divided by 8  */
#define EFR32_HPRE_DIV16                 (15U << 12) /**< HCLK is SYSCLK divided by 16 */

#define EFR32_RHPRE_DIV1                 (0U << 16)  /**< Radio HCLK is HCLK divided by 1 */
#define EFR32_RHPRE_DIV2                 (1U << 16)  /**< Radio HCLK is HCLK divided by 2 */

#define EFR32_HFRCOPRE_DIV1              (0U << 24)  /**< HFRCO clock is divided by 1  */
#define EFR32_HFRCOPRE_DIV2              (1U << 24)  /**< HFRCO clock is divided by 2  */
#define EFR32_HFRCOPRE_DIV4              (2U << 24)  /**< HFRCO clock is divided by 4  */

/**
 * @brief   System clock source.
 */
#if (EFR32_SYSCLKSEL == EFR32_SYSCLKSEL_FSRCO) || defined(__DOXYGEN__)
#define EFR32_SYSCLK                EFR32_FSRCOCLK
#elif (EFR32_SYSCLKSEL == EFR32_SYSCLKSEL_HFRCODPLL)
#define EFR32_SYSCLK                EFR32_HFRCODPLLCLK
#elif (EFR32_SYSCLKSEL == EFR32_SYSCLKSEL_HFXO)
#define EFR32_SYSCLK                EFR32_HFXOCLK
#elif (EFR32_SYSCLKSEL == EFR32_SYSCLKSEL_CLKIN0)
#define EFR32_SYSCLK                EFR32_CLKIN0CLK
#else
#error "invalid EFR32_SYSCLKSEL value specified"
#endif

/**
 * @brief   HCLK frequency.
 */
#if (EFR32_HPRE == EFR32_HPRE_DIV1) || defined(__DOXYGEN__)
#define EFR32_HCLK                 (EFR32_SYSCLK / 1)
#elif EFR32_HPRE == EFR32_HPRE_DIV2
#define EFR32_HCLK                 (EFR32_SYSCLK / 2)
#elif EFR32_HPRE == EFR32_HPRE_DIV4
#define EFR32_HCLK                 (EFR32_SYSCLK / 4)
#elif EFR32_HPRE == EFR32_HPRE_DIV8
#define EFR32_HCLK                 (EFR32_SYSCLK / 8)
#elif EFR32_HPRE == EFR32_HPRE_DIV16
#define EFR32_HCLK                 (EFR32_SYSCLK / 16)
#else
#error "invalid EFR32_HPRE value specified"
#endif

/**
 * @brief   PCLK frequency.
 */
#if (EFR32_PPRE == EFR32_PPRE_DIV1) || defined(__DOXYGEN__)
#define EFR32_PCLK                 (EFR32_HCLK / 1)
#elif EFR32_PPRE == EFR32_PPRE_DIV2
#define EFR32_PCLK                  (EFR32_HCLK / 2)
#else
#error "invalid EFR32_PPRE value specified"
#endif

/**
 * @brief   RHCLK frequency.
 */
#if (EFR32_RHPRE == EFR32_RHPRE_DIV1) || defined(__DOXYGEN__)
#define EFR32_RHCLK                (EFR32_SYSCLK / 1)
#elif EFR32_RHPRE == EFR32_RHPRE_DIV2
#define EFR32_RHCLK                (EFR32_SYSCLK / 2)
#else
#error "invalid EFR32_RHPRE value specified"
#endif

/**
 * @brief   HFRCODPLL and HFRCOEM23 frequencies.
 */
#if (EFR32_HFRCOPRE == EFR32_HFRCOPRE_DIV1) || defined(__DOXYGEN__)
#define EFR32_HFRCODPLLCLK         (EFR32_HFRCODPLL_FREQ / 1)
#define EFR32_HFRCOEM23CLK         (EFR32_HFRCODPLL_FREQ / 1)
#elif EFR32_HFRCOPRE == EFR32_HFRCOEM23PRE_DIV2
#define EFR32_HFRCODPLLCLK         (EFR32_HFRCODPLL_FREQ / 1)
#define EFR32_HFRCOEM23CLK         (EFR32_HFRCODPLL_FREQ / 2)
#elif EFR32_HFRCOPRE == EFR32_HFRCOEM23PRE_DIV4
#define EFR32_HFRCODPLLCLK         (EFR32_HFRCODPLL_FREQ / 1)
#define EFR32_HFRCOEM23CLK         (EFR32_HFRCODPLLCLK / 4)
#else
#error "invalid EFR32_HFRCOEM23PRE value specified"
#endif

#if EFR32_HFXO_ENABLED
#else
#if EFR32_SYSCLKSEL == EFR32_SYSCLKSEL_HFXO
#error "HFXO is not enabled, required by EFR32_SYSCLKSEL"
#endif
#endif

#if EFR32_FSRCO_ENABLED
#else
#if EFR32_SYSCLKSEL == EFR32_SYSCLKSEL_FSRCO
#error "FSRCO is not enabled, required by EFR32_SYSCLKSEL"
#endif
#endif

#if EFR32_CLKIN0CLK == 0U
#else
#if EFR32_SYSCLKSEL == EFR32_SYSCLKSEL_CLKIN0
#error "CLKIN0 is not enabled, required by EFR32_SYSCLKSEL"
#endif
#endif

#if EFR32_EM01VSCALE_SEL >= EFR32_EM23VSCALE_SEL
#if EFR32_EM01VSCALE_SEL == EFR32_EM01_VSCALESEL_1V0
#define EFR32_EM01_VSCALE                EFR32_EM01_VSCALE_1V0
#elif EFR32_EM01VSCALE_SEL == EFR32_EM01_VSCALESEL_1V1
#define EFR32_EM01_VSCALE                EFR32_EM01_VSCALE_1V1
#else
#error "EFR32_EM01VSCALE_SEL is not set"
#endif
#else
#error "EFR32_EM01VSCALE_SEL must be equal or greater than EFR32_EM23VSCALE_SEL"
#endif

#if EFR32_EM23VSCALE_SEL == EFR32_EM23_VSCALESEL_0V9
#define EFR32_EM23_VSCALE                EFR32_EM23_VSCALE_0V9
#elif EFR32_EM23VSCALE_SEL == EFR32_EM23_VSCALESEL_1V0
#define EFR32_EM23_VSCALE                EFR32_EM23_VSCALE_1V0
#elif EFR32_EM23VSCALE_SEL == EFR32_EM23_VSCALESEL_1V1
#define EFR32_EM23_VSCALE                EFR32_EM23_VSCALE_1V1
#else
#error "EFR32_EM23VSCALE_SEL is not set"
#endif

#if (EFR32_HCLK > VSCALE_1V1_MAX_FREQ)
#error "EFR32_HCLK must be lower than VSCALE_1V1_MAX_FREQ"
#elif (EFR32_HCLK > VSCALE_1V0_MAX_FREQ) && (EFR32_EM01_VSCALE != EFR32_EM23_VSCALESEL_1V1)
#error "EFR32_EM01_VSCALE must be set to EFR32_EM23_VSCALESEL_1V1"
#endif

#define EFR32_EM01GRPACLKSEL_NOCLOCK     (1u << 0)
#define EFR32_EM01GRPACLKSEL_HFRCODPLL   (1u << 1)
#define EFR32_EM01GRPACLKSEL_HFXO        (1u << 2)
#define EFR32_EM01GRPACLKSEL_FSRCO       (1u << 3)
#define EFR32_EM01GRPACLKSEL_HFRCOEM23   (1u << 4)
#define EFR32_EM01GRPACLKSEL_HFRCODPLLRT (1u << 5)
#define EFR32_EM01GRPACLKSEL_HFXORT      (1u << 6)

#define EFR32_EM01GRPCCLKSEL_NOCLOCK     (1u << 0)
#define EFR32_EM01GRPCCLKSEL_HFRCODPLL   (1u << 1)
#define EFR32_EM01GRPCCLKSEL_HFXO        (1u << 2)
#define EFR32_EM01GRPCCLKSEL_FSRCO       (1u << 3)
#define EFR32_EM01GRPCCLKSEL_HFRCOEM23   (1u << 4)
#define EFR32_EM01GRPCCLKSEL_HFRCODPLLRT (1u << 5)
#define EFR32_EM01GRPCCLKSEL_HFXORT      (1u << 6)

#define EFR32_EM23GRPACLKSEL_NOCLOCK     (1u << 0)
#define EFR32_EM23GRPACLKSEL_LFXO        (1u << 1)
#define EFR32_EM23GRPACLKSEL_LFRCO       (1u << 2)
#define EFR32_EM23GRPACLKSEL_ULFRCO      (1u << 3)

#define EFR32_EM4GRPACLKSEL_NOCLOCK      (1u << 0)
#define EFR32_EM4GRPACLKSEL_LFXO         (1u << 1)
#define EFR32_EM4GRPACLKSEL_LFRCO        (1u << 2)
#define EFR32_EM4GRPACLKSEL_ULFRCO       (1u << 3)

#define EFR32_EUSART1SEL_NOCLOCK         (0u << 0)
#define EFR32_EUSART1SEL_EM01GRPCCLK     (1u << 1)
#define EFR32_EUSART1SEL_HFRCOEM23       (1u << 2)
#define EFR32_EUSART1SEL_LFRCO           (1u << 3)
#define EFR32_EUSART1SEL_LFXO            (1u << 4)

#define EFR32_EUSART23SEL_NOCLOCK        (0u << 0)
#define EFR32_EUSART23SEL_HFRCODPLL      (1u << 0)
#define EFR32_EUSART23SEL_HFRCOEM23      (1u << 1)
#define EFR32_EUSART23SEL_HFXO           (1u << 2)
#define EFR32_EUSART23SEL_HFRCODPLLRT    (1u << 3)
#define EFR32_EUSART23SEL_HFXORT         (1u << 4)
#define EFR32_EUSART23SEL_FSRCO          (1u << 5)


#if EFR32_EM01GRPACLKSEL == EFR32_EM01GRPACLKSEL_HFRCODPLL
#define EFR32_EM01GRPACLK               EFR32_HCLK
#elif EFR32_EM01GRPACLKSEL == EFR32_EM01GRPACLKSEL_HFXO
#define EFR32_EM01GRPACLK               EFR32_HFXOCLK
#elif EFR32_EM01GRPACLKSEL == EFR32_EM01GRPACLKSEL_FSRCO
#define EFR32_EM01GRPACLK               EFR32_FSRCOCLK
#elif EFR32_EM01GRPACLKSEL == EFR32_EM01GRPACLKSEL_HFRCOEM23
#error "EFR32_EM01GRPACLKSEL_HFRCOEM23 is not implemented"
#elif EFR32_EM01GRPACLKSEL == EFR32_EM01GRPACLKSEL_HFRCODPLLRT
#error "EFR32_EM01GRPACLKSEL_HFRCODPLLRT is not implemented"
#elif EFR32_EM01GRPACLKSEL == EFR32_EM01GRPACLKSEL_HFXORT
#error "EFR32_EM01GRPACLKSEL_HFXORT is not implemented"
#endif


#if EFR32_EM01GRPCCLKSEL == EFR32_EM01GRPCCLKSEL_HFRCODPLL
#define EFR32_EM01GRPCCLK               EFR32_HCLK
#elif EFR32_EM01GRPCCLKSEL == EFR32_EM01GRPCCLKSEL_HFXO
#define EFR32_EM01GRPCCLK               EFR32_HFXOCLK
#elif EFR32_EM01GRPCCLKSEL == EFR32_EM01GRPCCLKSEL_FSRCO
#define EFR32_EM01GRPCCLK               EFR32_FSRCOCLK
#elif EFR32_EM01GRPCCLKSEL == EFR32_EM01GRPCCLKSEL_HFRCOEM23
#error "EFR32_EM01GRPCCLKSEL_HFRCOEM23 is not implemented"
#elif EFR32_EM01GRPCCLKSEL == EFR32_EM01GRPCCLKSEL_HFRCODPLLRT
#error "EFR32_EM01GRPCCLKSEL_HFRCODPLLRT is not implemented"
#elif EFR32_EM01GRPCCLKSEL == EFR32_EM01GRPCCLKSEL_HFXORT
#error "EFR32_EM01GRPCCLKSEL_HFXORT is not implemented"
#endif


#if EFR32_EM23GRPACLKSEL == EFR32_EM23GRPACLKSEL_LFXO
#define EFR32_EM23GRPACLK               EFR32_LFXOCLK
#elif EFR32_EM23GRPACLKSEL == EFR32_EM23GRPACLKSEL_LFRCO
#define EFR32_EM23GRPACLK               EFR32_LFRCOCLK
#elif EFR32_EM23GRPACLKSEL == EFR32_EM23GRPACLKSEL_ULFRCO
#define EFR32_EM23GRPACLK               EFR32_ULFRCOCLK
#endif


#if EFR32_EM4GRPACLKSEL == EFR32_EM4GRPACLKSEL_LFXO
#define EFR32_EM4GRPACLK                EFR32_LFXOCLK
#elif EFR32_EM4GRPACLKSEL == EFR32_EM4GRPACLKSEL_LFRCO
#define EFR32_EM4GRPACLK                EFR32_LFRCOCLK
#elif EFR32_EM4GRPACLKSEL == EFR32_EM4GRPACLKSEL_ULFRCO
#define EFR32_EM4GRPACLK                EFR32_ULFRCOCLK
#endif

#define EFR32_LETIM1CLK                 EFR32_EM23GRPACLK

#define EFR32_BURTCCLK                  EFR32_EM4GRPACLK

#if EFR32_EUSART1SEL == EFR32_EUSART1SEL_EM01GRPCCLK
#define EFR32_EUSART1CLK                EFR32_EM01GRPCCLK
#elif EFR32_EUSART1SEL == EFR32_EUSART1SEL_HFRCOEM23
#define EFR32_EUSART1CLK                EFR32_HFRCOEM23CLK
#elif EFR32_EUSART1SEL == EFR32_EUSART1SEL_LFRCO
#define EFR32_EUSART1CLK                EFR32_LFRCOCLK
#elif EFR32_EUSART1SEL == EFR32_EUSART1SEL_LFXO
#define EFR32_EUSART1CLK                EFR32_LFXOCLK
#endif

#define EFR32_EUSART23CLK               EFR32_EM01GRPCCLK

#define EFR32_USART1CLK                 EFR32_PCLK


#define EFR32_DAC1SEL_NOCLOCK            (0u << 0)
#define EFR32_DAC1SEL_EM01GRPACLK        (1u << 1)
#define EFR32_DAC1SEL_EM23GRPACLK        (1u << 2)
#define EFR32_DAC1SEL_FSRCO              (1u << 3)
#define EFR32_DAC1SEL_HFRCOEM23          (1u << 4)

#if EFR32_DAC1SEL == EFR32_DAC1SEL_EM01GRPACLK
#define EFR32_DAC1SELCLK                EFR32_EM01GRPACLK
#elif EFR32_DAC1SEL == EFR32_DAC1SEL_EM23GRPACLK
#define EFR32_DAC1SELCLK                EFR32_EM23GRPACLK
#elif EFR32_DAC1SEL == EFR32_DAC1SEL_FSRCO
#define EFR32_DAC1SELCLK                EFR32_FSRCOCLK
#elif EFR32_DAC1SEL == EFR32_DAC1SEL_HFRCOEM23
#define EFR32_DAC1SELCLK                EFR32_HFRCOEM23CLK
#endif


/* Vector8C = Vector(40 + 4*LETIMER0_IRQn) */
#define EFR32_LETIM1_HANDLER            Vector8C
#define EFR32_LETIM1_NUMBER             LETIMER0_IRQn

/* Vector120 = Vector(40 + 4*PCNT0_IRQn) */
#define EFR32_PCNT0_HANDLER			        Vector120
#define EFR32_PCNT0_NUMBER			        PCNT0_IRQn

/* Vector154 = Vector(40 + 4*LESENSE_IRQn) */
#define EFR32_LESENSE_HANDLER 	        Vector154
#define EFR32_LESENSE_NUMBER  		      LESENSE_IRQn

/* VectorA8 = Vector(40 + 4*GPIO_ODD_IRQn) */
#define EFR32_GPIO_ODD_HANDLER 		      VectorA8
#define EFR32_GPIO_ODD_NUMBER			      GPIO_ODD_IRQn

/* VectorAC = Vector(40 + 4*GPIO_EVEN_IRQn) */
#define EFR32_GPIO_EVEN_HANDLER 	    	VectorAC
#define EFR32_GPIO_EVEN_NUMBER		    	GPIO_EVEN_IRQn

/* Vector64 = Vector(40 + 4*USART0_RX_IRQn) */
#define EFR32_USART1_RX_HANDLER 	      Vector64
#define EFR32_USART1_RX_NUMBER		      USART0_RX_IRQn

/* Vector68 = Vector(40 + 4*USART0_TX_IRQn) */
#define EFR32_USART1_TX_HANDLER 	      Vector68
#define EFR32_USART1_TX_NUMBER		      USART0_TX_IRQn

/* Vector6C = Vector(40 + 4*EUSART0_RX_IRQn) */
#define EFR32_EUSART1_RX_HANDLER 		    Vector6C
#define EFR32_EUSART1_RX_NUMBER			    EUSART0_RX_IRQn

/* Vector70 = Vector(40 + 4*EUSART0_TX_IRQn) */
#define EFR32_EUSART1_TX_HANDLER 	    	Vector70
#define EFR32_EUSART1_TX_NUMBER		    	EUSART0_TX_IRQn

/* Vector74 = Vector(40 + 4*EUSART1_RX_IRQn) */
#define EFR32_EUSART2_RX_HANDLER 	      Vector74
#define EFR32_EUSART2_RX_NUMBER		      EUSART1_RX_IRQn

/* Vector78 = Vector(40 + 4*EUSART1_TX_IRQn) */
#define EFR32_EUSART2_TX_HANDLER 	      Vector78
#define EFR32_EUSART2_TX_NUMBER		      EUSART1_TX_IRQn

/* Vector7C = Vector(40 + 4*EUSART2_RX_IRQn) */
#define EFR32_EUSART3_RX_HANDLER 	      Vector7C
#define EFR32_EUSART3_RX_NUMBER		      EUSART2_RX_IRQn

/* Vector80 = Vector(40 + 4*EUSART2_TX_IRQn) */
#define EFR32_EUSART3_TX_HANDLER 	    	Vector80
#define EFR32_EUSART3_TX_NUMBER		    	EUSART2_TX_IRQn

/* Vector88 = Vector(40 + 4*BURTC_IRQn) */
#define EFR32_BURTC_HANDLER             Vector88
#define EFR32_BURTC_NUMBER              BURTC_IRQn

/* Vector88 = Vector(40 + 4*LDMA_IRQn) */
#define EFR32_LDMA_HANDLER              Vector98
#define EFR32_LDMA_NUMBER               LDMA_IRQn

/* Vector168 = Vector(40 + 4*RFECA0_IRQn) */
#define EFR32_RFECA0_HANDLER            Vector168
#define EFR32_RFECA0_NUMBER             RFECA0_IRQn

/* Vector16C = Vector(40 + 4*RFECA1_IRQn) */
#define EFR32_RFECA1_HANDLER            Vector16C
#define EFR32_RFECA1_NUMBER             RFECA1_IRQn

/* Vector15C = Vector(40 + 4*SYSRTC_SEQ_IRQn) */
#define EFR32_SYSRTC_SEQ_HANDLER        Vector15C
#define EFR32_SYSRTC_SEQ_NUMBER         SYSRTC_SEQ_IRQn

/* VectorBC = Vector(40 + 4*AGC_IRQn) */
#define EFR32_AGC_HANDLER               VectorBC
#define EFR32_AGC_NUMBER                AGC_IRQn

/* VectorC0 = Vector(40 + 4*BUFC_IRQn) */
#define EFR32_BUFC_HANDLER              VectorC0
#define EFR32_BUFC_NUMBER               BUFC_IRQn

/* VectorB8 = Vector(40 + 4*EMUDG_IRQn) */
#define EFR32_EMUDG_HANDLER             VectorB8
#define EFR32_EMUDG_NUMBER              EMUDG_IRQn

/* VectorC8 = Vector(40 + 4*FRC_IRQn) */
#define EFR32_FRC_HANDLER               VectorC8
#define EFR32_FRC_NUMBER                FRC_IRQn

/* VectorC4 = Vector(40 + 4*FRC_PRI_IRQn) */
#define EFR32_FRC_PRI_HANDLER           VectorC4
#define EFR32_FRC_PRI_NUMBER            FRC_PRI_IRQn

/* VectorDC = Vector(40 + 4*HOSTMAILBOX_IRQn) */
#define EFR32_HOSTMAILBOX_HANDLER       VectorDC
#define EFR32_HOSTMAILBOX_NUMBER        HOSTMAILBOX_IRQn

/* VectorCC = Vector(40 + 4*MODEM_IRQn) */
#define EFR32_MODEM_HANDLER             VectorCC
#define EFR32_MODEM_NUMBER              MODEM_IRQn

/* VectorD0 = Vector(40 + 4*PROTIMER_IRQn) */
#define EFR32_PROTIMER_HANDLER          VectorD0
#define EFR32_PROTIMER_NUMBER           PROTIMER_IRQn

/* VectorD4 = Vector(40 + 4*RAC_RSM_IRQn) */
#define EFR32_RAC_RSM_HANDLER           VectorD4
#define EFR32_RAC_RSM_NUMBER            RAC_RSM_IRQn

/* VectorD8 = Vector(40 + 4*RAC_SEQ_IRQn) */
#define EFR32_RAC_SEQ_HANDLER           VectorD8
#define EFR32_RAC_SEQ_NUMBER            RAC_SEQ_IRQn

/* VectorE0 = Vector(40 + 4*SYNTH_IRQn) */
#define EFR32_SYNTH_HANDLER             VectorE0
#define EFR32_SYNTH_NUMBER              SYNTH_IRQn

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

/* Various helpers.*/
#include "nvic.h"
#include "efr32_dma.h"

#ifdef __cplusplus
extern "C" {
#endif
void efr32_chip_init(void);

void efr32_clock_init(void);

/**
 * @brief The user can overwrite the weak function in order to use his own values.
 *
 * @param gain    LFXO gain value
 * @param captune LFXO cap tuning value
 */
void efr32_get_lfxo_calibration_values(uint32_t *gain, uint32_t *captune);

void efr32_escape_hatch(void);

void hal_lld_init(void);
#ifdef __cplusplus
}
#endif

#endif /* _HAL_LLD_H_ */

/** @} */
