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

/*
 * EFR32FG23 drivers configuration.
 * The following settings override the default settings present in
 * the various device driver implementation headers.
 * Note that the settings for each driver only have effect if the whole
 * driver is enabled in halconf.h.
 */

#ifndef MCUCONF_H
#define MCUCONF_H

#define EFR32FG23X_MCUCONF

/* Enable the shared DMA helper driver (required by SPI, I2C, ADC, DAC). */
#define EFR32_DMA_REQUIRED

/*
 * HAL driver system settings.
 */

/*
 * Power settings.
 */
#define EFR32_EM01VSCALE_SEL                EFR32_EM01_VSCALESEL_1V1
#define EFR32_EM23VSCALE_SEL                EFR32_EM23_VSCALESEL_1V1

/*
 * Clock settings.
 */
#define EFR32_CMU_SYSCLKCTRL                (EFR32_SYSCLKSEL | EFR32_PPRE | EFR32_HPRE | EFR32_RHPRE)
#define EFR32_LFXO_ENABLED                  TRUE
#define EFR32_LFRCO_ENABLED                 FALSE
#define EFR32_ULFRCO_ENABLED                FALSE
#define EFR32_FSRCO_ENABLED                 FALSE
#define EFR32_HFRCODPLL_ENABLED             TRUE
#define EFR32_HFRCOEM23_ENABLED             TRUE
#define EFR32_HFXO_ENABLED                  FALSE
#define EFR32_SYSCLKSEL                     EFR32_SYSCLKSEL_HFRCODPLL
#define EFR32_PPRE                          EFR32_PPRE_DIV1
#define EFR32_HPRE                          EFR32_HPRE_DIV1
#define EFR32_RHPRE                         EFR32_RHPRE_DIV1
#define EFR32_HFRCOPRE                      EFR32_HFRCOPRE_DIV1

/*
 * Peripherals clock sources.
 */
#define EFR32_EM01GRPACLKSEL                EFR32_EM01GRPACLKSEL_HFRCODPLL
#define EFR32_EM01GRPACLK_ENABLED           (EFR32_EM01GRPACLKSEL != EFR32_EM01GRPACLKSEL_NOCLOCK)

#define EFR32_EM01GRPCCLKSEL                EFR32_EM01GRPCCLKSEL_HFRCODPLL
#define EFR32_EM01GRPCCLK_ENABLED           (EFR32_EM01GRPCCLKSEL != EFR32_EM01GRPCCLKSEL_NOCLOCK)

#define EFR32_EM23GRPACLKSEL                EFR32_EM23GRPACLKSEL_LFXO
#define EFR32_EM23GRPACLK_ENABLED           (EFR32_EM23GRPACLKSEL != EFR32_EM23GRPACLKSEL_NOCLOCK)

#define EFR32_EM4GRPACLKSEL                 EFR32_EM4GRPACLKSEL_LFXO
#define EFR32_EM4GRPACLK_ENABLED            (EFR32_EM4GRPACLKSEL != EFR32_EM4GRPACLKSEL_NOCLOCK)

#define EFR32_EUSART1SEL                    EFR32_EUSART1SEL_LFXO

#define EFR32_EUSART23SEL                   EFR32_EUSART23SEL_HFRCODPLL

#define EFR32_DAC1SEL                       EFR32_DAC1SEL_HFRCOEM23

/*
 * ST driver system settings.
 */
#define EFR32_ST_IRQ_PRIORITY               4
#define EFR32_ST_USE_TIMER                  1

/*
 * GPIO driver system settings.
 */
#define EFR32_GPIO_ODD_IRQ_PRIORITY         4
#define EFR32_GPIO_EVEN_IRQ_PRIORITY        4

/*
 * SIO driver system settings.
 */
#define EFR32_SIO_USE_EUSART1               TRUE
#define EFR32_SIO_USE_EUSART2               TRUE
#define EFR32_SIO_USE_EUSART3               FALSE
#define EFR32_SIO_USE_USART1                TRUE
#define EFR32_EUSART1_RX_IRQ_PRIORITY       4
#define EFR32_EUSART1_TX_IRQ_PRIORITY       4
#define EFR32_EUSART2_RX_IRQ_PRIORITY       4
#define EFR32_EUSART2_TX_IRQ_PRIORITY       4
#define EFR32_EUSART3_RX_IRQ_PRIORITY       4
#define EFR32_EUSART3_TX_IRQ_PRIORITY       4
#define EFR32_USART1_RX_IRQ_PRIORITY        4
#define EFR32_USART1_TX_IRQ_PRIORITY        4

