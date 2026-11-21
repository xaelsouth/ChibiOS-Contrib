/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

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
 * @file    IMX23/IMX23xx/imx23.h
 * @brief   i.MX23 (ARM926EJ-S) register map and IRQ numbers.
 *
 * @details Register bases from the i.MX23 Reference Manual memory map
 *          (Table 40-1), bit definitions from the RM and from the
 *          U-Boot/Linux mxs code.
 *
 * @addtogroup IMX23
 * @{
 */

#ifndef IMX23_H
#define IMX23_H

#include <stdint.h>

/*===========================================================================*/
/* Memory map (i.MX23 RM, Table 40-1).                                       */
/*===========================================================================*/

#define IMX23_OCRAM_BASE        0x00000000U     /* 32KB on-chip SRAM (aliased
                                                   every 32KB up to 0x3FFFFFFF) */
#define IMX23_DRAM_BASE         0x40000000U     /* EMI CS0, external SDRAM      */
#define IMX23_ICOLL_BASE        0x80000000U
#define IMX23_APBH_BASE         0x80004000U
#define IMX23_ECC8_BASE         0x80008000U
#define IMX23_BCH_BASE          0x8000A000U
#define IMX23_GPMI_BASE         0x8000C000U
#define IMX23_SSP0_BASE         0x80010000U
#define IMX23_ETM_BASE          0x80014000U
#define IMX23_PINCTRL_BASE      0x80018000U
#define IMX23_DIGCTL_BASE       0x8001C000U
#define IMX23_EMI_BASE          0x80020000U
#define IMX23_APBX_BASE         0x80024000U
#define IMX23_DCP_BASE          0x80028000U
#define IMX23_PXP_BASE          0x8002A000U
#define IMX23_OCOTP_BASE        0x8002C000U
#define IMX23_AXI_BASE          0x8002E000U
#define IMX23_LCDIF_BASE        0x80030000U
#define IMX23_SSP1_BASE         0x80034000U
#define IMX23_TVENC_BASE        0x80038000U
#define IMX23_CLKCTRL_BASE      0x80040000U
#define IMX23_SAIF0_BASE        0x80042000U
#define IMX23_POWER_BASE        0x80044000U
#define IMX23_SAIF1_BASE        0x80046000U
#define IMX23_AUDIOOUT_BASE     0x80048000U
#define IMX23_AUDIOIN_BASE      0x8004C000U
#define IMX23_LRADC_BASE        0x80050000U
#define IMX23_SPDIF_BASE        0x80054000U
#define IMX23_I2C0_BASE         0x80058000U
#define IMX23_RTC_BASE          0x8005C000U
#define IMX23_PWM_BASE          0x80064000U
#define IMX23_TIMROT_BASE       0x80068000U
#define IMX23_AUART0_BASE       0x8006C000U
#define IMX23_AUART1_BASE       0x8006E000U
#define IMX23_DUART_BASE        0x80070000U     /* ARM PL011 debug UART        */
#define IMX23_USBPHY0_BASE      0x8007C000U
#define IMX23_USBCTRL0_BASE     0x80080000U
#define IMX23_DRAM_REGS_BASE    0x800E0000U

/*===========================================================================*/
/* ICOLL (interrupt collector) registers.                                    */
/*===========================================================================*/

#define IMX23_ICOLL_VECTOR      (IMX23_ICOLL_BASE + 0x000U) /* read: vector addr */
#define IMX23_ICOLL_LEVELACK    (IMX23_ICOLL_BASE + 0x010U)
#define IMX23_ICOLL_CTRL        (IMX23_ICOLL_BASE + 0x020U)
#define IMX23_ICOLL_VBASE       (IMX23_ICOLL_BASE + 0x040U)
#define IMX23_ICOLL_STAT        (IMX23_ICOLL_BASE + 0x070U)
#define IMX23_ICOLL_INTERRUPT0  (IMX23_ICOLL_BASE + 0x120U) /* n: +0x10*n, SET +4, CLR +8, TOG +12 */

#define IMX23_ICOLL_CTRL_IRQ_FINAL_ENABLE   (1U << 16)
#define IMX23_ICOLL_CTRL_FIQ_FINAL_ENABLE   (1U << 17)
#define IMX23_ICOLL_CTRL_VECTOR_PITCH       (1U << 21)  /* pitch = 4 bytes     */
#define IMX23_ICOLL_STAT_VECTOR_NUMBER      0x7FU
#define IMX23_ICOLL_INTR_ENABLE             (1U << 2)
#define IMX23_ICOLL_INTR_SOFTIRQ            (1U << 3)
#define IMX23_ICOLL_INTR_ENFIQ              (1U << 4)
#define IMX23_ICOLL_INTR_PRIORITY           0x3U

