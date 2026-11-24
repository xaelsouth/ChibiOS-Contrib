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
 * @file    LESENSEv1/hal_lesense_lld.h
 * @brief   LESENSE (Low Energy Sensor Interface) low-level driver header.
 *
 * @details LESENSE autonomously scans up to 16 sensor channels in EM2.
 *          Ideal for inductive sensing (e.g. water meter rotating disc with
 *          metal element over an LC coil), capacitive buttons, etc.
 *
 * @addtogroup LESENSE
 * @{
 */

#ifndef HAL_LESENSE_LLD_H
#define HAL_LESENSE_LLD_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

#define EFR32_LESENSE_MAX_CHANNELS    16

/* Scan modes. */
#define EFR32_LESENSE_SCAN_PERIODIC   LESENSE_CFG_SCANMODE_PERIODIC
#define EFR32_LESENSE_SCAN_ONESHOT    LESENSE_CFG_SCANMODE_ONESHOT
#define EFR32_LESENSE_SCAN_PRS        LESENSE_CFG_SCANMODE_PRS

/* Channel evaluation modes. */
#define EFR32_LESENSE_EVAL_THRES      LESENSE_CH_EVALCFG_MODE_THRES
#define EFR32_LESENSE_EVAL_SLIDINGWIN LESENSE_CH_EVALCFG_MODE_SLIDINGWIN
#define EFR32_LESENSE_EVAL_STEPDET    LESENSE_CH_EVALCFG_MODE_STEPDET

/* Channel excitation modes. */
#define EFR32_LESENSE_EX_DISABLE      LESENSE_CH_INTERACT_EXMODE_DISABLE
#define EFR32_LESENSE_EX_HIGH         LESENSE_CH_INTERACT_EXMODE_HIGH
#define EFR32_LESENSE_EX_LOW          LESENSE_CH_INTERACT_EXMODE_LOW
#define EFR32_LESENSE_EX_DACOUT       LESENSE_CH_INTERACT_EXMODE_DACOUT

/* Decoder control: count mode pulses DECOUT0 on every count-up event
   (STx_ARC with PRSACT=UP) and DECOUT1 with the direction — route DECOUT0
   via PRS to PCNT to count sensor hits fully autonomously, no CPU IRQ. */
#define EFR32_LESENSE_DEC_PRSCNT      LESENSE_DECCTRL_PRSCNT

/* Convenience value for STx_ARC.PRSACT in count mode. */
#define EFR32_LESENSE_ARC_COUNT_UP    LESENSE_ST_ARC_PRSACT_UP

/** Number of decoder FSM arcs (ST0..ST63). */
#define EFR32_LESENSE_MAX_ARCS        64

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(EFR32_LESENSE_USE_LESENSE) || defined(__DOXYGEN__)
#define EFR32_LESENSE_USE_LESENSE       FALSE
#endif

#if !defined(EFR32_LESENSE_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_LESENSE_IRQ_PRIORITY      3
#endif

/* DMA stream for result FIFO draining. EFR32_DMA_STREAM_ID_ANY = auto-alloc,
   specific 0-7 = pin, -1 = disable DMA (IRQ-only mode). */
#if !defined(EFR32_LESENSE_DMA_STREAM) || defined(__DOXYGEN__)
#define EFR32_LESENSE_DMA_STREAM        EFR32_DMA_STREAM_ID_ANY
#endif
#if !defined(EFR32_LESENSE_DMA_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_LESENSE_DMA_IRQ_PRIORITY  5
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if EFR32_LESENSE_USE_LESENSE && !EFR32_HAS_LESENSE
#error "LESENSE not present in the selected device"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

typedef enum {
  LESENSE_UNINIT = 0,
  LESENSE_STOP = 1,
  LESENSE_READY = 2,
  LESENSE_SCANNING = 3
} lesense_state_t;

/**
 * @brief   LESENSE channel configuration.
 */
typedef struct {
  uint8_t   pin;          /**< GPIO pin for this channel (0-15 maps to ch0-15). */
  uint32_t  interact;     /**< CHx_INTERACT value (threshold, exmode, altex...). */
  uint32_t  evalcfg;      /**< CHx_EVALCFG value (mode, etc.). */
  uint32_t  evalthres;   /**< CHx_EVALTHRES value. */
} LESENSEChannelConfig;

/**
 * @brief   LESENSE driver configuration.
 */
typedef struct {
  uint32_t  scanmode;     /**< EFR32_LESENSE_SCAN_*. */
  uint32_t  timctrl;      /**< TIMCTRL value (scan frequency, startup, etc.). */
  uint32_t  perctrl;      /**< PERCTRL value (ACMP modes, warmup, etc.). */
  uint32_t  prsctrl;      /**< PRSCTRL value (DECCMP PRS output enable/mask/val;
                               the decoder DECOUT0..2 producers are driven by the
                               decoder arcs, see arcs[] below). */
  uint32_t  decctrl;      /**< DECCTRL value (decoder control), e.g.
                               EFR32_LESENSE_DEC_PRSCNT for count mode.
                               0 = reset default (decoder on, no count mode). */
  const uint32_t *arcs;   /**< Decoder FSM arcs: ST[0..num_arcs-1].ARC values,
                               NULL = leave arcs at reset. Example for counting
                               channel-0 hits: arc0 = SCOMP(1) | SMASK(0xE) |
                               CURSTATE(0) | PRSACT(EFR32_LESENSE_ARC_COUNT_UP)
                               | NEXTSTATE(0) — every ch0 hit pulses DECOUT0. */
  uint8_t   num_arcs;     /**< Number of valid entries in arcs (0..64). */
  uint8_t   num_channels; /**< Number of active channels (1-16). */
  LESENSEChannelConfig channels[EFR32_LESENSE_MAX_CHANNELS];
  /**
   * @brief   Callback invoked on scan complete (channel hit or buffer full).
   * @param[in] result  SCANRES register value (bitmask of hit channels).
   * @note    If NULL, no interrupt is enabled at all: the driver runs fully
   *          autonomously in EM2 and per-scan detection is meant to be
   *          consumed via the decoder PRS outputs (arcs[]/decctrl), e.g.
   *          DECOUT0 -> PRS -> PCNT. This avoids one CPU wakeup per scan.
   */
  void (*callback)(uint32_t scanres, void *arg);
  void     *callback_arg;
} LESENSEConfig;

typedef struct {
  lesense_state_t  state;
  const LESENSEConfig *config;
  uint32_t         scanres;  /**< Last scan result. */
  const void      *dma;      /**< DMA stream for result FIFO (NULL if none). */
} LESENSEDriver;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if EFR32_LESENSE_USE_LESENSE && !defined(__DOXYGEN__)
extern LESENSEDriver LESENSED1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void lesense_lld_init(void);
  void lesense_lld_start(LESENSEDriver *lp, const LESENSEConfig *cfg);
  void lesense_lld_stop(LESENSEDriver *lp);
  void lesense_lld_start_scan(LESENSEDriver *lp);
  void lesense_lld_stop_scan(LESENSEDriver *lp);
  uint32_t lesense_lld_get_scanres(LESENSEDriver *lp);
  void lesense_lld_set_scan_hz(LESENSEDriver *lp, float hz);
  void lesense_lld_start_fifo_dma(LESENSEDriver *lp, uint32_t *buf, size_t n);
  void lesense_lld_stop_fifo_dma(LESENSEDriver *lp);
  void lesense_lld_serve_interrupt(LESENSEDriver *lp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_LESENSE_LLD_H */

/** @} */
