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
 * @file    IMX23/IMX23xx/icoll.S
 * @brief   i.MX23 ICOLL shared IRQ trampoline.
 *
 * @details Entry conditions (set up by the ARM port Irq_Handler in
 *          chcoreasm.S):
 *          - r0-r3, r12, lr already saved on the IRQ stack
 *          - lr = _irq_ret_arm (return point in Irq_Handler)
 *          - the ISR address has been read from HW_ICOLL_VECTOR; all vector
 *            table entries point here, so the vector number is recovered
 *            from HW_ICOLL_STAT.
 *          The trampoline must return to _irq_ret_arm with r0 = the bool
 *          result of the CH_IRQ_HANDLER (nonzero = reschedule requested).
 *
 * @addtogroup IMX23
 * @{
 */

#if !defined(__DOXYGEN__)

                .text
                .code   32
                .balign 4
                .global imx23_irq_trampoline
                .extern imx23_irq_handlers

imx23_irq_trampoline:
                stmfd   sp!, {r4, lr}           /* save handler ptr and return */

                /* Acknowledge ISR entry: write to HW_ICOLL_VECTOR (data
                   does not matter). */
                ldr     r1, =0x80000000         /* HW_ICOLL_VECTOR */
                str     r1, [r1]

                /* Recover the vector number from HW_ICOLL_STAT. */
                ldr     r1, =0x80000070         /* HW_ICOLL_STAT */
                ldr     r2, [r1]
                and     r2, r2, #0x7f           /* VECTOR_NUMBER */

                /* handler = imx23_irq_handlers[n] */
                ldr     r1, =imx23_irq_handlers
                ldr     r4, [r1, r2, lsl #2]
                cmp     r4, #0
                moveq   r0, #0                  /* no handler registered */
                beq     1f
                mov     lr, pc
                bx      r4                      /* call handler, ARM mode (bit0=0) */
1:
                /* Acknowledge completion: LEVELACK = 1 << priority(n). */
                ldr     r1, =0x80000120         /* HW_ICOLL_INTERRUPT0 */
                ldr     r3, [r1, r2, lsl #4]
                and     r3, r3, #3              /* PRIORITY */
                mov     r2, #1
                mov     r2, r2, lsl r3
                ldr     r1, =0x80000010         /* HW_ICOLL_LEVELACK */
                str     r2, [r1]

                ldmfd   sp!, {r4, lr}
                bx      lr                      /* back to _irq_ret_arm, r0 = result */

#endif /* !defined(__DOXYGEN__) */

/** @} */
