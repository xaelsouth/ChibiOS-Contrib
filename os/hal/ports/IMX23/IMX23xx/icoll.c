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
 * @file    IMX23/IMX23xx/icoll.c
 * @brief   i.MX23 interrupt collector (ICOLL) support code.
 *
 * @addtogroup IMX23
 * @{
 */

#include "hal.h"
#include "icoll.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/* Trampoline defined in icoll.S, entered via "bx" from the port IRQ code
   after it has read HW_ICOLL_VECTOR. */
extern void imx23_irq_trampoline(void);

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/* Handler table, indexed by ICOLL vector number. NULL = not registered. */
bool (*imx23_irq_handlers[IMX23_ICOLL_NUM_IRQS])(void);

/* The vector table read by the ICOLL hardware (VBASE). All entries point to
   the single trampoline, which recovers the vector number from STAT. */
uint32_t imx23_icoll_vectors[IMX23_ICOLL_NUM_IRQS];

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   ICOLL initialization.
 * @note    Called from hal_lld_init().
 *
 * @notapi
 */
void icoll_init(void) {
  unsigned i;

  /* All sources disabled, lowest priority, no FIQ steering. */
  for (i = 0U; i < IMX23_ICOLL_NUM_IRQS; i++) {
    IMX23_REG_CLR(IMX23_ICOLL_INTERRUPT0 + i * 0x10U,
                  IMX23_ICOLL_INTR_ENABLE | IMX23_ICOLL_INTR_ENFIQ |
                  IMX23_ICOLL_INTR_SOFTIRQ | IMX23_ICOLL_INTR_PRIORITY);
    imx23_irq_handlers[i] = NULL;
    imx23_icoll_vectors[i] = (uint32_t)(uintptr_t)&imx23_irq_trampoline;
  }

  /* Vector table base and 4-byte pitch, IRQ routed to the CPU. */
  IMX23_REG(IMX23_ICOLL_VBASE) = (uint32_t)(uintptr_t)imx23_icoll_vectors;
  IMX23_REG(IMX23_ICOLL_CTRL) = IMX23_ICOLL_CTRL_IRQ_FINAL_ENABLE |
                                IMX23_ICOLL_CTRL_VECTOR_PITCH;
}

/**
 * @brief   Registers an IRQ handler for a vector number.
 *
 * @param[in] irq       ICOLL vector number
 * @param[in] handler   CH_IRQ_HANDLER function (bool (*)(void))
 *
 * @notapi
 */
void icoll_register_handler(unsigned irq, bool (*handler)(void)) {

  osalDbgCheck(irq < IMX23_ICOLL_NUM_IRQS);
  imx23_irq_handlers[irq] = handler;
}

/**
 * @brief   Enables an interrupt source.
 *
 * @notapi
 */
void icoll_irq_enable(unsigned irq) {

  osalDbgCheck(irq < IMX23_ICOLL_NUM_IRQS);
  IMX23_REG_SET(IMX23_ICOLL_INTERRUPT0 + irq * 0x10U, IMX23_ICOLL_INTR_ENABLE);
}

/**
 * @brief   Disables an interrupt source.
 *
 * @notapi
 */
void icoll_irq_disable(unsigned irq) {

  osalDbgCheck(irq < IMX23_ICOLL_NUM_IRQS);
  IMX23_REG_CLR(IMX23_ICOLL_INTERRUPT0 + irq * 0x10U, IMX23_ICOLL_INTR_ENABLE);
}

/**
 * @brief   Sets the priority level of an interrupt source.
 *
 * @notapi
 */
void icoll_irq_set_priority(unsigned irq, unsigned level) {

  osalDbgCheck(irq < IMX23_ICOLL_NUM_IRQS);
  osalDbgCheck(level <= 3U);
  /* Must be disabled while changing priority (RM warning). */
  icoll_irq_disable(irq);
  IMX23_REG(IMX23_ICOLL_INTERRUPT0 + irq * 0x10U) =
      (IMX23_REG(IMX23_ICOLL_INTERRUPT0 + irq * 0x10U) &
       ~IMX23_ICOLL_INTR_PRIORITY) | level;
}

/**
 * @brief   Forces a software interrupt on a source.
 *
 * @notapi
 */
void icoll_irq_soft(unsigned irq) {

  osalDbgCheck(irq < IMX23_ICOLL_NUM_IRQS);
  IMX23_REG_SET(IMX23_ICOLL_INTERRUPT0 + irq * 0x10U, IMX23_ICOLL_INTR_SOFTIRQ);
}

/** @} */
