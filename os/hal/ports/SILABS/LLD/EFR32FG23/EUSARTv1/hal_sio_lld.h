/*
    ChibiOS - Copyright (C) 2024 Xael South

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
 * @file    hal_sio_lld.h
 * @brief   PLATFORM SIO subsystem low level driver header.
 *
 * @addtogroup SIO
 * @{
 */

#ifndef HAL_SIO_LLD_H
#define HAL_SIO_LLD_H

#if (HAL_USE_SIO == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

#define EFR32_SIO_LLD_USART_DATABITS_FOUR      USART_FRAME_DATABITS_FOUR
#define EFR32_SIO_LLD_USART_DATABITS_FIVE      USART_FRAME_DATABITS_FIVE
#define EFR32_SIO_LLD_USART_DATABITS_SIX       USART_FRAME_DATABITS_SIX
#define EFR32_SIO_LLD_USART_DATABITS_SEVEN     USART_FRAME_DATABITS_SEVEN   
#define EFR32_SIO_LLD_USART_DATABITS_EIGHT     USART_FRAME_DATABITS_EIGHT   
#define EFR32_SIO_LLD_USART_DATABITS_NINE      USART_FRAME_DATABITS_NINE    
#define EFR32_SIO_LLD_USART_DATABITS_TEN       USART_FRAME_DATABITS_TEN     
#define EFR32_SIO_LLD_USART_DATABITS_ELEVEN    USART_FRAME_DATABITS_ELEVEN  
#define EFR32_SIO_LLD_USART_DATABITS_TWELVE    USART_FRAME_DATABITS_TWELVE  
#define EFR32_SIO_LLD_USART_DATABITS_THIRTEEN  USART_FRAME_DATABITS_THIRTEEN
#define EFR32_SIO_LLD_USART_DATABITS_FOURTEEN  USART_FRAME_DATABITS_FOURTEEN
#define EFR32_SIO_LLD_USART_DATABITS_FIFTEEN   USART_FRAME_DATABITS_FIFTEEN 
#define EFR32_SIO_LLD_USART_DATABITS_SIXTEEN   USART_FRAME_DATABITS_SIXTEEN 

#define EFR32_SIO_LLD_USART_PARITY_NONE        USART_FRAME_PARITY_NONE
#define EFR32_SIO_LLD_USART_PARITY_EVEN        USART_FRAME_PARITY_EVEN
#define EFR32_SIO_LLD_USART_PARITY_ODD         USART_FRAME_PARITY_ODD

#define EFR32_SIO_LLD_USART_STOPBITS_HALF       USART_FRAME_STOPBITS_HALF
#define EFR32_SIO_LLD_USART_STOPBITS_ONE        USART_FRAME_STOPBITS_ONE
#define EFR32_SIO_LLD_USART_STOPBITS_ONEANDHALF USART_FRAME_STOPBITS_ONEANDAHALF
#define EFR32_SIO_LLD_USART_STOPBITS_TWO        USART_FRAME_STOPBITS_TWO

#define EFR32_SIO_LLD_USART_8N1                (EFR32_SIO_LLD_USART_STOPBITS_ONE |\
                                                EFR32_SIO_LLD_USART_PARITY_NONE  |\
                                                EFR32_SIO_LLD_USART_DATABITS_EIGHT)

#define EFR32_SIO_LLD_USART_8E1                (EFR32_SIO_LLD_USART_STOPBITS_ONE |\
                                                EFR32_SIO_LLD_USART_PARITY_EVEN  |\
                                                EFR32_SIO_LLD_USART_DATABITS_EIGHT)

#define EFR32_SIO_LLD_USART_8O1                (EFR32_SIO_LLD_USART_STOPBITS_ONE |\
                                                EFR32_SIO_LLD_USART_PARITY_ODD   |\
                                                EFR32_SIO_LLD_USART_DATABITS_EIGHT)

