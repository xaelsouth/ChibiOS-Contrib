/*
    ChibiOS - Copyright (C) 2006-2026 Xael South.

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
 * @file    IMX23/IMX23xx/icoll.h
 * @brief   i.MX23 interrupt collector (ICOLL) support.
 *
 * @details The classic ARM port expects ARM_IRQ_VECTOR_REG (HW_ICOLL_VECTOR)
 *          to return the address of the ISR to invoke. The ICOLL computes it
 *          as VBASE + vector_number * 4 * VECTOR_PITCH, so VBASE is pointed
 *          to a 128-entry table whose entries all reference a single
 *          trampoline (icoll.S). The trampoline acknowledges the interrupt
 *          (write to VECTOR, write to LEVELACK) and dispatches to the
 *          registered CH_IRQ_HANDLER through imx23_irq_handlers[].
 *
 * @addtogroup IMX23
 * @{
 */

#ifndef ICOLL_H
#define ICOLL_H

#include "imx23.h"

#ifdef __cplusplus
extern "C" {
#endif
  void icoll_init(void);
  void icoll_register_handler(unsigned irq, bool (*handler)(void));
  void icoll_irq_enable(unsigned irq);
  void icoll_irq_disable(unsigned irq);
  void icoll_irq_set_priority(unsigned irq, unsigned level); /* 0..3, 3 highest */
  void icoll_irq_soft(unsigned irq);
#ifdef __cplusplus
}
#endif

#endif /* ICOLL_H */

/** @} */