#define IMX23_ICOLL_NUM_IRQS    128

/*===========================================================================*/
/* IRQ numbers (ICOLL sources), from the i.MX23 DT interrupt parent map.     */
/*===========================================================================*/

enum imx23_irq_e {
  IMX23_IRQ_DUART       = 0,    /* PL011 debug UART                        */
  IMX23_IRQ_SSP1        = 2,
  IMX23_IRQ_AUDIOOUT1   = 4,
  IMX23_IRQ_EMI1        = 5,
  IMX23_IRQ_AUDIOOUT0   = 6,
  IMX23_IRQ_EMI0        = 7,
  IMX23_IRQ_AUDIOIN     = 8,
  IMX23_IRQ_EMI3        = 9,
  IMX23_IRQ_USB0        = 11,   /* USB OTG controller                      */
  IMX23_IRQ_SSP0        = 15,
  IMX23_IRQ_GPIO0       = 16,
  IMX23_IRQ_GPIO1       = 17,
  IMX23_IRQ_GPIO2       = 18,
  IMX23_IRQ_RTC         = 22,
  IMX23_IRQ_AUART0      = 24,
  IMX23_IRQ_EMI2        = 26,
  IMX23_IRQ_I2C0        = 27,
  IMX23_IRQ_TIMROT0     = 28,
  IMX23_IRQ_TIMROT1     = 29,
  IMX23_IRQ_TIMROT2     = 30,
  IMX23_IRQ_TIMROT3     = 31,
  IMX23_IRQ_POWER       = 33,   /* VDDD brownout etc.                      */
  IMX23_IRQ_LRADC0      = 36,
  IMX23_IRQ_LRADC1      = 37,
  IMX23_IRQ_LRADC2      = 38,
  IMX23_IRQ_LRADC3      = 39,
  IMX23_IRQ_LRADC4      = 40,
  IMX23_IRQ_LCDIF1      = 45,   /* LCDIF error                             */
  IMX23_IRQ_LCDIF0      = 46,   /* LCDIF VSYNC                             */
  IMX23_IRQ_DCP0        = 53,
  IMX23_IRQ_DCP1        = 54,
  IMX23_IRQ_GPMI        = 56,   /* NAND                                    */
  IMX23_IRQ_PXP         = 57,   /* pixel pipeline                          */
  IMX23_IRQ_AUART1      = 59
};

/*===========================================================================*/
/* TIMROT registers (timers 0..3 on the i.MX23).                              */
/*===========================================================================*/

#define IMX23_TIMROT_ROTCTRL        (IMX23_TIMROT_BASE + 0x000U)
#define IMX23_TIMROT_ROTCOUNT       (IMX23_TIMROT_BASE + 0x004U)
#define IMX23_TIMROT_TIMCTRL(n)     (IMX23_TIMROT_BASE + 0x020U + (n) * 0x40U)
#define IMX23_TIMROT_TIMCOUNT(n)    (IMX23_TIMROT_BASE + 0x024U + (n) * 0x40U)
/* v1 layout: running count in bits 31:16, fixed/reload count in bits 15:0. */
#define IMX23_TIMCOUNT_RUNNING(v)   (((v) >> 16) & 0xFFFFU)
#define IMX23_TIMCOUNT_FIXED(v)     ((v) & 0xFFFFU)
#define IMX23_TIMROT_VERSION        (IMX23_TIMROT_BASE + 0x0E0U)

#define IMX23_TIMROT_ROTCTRL_SFTRST     (1U << 31)
#define IMX23_TIMROT_ROTCTRL_CLKGATE    (1U << 30)

#define IMX23_TIMCTRL_IRQ           (1U << 15)  /* set at zero, write 0 to clear */
#define IMX23_TIMCTRL_IRQ_EN        (1U << 14)
#define IMX23_TIMCTRL_UPDATE        (1U << 7)
#define IMX23_TIMCTRL_RELOAD        (1U << 6)
#define IMX23_TIMCTRL_PRESCALE_DIV_BY_1 (0U << 4)
#define IMX23_TIMCTRL_PRESCALE_DIV_BY_2 (1U << 4)
#define IMX23_TIMCTRL_PRESCALE_DIV_BY_4 (2U << 4)
#define IMX23_TIMCTRL_PRESCALE_DIV_BY_8 (3U << 4)
#define IMX23_TIMCTRL_SELECT_32KHZ_XTAL     0x8U   /* 32kHz crystal (as Linux) */
#define IMX23_TIMCTRL_SELECT_TICK_ALWAYS    0xCU   /* always tick               */

