/*
    ChibiOS - Copyright (C) 2026 Xael South
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
        http://www.apache.org/licenses/LICENSE-2.0
*/

/**
 * @file    EUSARTv1/efr32_irda.h
 * @brief   Shared IrDA helper for EUSART-based drivers (SIO, UART, SERIAL).
 *
 * @addtogroup EUSART
 * @{
 */

#ifndef EFR32_IRDA_H
#define EFR32_IRDA_H

/* IrDA control values. */
#define EFR32_IRDA_NONE     0U  /**< IrDA disabled. */
#define EFR32_IRDA_HF       1U  /**< HF IrDA (IRHFCFG) — HFRCO/HFXO clocked. */
#define EFR32_IRDA_LF       2U  /**< LF IrDA (IRLFCFG) — LFXO/LFRCO in EM2/EM3. */
#define EFR32_IRDA_AUTO     3U  /**< Auto: LF if clock <= 40kHz, else HF. */

/* IrDA pulse width presets for HF mode (3/16 = standard IrDA). */
#define EFR32_IRDA_PW_1_16   EUSART_IRHFCFG_IRHFPW_ONE
#define EFR32_IRDA_PW_2_16   EUSART_IRHFCFG_IRHFPW_TWO
#define EFR32_IRDA_PW_3_16   EUSART_IRHFCFG_IRHFPW_THREE   /**< standard IrDA */
#define EFR32_IRDA_PW_4_16   EUSART_IRHFCFG_IRHFPW_FOUR

/**
 * @brief   Apply IrDA configuration to an EUSART peripheral.
 *
 * @param[in] u         pointer to EUSART_TypeDef
 * @param[in] irctrl    EFR32_IRDA_* mode
 * @param[in] irhfcg    HF config value (used if mode is HF or AUTO→HF)
 * @param[in] irlfcfg   LF config value (used if mode is LF or AUTO→LF)
 * @param[in] clock     EUSART clock frequency (for AUTO mode selection)
 */
__STATIC_INLINE void efr32_irda_apply(EUSART_TypeDef *u, uint32_t irctrl,
                                      uint32_t irhfcg, uint32_t irlfcfg,
                                      uint32_t clock) {
  uint32_t mode = irctrl;
  if (mode == EFR32_IRDA_AUTO)
    mode = (clock <= 40000U) ? EFR32_IRDA_LF : EFR32_IRDA_HF;

  if (mode == EFR32_IRDA_HF) {
    u->IRHFCFG = irhfcg | EUSART_IRHFCFG_IRHFEN;
    u->IRLFCFG = 0;
  } else if (mode == EFR32_IRDA_LF) {
    u->IRLFCFG = irlfcfg | EUSART_IRLFCFG_IRLFEN;
    u->IRHFCFG = 0;
  } else {
    u->IRHFCFG = 0;
    u->IRLFCFG = 0;
  }
}

#endif /* EFR32_IRDA_H */
/** @} */