/*
 * RTC driver system settings.
 */
#define EFR32_RTC_USE_RTC1                  TRUE
#define EFR32_BURTC_PRIORITY                4

/*
 * BURAM driver system settings.
 */
#define BURAM_USE_BURAM1                    TRUE
#define BURAM_USE_BURAM2                    FALSE
#define BURAM_USE_BURAM3                    FALSE
#define BURAM_USE_BURAM4                    FALSE

/*
 * DAC driver system settings.
 */
#define EFR32_DAC_DUAL_MODE                 FALSE
#define EFR32_DAC_USE_DAC1_CH1              TRUE
#define EFR32_DAC_USE_DAC1_CH2              TRUE
#define EFR32_DAC_DAC1_IRQ_PRIORITY         4
#define EFR32_DAC_DAC1_CH1_DMA_STREAM       EFR32_DMA_STREAM_ID_ANY
#define EFR32_DAC_DAC1_CH2_DMA_STREAM       EFR32_DMA_STREAM_ID_ANY

/*
 * SPI driver system settings.
 */
#define EFR32_SPI_USE_EUSART1               TRUE
#define EFR32_SPI_USE_EUSART2               TRUE
#define EFR32_SPI_USE_EUSART3               FALSE
#define EFR32_SPI_EUSART1_IRQ_PRIORITY      4
#define EFR32_SPI_EUSART2_IRQ_PRIORITY      4
#define EFR32_SPI_EUSART3_IRQ_PRIORITY      4
#define EFR32_SPI_EUSART1_TX_DMA_STREAM     EFR32_DMA_STREAM_ID_ANY
#define EFR32_SPI_EUSART1_RX_DMA_STREAM     EFR32_DMA_STREAM_ID_ANY
#define EFR32_SPI_EUSART2_TX_DMA_STREAM     EFR32_DMA_STREAM_ID_ANY
#define EFR32_SPI_EUSART2_RX_DMA_STREAM     EFR32_DMA_STREAM_ID_ANY
#define EFR32_SPI_EUSART3_TX_DMA_STREAM     EFR32_DMA_STREAM_ID_ANY
#define EFR32_SPI_EUSART3_RX_DMA_STREAM     EFR32_DMA_STREAM_ID_ANY
#define EFR32_SPI_EUSART1_DMA_IRQ_PRIORITY  5
#define EFR32_SPI_EUSART2_DMA_IRQ_PRIORITY  5
#define EFR32_SPI_EUSART3_DMA_IRQ_PRIORITY  5

/*
 * I2C driver system settings.
 */
#define EFR32_I2C_USE_I2C0                  TRUE
#define EFR32_I2C_USE_I2C1                  TRUE
#define EFR32_I2C0_IRQ_PRIORITY             4
#define EFR32_I2C1_IRQ_PRIORITY             4
#define EFR32_I2C0_TX_DMA_STREAM            EFR32_DMA_STREAM_ID_ANY
#define EFR32_I2C0_RX_DMA_STREAM            EFR32_DMA_STREAM_ID_ANY
#define EFR32_I2C1_TX_DMA_STREAM            EFR32_DMA_STREAM_ID_ANY
#define EFR32_I2C1_RX_DMA_STREAM            EFR32_DMA_STREAM_ID_ANY
#define EFR32_I2C0_DMA_IRQ_PRIORITY         5
#define EFR32_I2C1_DMA_IRQ_PRIORITY         5

/*
 * LCD segment controller driver settings.
 */
#define EFR32_LCD_USE_LCD                   FALSE
#define EFR32_LCD_IRQ_PRIORITY              3
#define EFR32_LCD_DMA_STREAM                EFR32_DMA_STREAM_ID_ANY
#define EFR32_LCD_DMA_IRQ_PRIORITY          5

/*
 * RAIL (radio) glue driver settings.
 */
#define EFR32_RAIL_USE_RAIL                 FALSE

/*
 * CRYPTO (RADIOAES AES engine) driver settings.
 */
#define EFR32_CRYPTO_USE_AES1               FALSE
#define EFR32_CRYPTO_AES_IRQ_MODE           FALSE
#define EFR32_AES_IRQ_PRIORITY              3

/*
 * WDG driver system settings.
 */
#define EFR32_WDG_USE_WDOG0                 TRUE
#define EFR32_WDG_USE_WDOG1                 FALSE
#define EFR32_WDG_WDOG0_IRQ_PRIORITY        2
#define EFR32_WDG_WDOG1_IRQ_PRIORITY        2

#define EFR32_PWM_USE_TIMER0    TRUE

#endif /* MCUCONF_H */