/* On the i.MX23 (TIMROT v1) TIMCOUNTn is { running[31:16], fixed[15:0] }.
   We run the ST timers from the 32kHz crystal, exactly like Linux. */
#define IMX23_TIMROT_TICK_HZ        32768U

/*===========================================================================*/
/* PINCTRL registers.                                                         */
/*===========================================================================*/

#define IMX23_PINCTRL_CTRL         (IMX23_PINCTRL_BASE + 0x000U)
/* MUXSEL: 2 bits per pin, 16 pins per register, 2 registers per bank. */
#define IMX23_PINCTRL_MUXSEL(bank, pin)                                     \
  (IMX23_PINCTRL_BASE + 0x100U + ((bank) * 2U + (pin) / 16U) * 0x10U)
#define IMX23_PINCTRL_MUXSEL_SHIFT(pin) (((pin) & 15U) * 2U)
#define IMX23_PINCTRL_MUXSEL_GPIO   3U      /* GPIO is function 3 on every pad */
/* DRIVE: 4 bits per pin, 8 pins per register, 4 registers per bank. */
#define IMX23_PINCTRL_DRIVE(bank, pin)                                      \
  (IMX23_PINCTRL_BASE + 0x200U + ((bank) * 4U + (pin) / 8U) * 0x10U)
#define IMX23_PINCTRL_DRIVE_SHIFT(pin)  (((pin) & 7U) * 4U)
/* DOUT/DIN/DOE/STAT: 1 bit per pin, 1 register per bank. SET/CLR/TOG at +4/+8/+12. */
#define IMX23_PINCTRL_DOUT(bank)   (IMX23_PINCTRL_BASE + 0x500U + (bank) * 0x10U)
#define IMX23_PINCTRL_DIN(bank)    (IMX23_PINCTRL_BASE + 0x600U + (bank) * 0x10U)
#define IMX23_PINCTRL_DOE(bank)    (IMX23_PINCTRL_BASE + 0x700U + (bank) * 0x10U)
#define IMX23_PINCTRL_STAT(bank)   (IMX23_PINCTRL_BASE + 0x800U + (bank) * 0x10U)

/* Pad configuration flags (as in U-Boot mxs iomux), packed in the upper bits
   of an iomux_cfg_t word: bit layout compatible with MXS_PAD_* flags. */
typedef uint32_t imx23_iomux_t;
#define IMX23_PAD_BANK_SHIFT    13
#define IMX23_PAD_PIN_SHIFT     8
#define IMX23_PAD_MUXSEL_SHIFT  8          /* not used, see below */
#define IMX23_PAD_FUNC_SHIFT    8
#define IMX23_PAD_FUNC_MASK     (0x3U << 8)
#define IMX23_PAD_FUNC_GPIO     (3U << 8)

#define IMX23_PAD(bank, pin, func)                                              \
  ((imx23_iomux_t)(((bank) << 13) | ((pin) << 8) | ((func) & IMX23_PAD_FUNC_MASK)))
#define IMX23_PAD_FUNC(pad)     ((pad) & IMX23_PAD_FUNC_MASK)
#define IMX23_PAD_BANK(pad)     (((pad) >> 13) & 0x7U)
#define IMX23_PAD_PIN(pad)      (((pad) >> 8) & 0x1FU)

/*===========================================================================*/
/* CLKCTRL registers (subset).                                               */
/*===========================================================================*/

#define IMX23_CLKCTRL_PLL0CTRL0 (IMX23_CLKCTRL_BASE + 0x000U)
#define IMX23_CLKCTRL_CPU       (IMX23_CLKCTRL_BASE + 0x020U)
#define IMX23_CLKCTRL_HBUS      (IMX23_CLKCTRL_BASE + 0x040U)
#define IMX23_CLKCTRL_XTAL      (IMX23_CLKCTRL_BASE + 0x050U)
#define IMX23_CLKCTRL_CLKSEQ    (IMX23_CLKCTRL_BASE + 0x110U)