#define EFR32_SIO_LLD_EUSART_DATABITS_SEVEN    EUSART_FRAMECFG_DATABITS_SEVEN
#define EFR32_SIO_LLD_EUSART_DATABITS_EIGHT    EUSART_FRAMECFG_DATABITS_EIGHT
#define EFR32_SIO_LLD_EUSART_DATABITS_NINE     EUSART_FRAMECFG_DATABITS_NINE
#define EFR32_SIO_LLD_EUSART_DATABITS_TEN      EUSART_FRAMECFG_DATABITS_TEN
#define EFR32_SIO_LLD_EUSART_DATABITS_ELEVEN   EUSART_FRAMECFG_DATABITS_ELEVEN
#define EFR32_SIO_LLD_EUSART_DATABITT_TWELVE   EUSART_FRAMECFG_DATABITS_TWELVE
#define EFR32_SIO_LLD_EUSART_DATABITS_THIRTEEN EUSART_FRAMECFG_DATABITS_THIRTEEN
#define EFR32_SIO_LLD_EUSART_DATABITS_FOURTEEN EUSART_FRAMECFG_DATABITS_FOURTEEN
#define EFR32_SIO_LLD_EUSART_DATABITS_FIFTEEN  EUSART_FRAMECFG_DATABITS_FIFTEEN
#define EFR32_SIO_LLD_EUSART_DATABITS_SIXTEEN  EUSART_FRAMECFG_DATABITS_SIXTEEN

#define EFR32_SIO_LLD_EUSART_PARITY_NONE       EUSART_FRAMECFG_PARITY_NONE
#define EFR32_SIO_LLD_EUSART_PARITY_EVEN       EUSART_FRAMECFG_PARITY_EVEN
#define EFR32_SIO_LLD_EUSART_PARITY_ODD        EUSART_FRAMECFG_PARITY_ODD

#define EFR32_SIO_LLD_EUSART_STOPBITS_HALF       EUSART_FRAMECFG_STOPBITS_HALF       
#define EFR32_SIO_LLD_EUSART_STOPBITS_ONE        EUSART_FRAMECFG_STOPBITS_ONE        
#define EFR32_SIO_LLD_EUSART_STOPBITS_ONEANDHALF EUSART_FRAMECFG_STOPBITS_ONEANDAHALF
#define EFR32_SIO_LLD_EUSART_STOPBITS_TWO        EUSART_FRAMECFG_STOPBITS_TWO        

#define EFR32_SIO_LLD_EUSART_8N1               (EFR32_SIO_LLD_EUSART_STOPBITS_ONE |\
                                                EFR32_SIO_LLD_EUSART_PARITY_NONE  |\
                                                EFR32_SIO_LLD_EUSART_DATABITS_EIGHT)

#define EFR32_SIO_LLD_EUSART_8E1               (EFR32_SIO_LLD_EUSART_STOPBITS_ONE |\
                                                EFR32_SIO_LLD_EUSART_PARITY_EVEN  |\
                                                EFR32_SIO_LLD_EUSART_DATABITS_EIGHT)

