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
 * @file    swo_trace.c
 * @brief   SWO (Single Wire Output) / ITM trace stream code.
 *
 * @addtogroup SWO_TRACE
 * @{
 */

#include "swo_trace.h"

#if defined(TPI_BASE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* Number of FIFO-full poll iterations before dropping a byte. */
#define SWO_TRACE_POLL_TRIES    10000U

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   SWO trace output stream.
 */
BaseSequentialStream SWOStream;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static size_t swo_write(void *ip, const uint8_t *bp, size_t n) {
  size_t i, written = 0;

  (void)ip;
  for (i = 0; i < n; i++) {
    uint32_t tries = SWO_TRACE_POLL_TRIES;
    /* Wait for FIFO space, but give up so logging never hangs. */
    while (ITM->PORT[0].u32 == 0 && --tries > 0) {
    }
    if (ITM->PORT[0].u32 == 0) {
      break;    /* No reader attached: drop the rest. */
    }
    ITM->PORT[0].u8 = bp[i];
    written++;
  }
  return written;
}

static size_t swo_read(void *ip, uint8_t *bp, size_t n) {

  /* SWO/ITM is output-only: park the reader thread. */
  (void)ip;
  (void)bp;
  (void)n;
  while (!chThdShouldTerminateX()) {
    chThdSleepMilliseconds(100);
  }
  return 0;
}

static msg_t swo_put(void *ip, uint8_t b) {
  uint8_t buf[1];

  buf[0] = b;
  return (swo_write(ip, buf, 1) == 1) ? MSG_OK : MSG_TIMEOUT;
}

static msg_t swo_get(void *ip) {

  (void)ip;
  while (!chThdShouldTerminateX()) {
    chThdSleepMilliseconds(100);
  }
  return MSG_TIMEOUT;
}

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Virtual Methods Table for the SWO stream.
 */
static const struct BaseSequentialStreamVMT swo_vmt = {
  (size_t)0,
  swo_write,
  swo_read,
  swo_put,
  swo_get
};

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Initializes the SWO trace block (TPI + ITM + DWT).
 * @note    The SWO pin muxing is board-specific and left to the
 *          application (PAL).
 *
 * @param[in] config    pointer to an @p SWOConfig structure
 *
 * @init
 */
void swoTraceInit(const SWOConfig *config) {

  osalDbgAssert(config != NULL, "NULL config");

  /* Enable trace in the core debug block. */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

  /* TPI: NRZ SWO protocol, prescaler for the SWO baud rate. */
  TPI->SPPR = 2U;                       /* SWO, asynchronous NRZ */
  TPI->ACPR = config->prescaler;
  TPI->FFCR = 0U;

  /* DWT: unlock and enable the cycle counter (for ITM timestamps). */
  DWT->LAR = SWO_TRACE_UNLOCK_KEY;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  /* ITM: unlock, no privileged access restriction, enable SWO + ports. */
  ITM->LAR = SWO_TRACE_UNLOCK_KEY;
  ITM->TPR = 0U;
  ITM->TCR = ITM_TCR_ITMENA_Msk | ITM_TCR_TSENA_Msk |
             ITM_TCR_SYNCENA_Msk | ITM_TCR_SWOENA_Msk;
  ITM->TER = config->stim_ports;

  SWOStream.vmt = &swo_vmt;
}

#endif /* defined(TPI_BASE) */

/** @}*/