#define IMX23_CLKCTRL_XTAL_UART_CLK_GATE    (1U << 31)  /* clear = UART clock on */
#define IMX23_CLKCTRL_XTAL_DIV_UART         0x3U        /* 1..3 -> /1../3       */

/*===========================================================================*/
/* PL011 (DUART) registers.                                                  */
/*===========================================================================*/

typedef struct {
  volatile uint32_t DR;         /* 0x00 data (RX read / TX write)          */
  volatile uint32_t RSR_ECR;    /* 0x04                                     */
  uint32_t          _rsvd[4];
  volatile uint32_t FR;         /* 0x18 flag                                */
  uint32_t          _rsvd1[2];
  volatile uint32_t ILPR;       /* 0x20                                     */
  volatile uint32_t IBRD;       /* 0x24 integer baud rate divisor           */
  volatile uint32_t FBRD;       /* 0x28 fractional baud rate divisor        */
  volatile uint32_t LCR_H;      /* 0x2C line control                        */
  volatile uint32_t CR;         /* 0x30 control                             */
  volatile uint32_t IFLS;       /* 0x34 interrupt FIFO level select         */
  volatile uint32_t IMSC;       /* 0x38 interrupt mask set/clear            */
  volatile uint32_t RIS;        /* 0x3C raw interrupt status                */
  volatile uint32_t MIS;        /* 0x40 masked interrupt status             */
  volatile uint32_t ICR;        /* 0x44 interrupt clear                     */
} imx23_pl011_t;

#define IMX23_PL011              ((imx23_pl011_t *)IMX23_DUART_BASE)

#define IMX23_PL011_FR_CTS      (1U << 0)
#define IMX23_PL011_FR_BUSY     (1U << 3)
#define IMX23_PL011_FR_RXFE     (1U << 4)   /* RX FIFO empty  */
#define IMX23_PL011_FR_TXFF     (1U << 5)   /* TX FIFO full   */
#define IMX23_PL011_FR_RXFF     (1U << 6)   /* RX FIFO full   */
#define IMX23_PL011_FR_TXFE     (1U << 7)   /* TX FIFO empty  */

#define IMX23_PL011_RIS_FE      (1U << 0)   /* framing error  */
#define IMX23_PL011_RIS_PE      (1U << 1)   /* parity error   */
#define IMX23_PL011_RIS_BE      (1U << 2)   /* break error    */
#define IMX23_PL011_RIS_OE      (1U << 3)   /* overrun error  */
#define IMX23_PL011_RIS_RX      (1U << 4)   /* RX interrupt   */
#define IMX23_PL011_RIS_TX      (1U << 5)   /* TX interrupt   */
#define IMX23_PL011_RIS_RT      (1U << 6)   /* RX timeout     */
#define IMX23_PL011_RIS_ERRORS  (IMX23_PL011_RIS_FE | IMX23_PL011_RIS_PE | \
                                 IMX23_PL011_RIS_BE | IMX23_PL011_RIS_OE)

#define IMX23_PL011_CR_UARTEN   (1U << 0)
#define IMX23_PL011_CR_TXE      (1U << 8)
#define IMX23_PL011_CR_RXE      (1U << 9)

#define IMX23_PL011_LCR_H_SPS   (1U << 0)
#define IMX23_PL011_LCR_H_PEN   (1U << 1)
#define IMX23_PL011_LCR_H_EPS   (1U << 2)
#define IMX23_PL011_LCR_H_STP2  (1U << 3)
#define IMX23_PL011_LCR_H_FEN   (1U << 4)
#define IMX23_PL011_LCR_H_WLEN_8 (3U << 5)

/* The DUART is clocked from the 24MHz XTAL (UART clock gate in CLKCTRL_XTAL). */
#define IMX23_DUART_CLK_HZ      24000000U

/*===========================================================================*/
/* Generic register access helpers (mxs SET/CLR/TOG at +4/+8/+12).           */
/*===========================================================================*/

#define IMX23_REG(addr)         (*(volatile uint32_t *)(uintptr_t)(addr))
#define IMX23_REG_SET(addr, v)  IMX23_REG((addr) + 4U)  = (uint32_t)(v)
#define IMX23_REG_CLR(addr, v)  IMX23_REG((addr) + 8U)  = (uint32_t)(v)
#define IMX23_REG_TOG(addr, v)  IMX23_REG((addr) + 12U) = (uint32_t)(v)

#endif /* IMX23_H */

/** @} */
