/*
    ChibiOS - Copyright (C) 2026 Xael South

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
 * @file    ADCv1/hal_adc_lld.h
 * @brief   EFR32FG23 IADC subsystem low level driver header.
 *
 * @addtogroup ADC
 * @{
 */

#ifndef HAL_ADC_LLD_H
#define HAL_ADC_LLD_H

#if (HAL_USE_ADC == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

#define ADC_ERR_DMAFAILURE      1U
#define ADC_ERR_OVERFLOW        2U
#define ADC_ERR_AWD             4U

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(EFR32_ADC_USE_IADC0) || defined(__DOXYGEN__)
#define EFR32_ADC_USE_IADC0                 TRUE
#endif

#if !defined(EFR32_ADC_IADC0_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_ADC_IADC0_IRQ_PRIORITY        4
#endif

#if !defined(EFR32_ADC_IADC0_DMA_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_ADC_IADC0_DMA_PRIORITY        4
#endif

#define EFR32_ADC_IADC0_DMA_STREAM          EFR32_DMA_STREAM_ID_ANY

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

typedef uint16_t adcsample_t;
typedef uint16_t adc_channels_num_t;
typedef uint32_t adcerror_t;

/**
 * @brief   IADC port selection for a channel.
 */
typedef enum {
  EFR32_ADC_PORT_GND   = 0U,
  EFR32_ADC_PORT_DAC1  = 2U,
  EFR32_ADC_PORT_PORTA = 8U,
  EFR32_ADC_PORT_PORTB = 9U,
  EFR32_ADC_PORT_PORTC = 10U,
  EFR32_ADC_PORT_PORTD = 11U,
} efr32_adc_port_t;

/**
 * @brief   ADC driver configuration (per-group).
 */
#define adc_lld_configuration_group_fields                                  \
  /* Positive input port for single conversion. */                         \
  efr32_adc_port_t          portpos;                                        \
  /* Negative input port (usually GND). */                                 \
  efr32_adc_port_t          portneg;                                        \
  /* Pin number within the port (0-15). */                                  \
  uint8_t                   pin;                                          \
  /* Reference selection (0=VBGR, 1=VREF, 3=VDDX). */                      \
  uint8_t                   refsel;                                       \
  /* Oversampling (0=HISPD2 ... 6=HISPD128). */                             \
  uint8_t                   osrhs

/**
 * @brief   ADC driver-level config fields.
 */
#define adc_lld_config_fields                                               \
  uint32_t                  dummy

/**
 * @brief   ADC driver structure fields.
 */
#define adc_lld_driver_fields                                               \
  /* Pointer to IADC registers. */                                          \
  IADC_TypeDef              *iadc;                                          \
  /* Associated DMA stream. */                                              \
  const efr32_dma_stream_t  *dma;                                           \
  /* Error flags. */                                                        \
  adcerror_t                errors

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if EFR32_ADC_USE_IADC0 && !defined(__DOXYGEN__)
extern ADCDriver ADCD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void adc_lld_init(void);
  msg_t adc_lld_start(ADCDriver *adcp);
  void adc_lld_stop(ADCDriver *adcp);
  void adc_lld_start_conversion(ADCDriver *adcp);
  void adc_lld_stop_conversion(ADCDriver *adcp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_ADC == TRUE */

#endif /* HAL_ADC_LLD_H */

/** @} */