#define EFR32_SIO_LLD_EUSART_8O1               (EFR32_SIO_LLD_EUSART_STOPBITS_ONE |\
                                                EFR32_SIO_LLD_EUSART_PARITY_ODD   |\
                                                EFR32_SIO_LLD_EUSART_DATABITS_EIGHT)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    PLATFORM configuration options
 * @{
 */
/**
 * @brief   SIO driver enable switch.
 * @details If set to @p TRUE the support for SIO1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(EFR32_SIO_USE_EUSART1) || defined(__DOXYGEN__)
#define EFR32_SIO_USE_EUSART1             FALSE
#endif

/**
 * @brief   SIO driver enable switch.
 * @details If set to @p TRUE the support for SIO2 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(EFR32_SIO_USE_EUSART2) || defined(__DOXYGEN__)
#define EFR32_SIO_USE_EUSART2             FALSE
#endif

/**
 * @brief   SIO driver enable switch.
 * @details If set to @p TRUE the support for SIO3 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(EFR32_SIO_USE_EUSART3) || defined(__DOXYGEN__)
#define EFR32_SIO_USE_EUSART3             FALSE
#endif
/** @} */

/**
 * @brief   SIO driver enable switch.
 * @details If set to @p TRUE the support for SIO4 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(EFR32_SIO_USE_USART1) || defined(__DOXYGEN__)
#define EFR32_SIO_USE_USART1              FALSE
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if EFR32_SIO_USE_EUSART1 && !EFR32_HAS_EUSART1
#error "EUSART1 not present in the selected device"
#endif

#if EFR32_SIO_USE_EUSART2 && !EFR32_HAS_EUSART2
#error "EUSART2 not present in the selected device"
#endif

#if EFR32_SIO_USE_EUSART3 && !EFR32_HAS_EUSART3
#error "EUSART3 not present in the selected device"
#endif

#if EFR32_SIO_USE_USART1 && !EFR32_HAS_USART1
#error "USART1 not present in the selected device"
#endif


/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Low level fields of the SIO driver structure.
 */
#define sio_lld_driver_fields                                               \
  /* Pointer to the EUSARTx or USARTx registers block.*/                    \
  void *usart;                                                              \
/* Clock frequency for the associated USART/UART.*/                         \
  uint32_t clock

/**
 * @brief   Low level fields of the SIO configuration structure.
 */
/* IrDA control values for sio_lld_config_fields.irctrl. */
#define EFR32_SIO_IRDA_NONE     0U  /**< IrDA disabled (default). */
#define EFR32_SIO_IRDA_HF       1U  /**< HF IrDA mode (IRHFCFG) - for HFRCO/HFXO clocked EUSART. */
#define EFR32_SIO_IRDA_LF       2U  /**< LF IrDA mode (IRLFCFG) - for LFXO/LFRCO clocked EUSART in EM2/EM3. */
#define EFR32_SIO_IRDA_AUTO     3U  /**< Auto: LF if clock <= 40kHz (LFXO/LFRCO), else HF. */

/*
 * IrDA pulse width presets for HF mode (IRHFCFG_IRHFPW field).
 * Standard IrDA uses 3/16 of a bit time.
 */
#define EFR32_SIO_IRDA_PW_1_16   EUSART_IRHFCFG_IRHFPW_ONE    /**< 1/16 bit time */
#define EFR32_SIO_IRDA_PW_2_16   EUSART_IRHFCFG_IRHFPW_TWO    /**< 2/16 bit time */
#define EFR32_SIO_IRDA_PW_3_16   EUSART_IRHFCFG_IRHFPW_THREE  /**< 3/16 bit time (standard IrDA) */
#define EFR32_SIO_IRDA_PW_4_16   EUSART_IRHFCFG_IRHFPW_FOUR   /**< 4/16 bit time */

/*
 * M-Bus (Meter-Bus) IrDA preset for 2400 baud, 8E1, LF IrDA mode.
 *
 * Use when the EUSART is clocked from LFXO/LFRCO (32768 Hz) — typical
 * for EM2/EM3 low-power operation. Oversampling is disabled because
 * 32768/2400 = 13.6 clocks/bit is enough without OVS. The LF IrDA
 * pulse generator (IRLFCFG_IRLFEN) automatically produces 3/16-bit-time
 * pulses from the LF clock domain.
 *
 * Requires EFR32_EUSART1CLK (or EUSART23CLK) to be LFXO or LFRCO.
 */
#define EFR32_SIO_LLD_IRDA_2400_8E1_LF                                           \
  {                                                                             \
    .baud     = 2400U,                                                          \
    .cfg0     = (0U << 0) | (4U << 5),  /* ASYNC, OVS disabled for LF clock */  \
    .framecfg = EFR32_SIO_LLD_EUSART_8E1,                                       \
    .irhfcg   = 0,                                                              \
    .irlfcfg  = 0,                                                              \
    .irctrl   = EFR32_SIO_IRDA_LF,                                              \
  }

/*
 * IrDA preset for 2400 baud, 8E1, HF IrDA mode (3/16 pulse, with RX filter).
 *
 * Use when the EUSART is clocked from HFRCO/HFXO (e.g. 19-78 MHz) in
 * EM0/EM1. Standard 16x oversampling. The HF IrDA modulator generates
 * 3/16-bit-time pulses; the RX filter rejects short glitches.
 */
#define EFR32_SIO_LLD_IRDA_2400_8E1_HF                                           \
  {                                                                             \
    .baud     = 2400U,                                                          \
    .cfg0     = (0U << 0) | (0U << 5),  /* ASYNC, 16x oversampling */           \
    .framecfg = EFR32_SIO_LLD_EUSART_8E1,                                       \
    .irhfcg   = EFR32_SIO_IRDA_PW_3_16 | EUSART_IRHFCFG_IRHFFILT,                \
    .irlfcfg  = 0,                                                              \
    .irctrl   = EFR32_SIO_IRDA_HF,                                              \
  }

/*
 * IrDA preset for 9600 baud, 8E1, HF IrDA mode (3/16 pulse, with RX filter).
 *
 * 9600 baud is too fast for LF IrDA from a 32768 Hz clock (needs >= 3
 * clocks/bit, 32768/9600 = 3.4 — marginal). Use HF IrDA with the EUSART
 * clocked from HFRCO/HFXO. Standard 16x oversampling.
 * At 9600 baud, 3/16 bit time = 19.5 µs IrDA pulse.
 */
#define EFR32_SIO_LLD_IRDA_9600_8E1_HF                                           \
  {                                                                             \
    .baud     = 9600U,                                                          \
    .cfg0     = (0U << 0) | (0U << 5),  /* ASYNC, 16x oversampling */           \
    .framecfg = EFR32_SIO_LLD_EUSART_8E1,                                       \
    .irhfcg   = EFR32_SIO_IRDA_PW_3_16 | EUSART_IRHFCFG_IRHFFILT,                \
    .irlfcfg  = 0,                                                              \
    .irctrl   = EFR32_SIO_IRDA_HF,                                              \
  }

/*
 * IrDA preset for 9600 baud, 8E1, LF IrDA mode.
 *
 * Usable when EUSART clocked from LFXO (32768 Hz): 32768/9600 = 3.41
 * clocks/bit — just above the 3-clock minimum. OVS disabled.
 * Useful for EM2/EM3 operation at 9600 baud with LF IrDA pulses.
 */
#define EFR32_SIO_LLD_IRDA_9600_8E1_LF                                           \
  {                                                                             \
    .baud     = 9600U,                                                          \
    .cfg0     = (0U << 0) | (4U << 5),  /* ASYNC, OVS disabled */               \
    .framecfg = EFR32_SIO_LLD_EUSART_8E1,                                       \
    .irhfcg   = 0,                                                              \
    .irlfcfg  = 0,                                                              \
    .irctrl   = EFR32_SIO_IRDA_LF,                                              \
  }

/* Backward-compatible alias. */
#define EFR32_SIO_LLD_MBUS_2400_8E1   EFR32_SIO_LLD_IRDA_2400_8E1_LF

#define sio_lld_config_fields                                               \
  uint32_t baud;    /**< Desired baud rate. */                              \
  uint32_t cfg0;    /**< EUSART_CFG _or_ USART_CTRL. */                     \
  uint32_t framecfg; /**< EUSART_FRAMECFG _or_ USART_FRAME. */             \
  uint32_t irhfcg;  /**< IrDA HF config (0=disabled), EUSART_IRHFCFG. */    \
  uint32_t irlfcfg; /**< IrDA LF config (0=disabled), EUSART_IRLFCFG. */    \
  uint32_t irctrl;  /**< IrDA control: 0=none, 1=HF, 2=LF (M-Bus 2400). */

/**
 * @brief   Determines the state of the RX FIFO.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The RX FIFO state.
 * @retval false        if RX FIFO is not empty
 * @retval true         if RX FIFO is empty
 *
 * @notapi
 */
#define sio_lld_is_rx_empty(siop) _sio_lld_is_rx_empty(siop)

/**
 * @brief   Determines the activity state of the receiver.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The RX activity state.
 * @retval false        if RX is in active state.
 * @retval true         if RX is in idle state.
 *
 * @notapi
 */
#define sio_lld_is_rx_idle(siop) _sio_lld_is_rx_idle(siop)

/**
 * @brief   Determines if RX has pending error events to be read and cleared.
 * @note    Only error and protocol errors are handled, data events are not
 *          considered.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The RX error events.
 * @retval false        if RX has no pending events
 * @retval true         if RX has pending events
 *
 * @notapi
 */
#define sio_lld_has_rx_errors(siop) _sio_lld_has_rx_errors(siop)

/**
 * @brief   Determines the state of the TX FIFO.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The TX FIFO state.
 * @retval false        if TX FIFO is not full
 * @retval true         if TX FIFO is full
 *
 * @notapi
 */
#define sio_lld_is_tx_full(siop) _sio_lld_is_tx_full(siop)

/**
 * @brief   Determines the transmission state.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The TX FIFO state.
 * @retval false        if transmission is idle
 * @retval true         if transmission is ongoing
 *
 * @notapi
 */
#define sio_lld_is_tx_ongoing(siop) _sio_lld_is_tx_ongoing(siop)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (EFR32_SIO_USE_EUSART1 == TRUE) && !defined(__DOXYGEN__)
extern SIODriver SIOD1;
#endif

#if (EFR32_SIO_USE_EUSART2 == TRUE) && !defined(__DOXYGEN__)
extern SIODriver SIOD2;
#endif

#if (EFR32_SIO_USE_EUSART3 == TRUE) && !defined(__DOXYGEN__)
extern SIODriver SIOD3;
#endif

#if (EFR32_SIO_USE_USART1 == TRUE) && !defined(__DOXYGEN__)
extern SIODriver SIOD4;
#endif

#ifdef __cplusplus
extern "C"
{
#endif
void sio_lld_init(void);
msg_t  sio_lld_start(SIODriver* siop);
void sio_lld_stop(SIODriver* siop);
void sio_lld_update_enable_flags(SIODriver* siop);
sioevents_t sio_lld_get_and_clear_errors(SIODriver* siop);
sioevents_t sio_lld_get_and_clear_events(SIODriver* siop);
sioevents_t sio_lld_get_events(SIODriver* siop);
size_t sio_lld_read(SIODriver* siop, uint8_t* buffer, size_t n);
size_t sio_lld_write(SIODriver* siop, const uint8_t* buffer, size_t n);
msg_t sio_lld_get(SIODriver* siop);
void sio_lld_put(SIODriver* siop, uint_fast16_t data);
msg_t sio_lld_control(SIODriver* siop, unsigned int operation, void* arg);
void sio_lld_serve_interrupt(SIODriver* siop);

/**
 * @brief   Change the baud rate of a running SIO driver at runtime.
 * @details Useful for M-Bus which may switch between 2400 and 9600 baud.
 *          Also updates the IrDA mode automatically (LF if clock <= 40kHz).
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @param[in] baud      new baud rate
 * @param[in] irda_mode EFR32_SIO_IRDA_NONE/HF/LF/AUTO
 *
 * @notapi
 */
void sio_lld_set_baud(SIODriver *siop, uint32_t baud, uint32_t irda_mode);

bool _sio_lld_is_rx_empty(SIODriver* siop);
bool _sio_lld_is_rx_idle(SIODriver* siop);
bool _sio_lld_has_rx_errors(SIODriver* siop);
bool _sio_lld_is_tx_full(SIODriver* siop);
bool _sio_lld_is_tx_ongoing(SIODriver* siop);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SIO == TRUE */

#endif /* HAL_SIO_LLD_H */

/** @} */