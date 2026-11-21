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

/* Per-bank GPIO interrupt registers (bank 0..2). */
#define IMX23_PINCTRL_IRQEN(bank)    (IMX23_PINCTRL_BASE + 0x900U + (bank) * 0x10U)
#define IMX23_PINCTRL_IRQLEVEL(bank) (IMX23_PINCTRL_BASE + 0xA00U + (bank) * 0x10U)
#define IMX23_PINCTRL_IRQPOL(bank)   (IMX23_PINCTRL_BASE + 0xB00U + (bank) * 0x10U)
#define IMX23_PINCTRL_IRQSTAT(bank)  (IMX23_PINCTRL_BASE + 0xC00U + (bank) * 0x10U)
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
#define IMX23_CLKCTRL_HBUS      (IMX23_CLKCTRL_BASE + 0x030U)
#define IMX23_CLKCTRL_XBUS      (IMX23_CLKCTRL_BASE + 0x040U)
#define IMX23_CLKCTRL_XTAL      (IMX23_CLKCTRL_BASE + 0x050U)
#define IMX23_CLKCTRL_CLKSEQ    (IMX23_CLKCTRL_BASE + 0x110U)

/* HW_CLKCTRL_PLL0CTRL0 bits. */
#define IMX23_CLKCTRL_PLL0CTRL0_EN_USB_CLKS (1U << 18)   /* usb_phy 480MHz gate */

/* HW_CLKCTRL_CPU bits. */
#define IMX23_CLKCTRL_CPU_BUSY_REF_XTAL     (1U << 29)
#define IMX23_CLKCTRL_CPU_BUSY_REF_CPU      (1U << 28)
#define IMX23_CLKCTRL_CPU_DIV_XTAL_MASK     (0x3FFU << 16)
#define IMX23_CLKCTRL_CPU_DIV_CPU_MASK      0x3FU

/* HW_CLKCTRL_HBUS bits. */
#define IMX23_CLKCTRL_HBUS_DIV_MASK         0x1FU

/* HW_CLKCTRL_CLKSEQ bypass bits. */
#define IMX23_CLKCTRL_CLKSEQ_BYPASS_CPU     (1U << 7)
#define IMX23_CLKCTRL_CLKSEQ_BYPASS_EMI     (1U << 6)
#define IMX23_CLKCTRL_CLKSEQ_BYPASS_GPMI    (1U << 4)

/* HW_CLKCTRL_GPMI (CLKCTRL + 0x80): GPMI NAND clock divider. */
#define IMX23_CLKCTRL_GPMI        (IMX23_CLKCTRL_BASE + 0x80U)
#define IMX23_CLKCTRL_GPMI_CLKGATE          (1U << 31)
#define IMX23_CLKCTRL_GPMI_BUSY             (1U << 29)
#define IMX23_CLKCTRL_GPMI_DIV_MASK         0x3FFU

/* PLL0 runs at a fixed 480MHz from the 24MHz XTAL on i.MX23. */
#define IMX23_PLL0_HZ               480000000U

/* HW_POWER_VDDDCTRL (0x40): TRG bits 4:0, 25mV steps from 0.8V. */
#define IMX23_POWER_VDDDCTRL        (IMX23_POWER_BASE + 0x040U)
#define IMX23_POWER_VDDDCTRL_TRG_MASK   0x1FU
#define IMX23_POWER_VDDD_STEP_MV    25U
#define IMX23_POWER_VDDD_BASE_MV    800U

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

/** @} */

/*===========================================================================*/
/* SSP (Sync Serial Port, SPI mode) registers - i.MX23 "old" layout.         */
/*===========================================================================*/

typedef struct {
  volatile uint32_t CTRL0;        /* 0x00 */
  volatile uint32_t CMD0;         /* 0x10 */
  volatile uint32_t CMD1;         /* 0x20 */
  volatile uint32_t COMPREF;      /* 0x30 */
  volatile uint32_t COMPMASK;     /* 0x40 */
  volatile uint32_t TIMING;       /* 0x50 */
  volatile uint32_t CTRL1;        /* 0x60 */
  volatile uint32_t DATA;         /* 0x70 */
  volatile uint32_t SDRESP0;      /* 0x80 */
  volatile uint32_t SDRESP1;      /* 0x90 */
  volatile uint32_t SDRESP2;      /* 0xA0 */
  volatile uint32_t SDRESP3;      /* 0xB0 */
  volatile uint32_t STATUS;       /* 0xC0 */
} imx23_ssp_t;

#define IMX23_SSP0              ((imx23_ssp_t *)IMX23_SSP0_BASE)
#define IMX23_SSP1              ((imx23_ssp_t *)IMX23_SSP1_BASE)

#define IMX23_SSP_CTRL0_SFTRST          (1U << 31)
#define IMX23_SSP_CTRL0_CLKGATE         (1U << 30)
#define IMX23_SSP_CTRL0_RUN             (1U << 29)
#define IMX23_SSP_CTRL0_IGNORE_CRC      (1U << 26)
#define IMX23_SSP_CTRL0_READ            (1U << 25)
#define IMX23_SSP_CTRL0_DATA_XFER       (1U << 24)
#define IMX23_SSP_CTRL0_BUS_WIDTH_8BIT  (2U << 22)
#define IMX23_SSP_CTRL0_ENABLE          (1U << 16)
#define IMX23_SSP_CTRL0_XFER_COUNT_MASK 0xFFFFU

#define IMX23_SSP_CTRL1_DMA_ENABLE      (1U << 13)
#define IMX23_SSP_CTRL1_PHASE           (1U << 10)  /* CPHA */
#define IMX23_SSP_CTRL1_POLARITY        (1U << 9)   /* CPOL */
#define IMX23_SSP_CTRL1_WORD_LENGTH_8   (7U << 4)
#define IMX23_SSP_CTRL1_SSP_MODE_SPI    0x0U

#define IMX23_SSP_TIMING_TIMEOUT_MASK   (0xFFFFU << 16)
#define IMX23_SSP_TIMING_CLOCK_DIVIDE   (0xFFU << 8)
#define IMX23_SSP_TIMING_CLOCK_RATE     0xFFU

#define IMX23_SSP_STATUS_CMD_BUSY       (1U << 0)
#define IMX23_SSP_STATUS_DATA_BUSY      (1U << 1)
#define IMX23_SSP_STATUS_FIFO_FULL      (1U << 8)
#define IMX23_SSP_STATUS_FIFO_EMPTY     (1U << 5)

/* CLKCTRL SSP clock control (0x70): gate bit31, divider bits 8:0. */
#define IMX23_CLKCTRL_SSP0      (IMX23_CLKCTRL_BASE + 0x070U)
#define IMX23_CLKCTRL_SSP_CLKGATE   (1U << 31)
#define IMX23_CLKCTRL_SSP_DIV_MASK  0x1FFU
/* SSP clock select mux in CLKSEQ bit 5: 0 = ref_io, 1 = ref_xtal (24MHz). */
#define IMX23_CLKCTRL_CLKSEQ_BYPASS_SSP0  (1U << 5)
#define IMX23_CLKCTRL_CLKSEQ_SSP_SEL_XTAL (1U << 5)

/* SSP clock: 24MHz XTAL / divider (default 1). */
#if !defined(IMX23_SSP_CLK_HZ) || defined(__DOXYGEN__)
#define IMX23_SSP_CLK_HZ        24000000U
#endif

/*===========================================================================*/
/* APBH/APBX DMA engines.                                                     */
/*===========================================================================*/

#define IMX23_DMA_APBH          0U      /* LCDIF, SSP, NAND... */
#define IMX23_DMA_APBX          1U      /* audio, PXP, AUART... */

#define IMX23_DMA_APBH_CHANNELS 8U
#define IMX23_DMA_APBX_CHANNELS 16U

/* Engine registers. */
#define IMX23_APBH_CTRL0        (IMX23_APBH_BASE + 0x000U)
#define IMX23_APBH_CTRL1        (IMX23_APBH_BASE + 0x010U)
#define IMX23_APBH_CTRL2        (IMX23_APBH_BASE + 0x020U)
#define IMX23_APBX_CTRL0        (IMX23_APBX_BASE + 0x000U)
#define IMX23_APBX_CTRL1        (IMX23_APBX_BASE + 0x010U)
#define IMX23_APBX_CTRL2        (IMX23_APBX_BASE + 0x020U)

#define IMX23_DMA_CTRL0_SFTRST          (1U << 31)
#define IMX23_DMA_CTRL0_CLKGATE         (1U << 30)
#define IMX23_DMA_CTRL0_RESET_CHANNEL(n) (1U << (16U + (n)))

/* Per-channel registers: APBH channel block at 0x40 + n*0x70,
   APBX channel block at 0x100 + n*0x70. */
#define IMX23_DMA_CH_BASE(bus, n)                                               \
  (((bus) == IMX23_DMA_APBH ? IMX23_APBH_BASE : IMX23_APBX_BASE) +             \
   (((bus) == IMX23_DMA_APBH ? 0x40U : 0x100U) + (n) * 0x70U))
#define IMX23_DMA_CH_CURCMDAR(bus, n)   (IMX23_DMA_CH_BASE(bus, n) + 0x00U)
#define IMX23_DMA_CH_NXTCMDAR(bus, n)   (IMX23_DMA_CH_BASE(bus, n) + 0x10U)
#define IMX23_DMA_CH_CMD(bus, n)        (IMX23_DMA_CH_BASE(bus, n) + 0x20U)
#define IMX23_DMA_CH_BAR(bus, n)        (IMX23_DMA_CH_BASE(bus, n) + 0x30U)
#define IMX23_DMA_CH_SEMA(bus, n)       (IMX23_DMA_CH_BASE(bus, n) + 0x40U)

/* Channel command word (CCW) bits. */
#define IMX23_CCW_COMMAND_NO_XFER   0x0U
#define IMX23_CCW_COMMAND_WRITE     0x1U    /* peripheral -> memory (RM DMA_WRITE) */
#define IMX23_CCW_COMMAND_READ      0x2U    /* memory -> peripheral (RM DMA_READ)  */
#define IMX23_CCW_CHAIN             (1U << 2)
#define IMX23_CCW_IRQ               (1U << 3)
#define IMX23_CCW_WAIT4READY        (1U << 5)    /* NAND: wait for RDY before next */
#define IMX23_CCW_SEMAPHORE         (1U << 6)
#define IMX23_CCW_WAIT4END          (1U << 7)
#define IMX23_CCW_HALTONTERMINATE   (1U << 8)
#define IMX23_CCW_PIO_NUM(n)        (((n) & 0xFU) << 12)

/* DMA descriptor (CCW) in memory, 16-byte aligned, in SDRAM. */
typedef struct {
  volatile uint32_t next;             /* phys addr of next CCW, 0 = last   */
  volatile uint16_t bits;             /* CCW bits                          */
  volatile uint16_t xfer;             /* transfer byte count (<= 0xFF00)   */
  volatile uint32_t bufaddr;          /* memory buffer address             */
  volatile uint32_t pio[16];          /* PIO words written to the periph.  */
} imx23_dma_ccw_t;

/*===========================================================================*/
/* I2C (I2C0 @ 0x80058000).                                                   */
/*===========================================================================*/

typedef struct {
  volatile uint32_t CTRL0;        /* 0x00 */
  volatile uint32_t TIMING0;      /* 0x10 */
  volatile uint32_t TIMING1;      /* 0x20 */
  volatile uint32_t TIMING2;      /* 0x30 */
  volatile uint32_t CTRL1;        /* 0x40 */
  volatile uint32_t STAT;         /* 0x50 */
  volatile uint32_t QUEUECTRL;    /* 0x60 */
  volatile uint32_t QUEUESTAT;    /* 0x70 */
  volatile uint32_t QUEUECMD;     /* 0x80 */
  volatile uint32_t QUEUEDATA;    /* 0x90 */
  volatile uint32_t DATA;         /* 0xA0 */
  volatile uint32_t DEBUG0;       /* 0xB0 */
  volatile uint32_t DEBUG1;       /* 0xC0 */
  volatile uint32_t VERSION;      /* 0xD0 */
} imx23_i2c_t;

#define IMX23_I2C0              ((imx23_i2c_t *)IMX23_I2C0_BASE)

#define IMX23_I2C_CTRL0_SFTRST          (1U << 31)
#define IMX23_I2C_CTRL0_CLKGATE         (1U << 30)
#define IMX23_I2C_CTRL0_RUN             (1U << 29)
#define IMX23_I2C_CTRL0_SEND_NAK_ON_LAST (1U << 25)
#define IMX23_I2C_CTRL0_PIO_MODE        (1U << 24)
#define IMX23_I2C_CTRL0_RETAIN_CLOCK    (1U << 21)
#define IMX23_I2C_CTRL0_POST_SEND_STOP  (1U << 20)
#define IMX23_I2C_CTRL0_PRE_SEND_START  (1U << 19)
#define IMX23_I2C_CTRL0_MASTER_MODE     (1U << 17)
#define IMX23_I2C_CTRL0_DIRECTION       (1U << 16)  /* 1 = master receive */
#define IMX23_I2C_CTRL0_XFER_COUNT_MASK 0xFFFFU

#define IMX23_I2C_CTRL1_SLAVE_ADDR_BYTE(addr_rw) ((uint32_t)(addr_rw) << 16)
#define IMX23_I2C_CTRL1_NO_SLAVE_ACK_IRQ (1U << 29)
#define IMX23_I2C_CTRL1_DATA_ENGINE_CMPLT_IRQ (1U << 14)

#define IMX23_I2C_STAT_GOT_A_NAK        (1U << 28)
#define IMX23_I2C_STAT_BUS_BUSY         (1U << 11)
#define IMX23_I2C_STAT_CLK_GEN_BUSY     (1U << 10)
#define IMX23_I2C_STAT_DATA_ENGINE_BUSY (1U << 9)

/* I2C bus clock is the 24MHz XTAL. */
#define IMX23_I2C_CLK_HZ        24000000U

/*===========================================================================*/
/* AUART (application UARTs 0/1).                                            */
/*===========================================================================*/

typedef struct {
  volatile uint32_t CTRL0;        /* 0x00 */
  volatile uint32_t CTRL1;        /* 0x10 */
  volatile uint32_t CTRL2;        /* 0x20 */
  volatile uint32_t LINECTRL;     /* 0x30 */
  volatile uint32_t LINECTRL2;    /* 0x40 */
  volatile uint32_t INTR;         /* 0x50 */
  volatile uint32_t DATA;         /* 0x60 */
  volatile uint32_t STAT;         /* 0x70 */
  volatile uint32_t DEBUG0;       /* 0x80 */
  volatile uint32_t VERSION;      /* 0xB0 */
} imx23_auart_t;

#define IMX23_AUART0            ((imx23_auart_t *)IMX23_AUART0_BASE)
#define IMX23_AUART1            ((imx23_auart_t *)IMX23_AUART1_BASE)

#define IMX23_AUART_CTRL0_SFTRST        (1U << 31)
#define IMX23_AUART_CTRL0_CLKGATE       (1U << 30)

#define IMX23_AUART_CTRL2_UARTEN        (1U << 0)
#define IMX23_AUART_CTRL2_TXE           (1U << 8)
#define IMX23_AUART_CTRL2_RXE           (1U << 9)

#define IMX23_AUART_LINECTRL_BAUD_DIVINT(v)  (((v) & 0xFFFFU) << 16)
#define IMX23_AUART_LINECTRL_BAUD_DIVFRAC(v) (((v) & 0x3FU) << 8)
#define IMX23_AUART_LINECTRL_WLEN(v)    ((((v) - 5U) & 0x3U) << 5)
#define IMX23_AUART_LINECTRL_FEN        (1U << 4)
#define IMX23_AUART_LINECTRL_STP2       (1U << 3)
#define IMX23_AUART_LINECTRL_EPS        (1U << 2)
#define IMX23_AUART_LINECTRL_PEN        (1U << 1)

#define IMX23_AUART_INTR_RXIEN          (1U << 20)
#define IMX23_AUART_INTR_TXIEN          (1U << 21)
#define IMX23_AUART_INTR_RTIEN          (1U << 22)
#define IMX23_AUART_INTR_RXIS           (1U << 4)
#define IMX23_AUART_INTR_TXIS           (1U << 5)
#define IMX23_AUART_INTR_RTIS           (1U << 6)
#define IMX23_AUART_INTR_ERRORS         (IMX23_AUART_INTR_RTIS | 0x70000U)

#define IMX23_AUART_STAT_BUSY           (1U << 29)
#define IMX23_AUART_STAT_TXFE           (1U << 27)
#define IMX23_AUART_STAT_TXFF           (1U << 25)
#define IMX23_AUART_STAT_RXFE           (1U << 24)
#define IMX23_AUART_STAT_OERR           (1U << 19)
#define IMX23_AUART_STAT_BERR           (1U << 18)
#define IMX23_AUART_STAT_PERR           (1U << 17)
#define IMX23_AUART_STAT_FERR           (1U << 16)
#define IMX23_AUART_STAT_ERRORS         (IMX23_AUART_STAT_OERR | IMX23_AUART_STAT_BERR | \
                                         IMX23_AUART_STAT_PERR | IMX23_AUART_STAT_FERR)

/* AUART frame configs (LINECTRL values). */
#define IMX23_SIO_AUART_8N1     (IMX23_AUART_LINECTRL_WLEN(8) | IMX23_AUART_LINECTRL_FEN)
#define IMX23_SIO_AUART_8E1     (IMX23_SIO_AUART_8N1 | IMX23_AUART_LINECTRL_PEN | IMX23_AUART_LINECTRL_EPS)
#define IMX23_SIO_AUART_8O1     (IMX23_SIO_AUART_8N1 | IMX23_AUART_LINECTRL_PEN)

/*===========================================================================*/
/* PWM block (5 channels, fixed 24MHz clock CLK_PWM24M).                     */
/*===========================================================================*/

typedef struct {
  volatile uint32_t CTRL;        /* 0x00, SET 0x04, CLR 0x08, TOG 0x0C       */
  volatile uint32_t CTRL_SET;
  volatile uint32_t CTRL_CLR;
  volatile uint32_t CTRL_TOG;
  volatile uint32_t CH[5][8];    /* CH[n][0]=ACTIVE @0x10+n*0x20 (INACTIVE[31:16]
                                    |ACTIVE[15:0]), CH[n][4]=PERIOD @0x20+n*0x20 */
} imx23_pwm_t;

#define IMX23_PWM               ((imx23_pwm_t *)IMX23_PWM_BASE)
#define IMX23_PWM_ACTIVE(pwm,n)    ((pwm)->CH[n][0])
#define IMX23_PWM_PERIODREG(pwm,n) ((pwm)->CH[n][4])

#define IMX23_PWM_CTRL_SFTRST           (1U << 31)
#define IMX23_PWM_CTRL_CLKGATE          (1U << 30)
#define IMX23_PWM_CTRL_PWM_EN(n)        (1U << (n))

#define IMX23_PWM_PERIOD_PERIOD(p)      ((uint32_t)(p) & 0xFFFFU)
#define IMX23_PWM_PERIOD_ACTIVE_HIGH    (3U << 16)
#define IMX23_PWM_PERIOD_ACTIVE_LOW     (2U << 16)
#define IMX23_PWM_PERIOD_INACTIVE_HIGH  (3U << 18)
#define IMX23_PWM_PERIOD_INACTIVE_LOW   (2U << 18)
#define IMX23_PWM_PERIOD_POLARITY_NORMAL   (IMX23_PWM_PERIOD_ACTIVE_HIGH | IMX23_PWM_PERIOD_INACTIVE_LOW)
#define IMX23_PWM_PERIOD_POLARITY_INVERSE  (IMX23_PWM_PERIOD_ACTIVE_LOW | IMX23_PWM_PERIOD_INACTIVE_HIGH)
#define IMX23_PWM_PERIOD_CDIV(div)      (((uint32_t)(div) & 0x7U) << 20)

/* Divider shifts for CDIV 0..7 (24MHz / 2^shift). */
extern const uint8_t imx23_pwm_cdiv_shift[8];

/* Ungate the fixed 24MHz PWM clock (CLKCTRL_XTAL bit 29, 0 = on). */
#define IMX23_CLKCTRL_XTAL_PWM_CLK24M_GATE  (1U << 29)

#define IMX23_PWM_CHANNELS      5
#define IMX23_PWM_CLK_HZ        24000000U

/*===========================================================================*/
/* LCDIF (MX23 register layout, no CTRL2).                                   */
/*===========================================================================*/

typedef struct {
  volatile uint32_t CTRL;         /* 0x00 SET 0x04 CLR 0x08 TOG 0x0C        */
  volatile uint32_t CTRL_SET;
  volatile uint32_t CTRL_CLR;
  volatile uint32_t CTRL_TOG;
  volatile uint32_t CTRL1;        /* 0x10 SET 0x14 CLR 0x18 TOG 0x1C        */
  volatile uint32_t CTRL1_SET;
  volatile uint32_t CTRL1_CLR;
  volatile uint32_t CTRL1_TOG;
  volatile uint32_t TRANSFER_COUNT; /* 0x20: V_COUNT[31:16] H_COUNT[15:0]   */
  volatile uint32_t TRANSFER_COUNT_SET;
  volatile uint32_t TRANSFER_COUNT_CLR;
  volatile uint32_t TRANSFER_COUNT_TOG;
  volatile uint32_t CUR_BUF;      /* 0x30                                   */
  volatile uint32_t CUR_BUF_SET;
  volatile uint32_t CUR_BUF_CLR;
  volatile uint32_t CUR_BUF_TOG;
  volatile uint32_t NEXT_BUF;     /* 0x40                                   */
  volatile uint32_t NEXT_BUF_SET;
  volatile uint32_t NEXT_BUF_CLR;
  volatile uint32_t NEXT_BUF_TOG;
  volatile uint32_t RESERVED0[4]; /* 0x50-0x5C                              */
  volatile uint32_t TIMING;       /* 0x60                                   */
  volatile uint32_t TIMING_SET;
  volatile uint32_t TIMING_CLR;
  volatile uint32_t TIMING_TOG;
  volatile uint32_t VDCTRL0;      /* 0x70                                   */
  volatile uint32_t VDCTRL0_SET;
  volatile uint32_t VDCTRL0_CLR;
  volatile uint32_t VDCTRL0_TOG;
  volatile uint32_t VDCTRL1;      /* 0x80: VSYNC_PERIOD                     */
  volatile uint32_t VDCTRL1_SET;
  volatile uint32_t VDCTRL1_CLR;
  volatile uint32_t VDCTRL1_TOG;
  volatile uint32_t VDCTRL2;      /* 0x90: HSYNC_PULSE_WIDTH[31:24] PERIOD  */
  volatile uint32_t VDCTRL2_SET;
  volatile uint32_t VDCTRL2_CLR;
  volatile uint32_t VDCTRL2_TOG;
  volatile uint32_t VDCTRL3;      /* 0xA0: H_WAIT[27:16] V_WAIT[15:0]       */
  volatile uint32_t VDCTRL3_SET;
  volatile uint32_t VDCTRL3_CLR;
  volatile uint32_t VDCTRL3_TOG;
  volatile uint32_t VDCTRL4;      /* 0xB0: DOTCLK_DLY[31:29] SYNC_ON bit18  */
  volatile uint32_t VDCTRL4_SET;  /*       DOTCLK_H_VALID_DATA_CNT[17:0]    */
  volatile uint32_t VDCTRL4_CLR;
  volatile uint32_t VDCTRL4_TOG;
  volatile uint32_t RESERVED1[0x1B0 / 4 - (0xC0 / 4)]; /* up to DATA  */
  volatile uint32_t DATA;         /* 0x1B0                                  */
  volatile uint32_t DATA_SET;
  volatile uint32_t DATA_CLR;
  volatile uint32_t DATA_TOG;
  volatile uint32_t BM_ERROR_STAT;/* 0x1C0                                  */
  volatile uint32_t BM_ERROR_STAT_SET;
  volatile uint32_t BM_ERROR_STAT_CLR;
  volatile uint32_t BM_ERROR_STAT_TOG;
  volatile uint32_t RESERVED2[4]; /* 0x1D0-0x1DC? STAT at 0x1D0           */
  volatile uint32_t STAT;         /* 0x1D0                                  */
  volatile uint32_t STAT_SET;
  volatile uint32_t STAT_CLR;
  volatile uint32_t STAT_TOG;
  volatile uint32_t VERSION;      /* 0x1E0                                  */
} imx23_lcdif_t;

#define IMX23_LCDIF             ((imx23_lcdif_t *)IMX23_LCDIF_BASE)

#define IMX23_LCDIF_CTRL_SFTRST             (1U << 31)
#define IMX23_LCDIF_CTRL_CLKGATE            (1U << 30)
#define IMX23_LCDIF_CTRL_DVI_MODE           (1U << 20)
#define IMX23_LCDIF_CTRL_BYPASS_COUNT       (1U << 19)
#define IMX23_LCDIF_CTRL_VSYNC_MODE         (1U << 18)
#define IMX23_LCDIF_CTRL_DOTCLK_MODE        (1U << 17)
#define IMX23_LCDIF_CTRL_DATABUS_WIDTH_16   (0U << 10)
#define IMX23_LCDIF_CTRL_DATABUS_WIDTH_8    (1U << 10)
#define IMX23_LCDIF_CTRL_DATABUS_WIDTH_18   (2U << 10)
#define IMX23_LCDIF_CTRL_DATABUS_WIDTH_24   (3U << 10)
#define IMX23_LCDIF_CTRL_WORD_LENGTH_16     (0U << 8)
#define IMX23_LCDIF_CTRL_WORD_LENGTH_8      (1U << 8)
#define IMX23_LCDIF_CTRL_WORD_LENGTH_18     (2U << 8)
#define IMX23_LCDIF_CTRL_WORD_LENGTH_24     (3U << 8)
#define IMX23_LCDIF_CTRL_LCDIF_MASTER       (1U << 5)
#define IMX23_LCDIF_CTRL_DATA_FORMAT_16     (1U << 3)
#define IMX23_LCDIF_CTRL_DATA_FORMAT_24     (1U << 1)
#define IMX23_LCDIF_CTRL_RUN                (1U << 0)

#define IMX23_LCDIF_CTRL1_RECOVER_ON_UNDERFLOW  (1U << 24)
#define IMX23_LCDIF_CTRL1_FIFO_CLEAR            (1U << 21)
#define IMX23_LCDIF_CTRL1_BYTE_PACKING(v)       (((v) & 0xFU) << 16)
#define IMX23_LCDIF_CTRL1_VSYNC_EDGE_IRQ_EN     (1U << 12)
#define IMX23_LCDIF_CTRL1_CUR_FRAME_DONE_IRQ_EN (1U << 13)
#define IMX23_LCDIF_CTRL1_VSYNC_EDGE_IRQ        (1U << 8)
#define IMX23_LCDIF_CTRL1_CUR_FRAME_DONE_IRQ    (1U << 9)

#define IMX23_LCDIF_VDCTRL0_ENABLE_PRESENT       (1U << 28)
#define IMX23_LCDIF_VDCTRL0_VSYNC_POL           (1U << 27)
#define IMX23_LCDIF_VDCTRL0_HSYNC_POL           (1U << 26)
#define IMX23_LCDIF_VDCTRL0_DOTCLK_POL          (1U << 25)
#define IMX23_LCDIF_VDCTRL0_ENABLE_POL          (1U << 24)
#define IMX23_LCDIF_VDCTRL0_VSYNC_PERIOD_UNIT   (1U << 21)
#define IMX23_LCDIF_VDCTRL0_VSYNC_PULSE_WIDTH_UNIT (1U << 20)
#define IMX23_LCDIF_VDCTRL0_VSYNC_PULSE_WIDTH(v) ((v) & 0x3FFFFU)

#define IMX23_LCDIF_VDCTRL2_HSYNC_PULSE_WIDTH(v) (((v) & 0xFFU) << 24)
#define IMX23_LCDIF_VDCTRL2_HSYNC_PERIOD(v)      ((v) & 0x3FFFFU)

#define IMX23_LCDIF_VDCTRL3_HORIZONTAL_WAIT_CNT(v) (((v) & 0xFFFU) << 16)
#define IMX23_LCDIF_VDCTRL3_VERTICAL_WAIT_CNT(v)   ((v) & 0xFFFFU)

#define IMX23_LCDIF_VDCTRL4_SYNC_SIGNALS_ON         (1U << 18)
#define IMX23_LCDIF_VDCTRL4_DOTCLK_H_VALID_DATA_CNT(v) ((v) & 0x3FFFFU)

/* CLKCTRL_PIX (0x60) and FRAC0 byte for the pixel clock. */
#define IMX23_CLKCTRL_PIX           (IMX23_CLKCTRL_BASE + 0x060U)
#define IMX23_CLKCTRL_PIX_DIV_MASK  0xFFFU
#define IMX23_CLKCTRL_PIX_DIV_FRAC_EN (1U << 12)
#define IMX23_CLKCTRL_PIX_BUSY      (1U << 29)
#define IMX23_CLKCTRL_PIX_CLKGATE   (1U << 31)
#define IMX23_CLKCTRL_FRAC0         (IMX23_CLKCTRL_BASE + 0x0F0U)
#define IMX23_CLKCTRL_FRAC0_PIX     2U
#define IMX23_CLKCTRL_FRAC_CLKGATE  (1U << 7)
#define IMX23_CLKCTRL_FRAC_MASK     0x3FU
#define IMX23_CLKCTRL_CLKSEQ_BYPASS_PIX (1U << 1)

/*===========================================================================*/
/* PXP (pixel pipeline, RTL v2 on i.MX23, S0 only, no overlay).             */
/*===========================================================================*/

#define IMX23_PXP_CTRL              (IMX23_PXP_BASE + 0x000U)
#define IMX23_PXP_CTRL_SET          (IMX23_PXP_BASE + 0x004U)
#define IMX23_PXP_CTRL_CLR          (IMX23_PXP_BASE + 0x008U)
#define IMX23_PXP_STAT              (IMX23_PXP_BASE + 0x010U)
#define IMX23_PXP_STAT_CLR          (IMX23_PXP_BASE + 0x018U)
#define IMX23_PXP_RGBBUF            (IMX23_PXP_BASE + 0x020U)
#define IMX23_PXP_RGBSIZE           (IMX23_PXP_BASE + 0x040U)
#define IMX23_PXP_S0BUF             (IMX23_PXP_BASE + 0x050U)
#define IMX23_PXP_S0UBUF            (IMX23_PXP_BASE + 0x060U)
#define IMX23_PXP_S0VBUF            (IMX23_PXP_BASE + 0x070U)
#define IMX23_PXP_S0PARAM           (IMX23_PXP_BASE + 0x080U)
#define IMX23_PXP_S0BACKGROUND      (IMX23_PXP_BASE + 0x090U)
#define IMX23_PXP_S0CROP            (IMX23_PXP_BASE + 0x0A0U)
#define IMX23_PXP_S0SCALE           (IMX23_PXP_BASE + 0x0B0U)
#define IMX23_PXP_S0OFFSET          (IMX23_PXP_BASE + 0x0C0U)
#define IMX23_PXP_CSCCOEFF0         (IMX23_PXP_BASE + 0x0D0U)
#define IMX23_PXP_CSCCOEFF1         (IMX23_PXP_BASE + 0x0E0U)
#define IMX23_PXP_CSCCOEFF2         (IMX23_PXP_BASE + 0x0F0U)
#define IMX23_PXP_S0COLORKEYLOW     (IMX23_PXP_BASE + 0x180U)
#define IMX23_PXP_S0COLORKEYHIGH    (IMX23_PXP_BASE + 0x190U)
#define IMX23_PXP_VERSION           (IMX23_PXP_BASE + 0x1F0U)

#define IMX23_PXP_CTRL_SFTRST           (1U << 31)
#define IMX23_PXP_CTRL_CLKGATE          (1U << 30)
#define IMX23_PXP_CTRL_ALPHA_OUTPUT     (1U << 22)
#define IMX23_PXP_CTRL_CROP             (1U << 19)
#define IMX23_PXP_CTRL_SCALE            (1U << 18)
#define IMX23_PXP_CTRL_S0_FORMAT(v)     (((v) & 0xFU) << 12)
#define IMX23_PXP_CTRL_VFLIP            (1U << 11)
#define IMX23_PXP_CTRL_HFLIP            (1U << 10)
#define IMX23_PXP_CTRL_ROTATE(v)        (((v) & 0x3U) << 8)
#define IMX23_PXP_CTRL_OUT_FMT(v)       (((v) & 0xFU) << 4)
#define IMX23_PXP_CTRL_IRQ_ENABLE       (1U << 1)
#define IMX23_PXP_CTRL_ENABLE           (1U << 0)

#define IMX23_PXP_S0_FMT_RGB888         0x1U
#define IMX23_PXP_S0_FMT_RGB565         0x4U
#define IMX23_PXP_S0_FMT_YUV422         0x8U
#define IMX23_PXP_S0_FMT_YUV420         0x9U

#define IMX23_PXP_OUT_FMT_ARGB8888      0x0U
#define IMX23_PXP_OUT_FMT_RGB888        0x1U
#define IMX23_PXP_OUT_FMT_RGB565        0x4U

#define IMX23_PXP_STAT_IRQ              (1U << 0)
#define IMX23_PXP_STAT_AXI_WRITE_ERROR  (1U << 1)
#define IMX23_PXP_STAT_AXI_READ_ERROR   (1U << 2)

/* Sizes are programmed in 8x8 blocks. */
#define IMX23_PXP_BLOCKS(px)            ((px) >> 3)
#define IMX23_PXP_BLOCK_PARAM(x, y, w, h) \
    (((uint32_t)(x) & 0xFFU) << 24 | ((uint32_t)(y) & 0xFFU) << 16 | \
     ((uint32_t)(w) & 0xFFU) << 8  | ((uint32_t)(h) & 0xFFU))

/* Scale registers: 2.12 fixed point, 0x1000 = 1:1. */
#define IMX23_PXP_SCALE_ONE             0x1000U

/*===========================================================================*/
/* AUDIOOUT (mxs-builtin DAC, playback path).                                */
/*===========================================================================*/

#define IMX23_AOUT_REG(off)     (IMX23_AUDIOOUT_BASE + (off))
#define IMX23_AOUT_CTRL         IMX23_AOUT_REG(0x000U)
#define IMX23_AOUT_STAT         IMX23_AOUT_REG(0x010U)
#define IMX23_AOUT_DACSRR       IMX23_AOUT_REG(0x020U)
#define IMX23_AOUT_DACVOLUME    IMX23_AOUT_REG(0x030U)
#define IMX23_AOUT_HPVOL        IMX23_AOUT_REG(0x050U)
#define IMX23_AOUT_PWRDN        IMX23_AOUT_REG(0x070U)
#define IMX23_AOUT_REFCTRL      IMX23_AOUT_REG(0x080U)
#define IMX23_AOUT_ANACTRL      IMX23_AOUT_REG(0x090U)
#define IMX23_AOUT_ANACLKCTRL   IMX23_AOUT_REG(0x0E0U)
#define IMX23_AOUT_DATA         IMX23_AOUT_REG(0x0F0U)
#define IMX23_AOUT_SPEAKERCTRL  IMX23_AOUT_REG(0x100U)
#define IMX23_AOUT_VERSION      IMX23_AOUT_REG(0x200U)

#define IMX23_AOUT_CTRL_SFTRST              (1U << 31)
#define IMX23_AOUT_CTRL_CLKGATE             (1U << 30)
#define IMX23_AOUT_CTRL_DMAWAIT_COUNT(v)    (((v) & 0x1FU) << 16)
#define IMX23_AOUT_CTRL_WORD_LENGTH         (1U << 6)   /* 1=16-bit, 0=32-bit */
#define IMX23_AOUT_CTRL_FIFO_UNDERFLOW_IRQ  (1U << 3)
#define IMX23_AOUT_CTRL_FIFO_OVERFLOW_IRQ   (1U << 2)
#define IMX23_AOUT_CTRL_FIFO_ERROR_IRQ_EN   (1U << 1)
#define IMX23_AOUT_CTRL_RUN                 (1U << 0)

#define IMX23_AOUT_DACSRR_OSR               (1U << 31)
#define IMX23_AOUT_DACSRR_BASEMULT(v)       (((v) & 0x7U) << 28)
#define IMX23_AOUT_DACSRR_SRC_HOLD(v)       (((v) & 0x7U) << 24)
#define IMX23_AOUT_DACSRR_SRC_INT(v)        (((v) & 0x1FU) << 16)
#define IMX23_AOUT_DACSRR_SRC_FRAC(v)       ((v) & 0xFFFFU)

#define IMX23_AOUT_DACVOLUME_EN_ZCD         (1U << 25)
#define IMX23_AOUT_DACVOLUME_MUTE_LEFT      (1U << 24)
#define IMX23_AOUT_DACVOLUME_VOL_LEFT(v)    (((v) & 0xFFU) << 16)
#define IMX23_AOUT_DACVOLUME_MUTE_RIGHT     (1U << 8)
#define IMX23_AOUT_DACVOLUME_VOL_RIGHT(v)   ((v) & 0xFFU)

#define IMX23_AOUT_HPVOL_EN_MSTR_ZCD        (1U << 25)
#define IMX23_AOUT_HPVOL_MUTE               (1U << 24)
#define IMX23_AOUT_HPVOL_VOL_LEFT(v)        (((v) & 0x7FU) << 8)
#define IMX23_AOUT_HPVOL_VOL_RIGHT(v)       ((v) & 0x7FU)

#define IMX23_AOUT_PWRDN_SPEAKER            (1U << 24)
#define IMX23_AOUT_PWRDN_DAC                (1U << 12)
#define IMX23_AOUT_PWRDN_CAPLESS            (1U << 4)
#define IMX23_AOUT_PWRDN_HEADPHONE          (1U << 0)

#define IMX23_AOUT_ANACTRL_HP_CLASSAB       (1U << 4)
#define IMX23_AOUT_ANACTRL_HP_HOLD_GND      (1U << 5)

#define IMX23_AOUT_ANACLKCTRL_CLKGATE       (1U << 31)
#define IMX23_AOUT_ANACLKCTRL_DACDIV(v)     ((v) & 0x7U)

#define IMX23_AOUT_SPEAKERCTRL_MUTE         (1U << 24)

/* APBX DMA channel bound to the DAC (audio-dac). */
#define IMX23_DMA_CH_AUDIO_DAC              1U

/* APBH DMA channels bound to the GPMI NAND chip selects. */
#define IMX23_DMA_CH_GPMI0                  0U
#define IMX23_DMA_CH_GPMI1                  1U
#define IMX23_DMA_CH_GPMI2                  2U
#define IMX23_DMA_CH_GPMI3                  3U

/*===========================================================================*/
/* GPMI NAND controller (0x8000C000).                                        */
/*===========================================================================*/

/* GPMI registers (each is an mxs_reg_32: REG/SET/CLR/TOG at +0/+4/+8/+12). */
#define IMX23_GPMI_CTRL0        (IMX23_GPMI_BASE + 0x00U)
#define IMX23_GPMI_COMPARE      (IMX23_GPMI_BASE + 0x10U)
#define IMX23_GPMI_ECCCTRL      (IMX23_GPMI_BASE + 0x20U)
#define IMX23_GPMI_ECCCOUNT     (IMX23_GPMI_BASE + 0x30U)
#define IMX23_GPMI_PAYLOAD      (IMX23_GPMI_BASE + 0x40U)
#define IMX23_GPMI_AUXILIARY    (IMX23_GPMI_BASE + 0x50U)
#define IMX23_GPMI_CTRL1        (IMX23_GPMI_BASE + 0x60U)
#define IMX23_GPMI_TIMING0      (IMX23_GPMI_BASE + 0x70U)
#define IMX23_GPMI_TIMING1      (IMX23_GPMI_BASE + 0x80U)
#define IMX23_GPMI_DATA         (IMX23_GPMI_BASE + 0xA0U)
#define IMX23_GPMI_STAT         (IMX23_GPMI_BASE + 0xB0U)
#define IMX23_GPMI_DEBUG        (IMX23_GPMI_BASE + 0xC0U)
#define IMX23_GPMI_VERSION      (IMX23_GPMI_BASE + 0xD0U)

/* HW_GPMI_CTRL0 bits. */
#define IMX23_GPMI_CTRL0_SFTRST             (1U << 31)
#define IMX23_GPMI_CTRL0_CLKGATE            (1U << 30)
#define IMX23_GPMI_CTRL0_RUN                (1U << 29)
#define IMX23_GPMI_CTRL0_DEV_IRQ_EN         (1U << 28)
#define IMX23_GPMI_CTRL0_LOCK_CS            (1U << 27)
#define IMX23_GPMI_CTRL0_UDMA               (1U << 26)
#define IMX23_GPMI_CTRL0_CMD_MODE_WRITE     (0x0U << 24)
#define IMX23_GPMI_CTRL0_CMD_MODE_READ      (0x1U << 24)
#define IMX23_GPMI_CTRL0_CMD_MODE_WAITRDY   (0x3U << 24)
#define IMX23_GPMI_CTRL0_WORD_LENGTH        (1U << 23)
#define IMX23_GPMI_CTRL0_CS(n)              (((n) & 0x7U) << 20)
#define IMX23_GPMI_CTRL0_ADDR_DATA          (0x0U << 17)
#define IMX23_GPMI_CTRL0_ADDR_CLE           (0x1U << 17)
#define IMX23_GPMI_CTRL0_ADDR_ALE           (0x2U << 17)
#define IMX23_GPMI_CTRL0_ADDR_INCREMENT     (1U << 16)
#define IMX23_GPMI_CTRL0_XFER_COUNT(n)      ((n) & 0xFFFFU)

/* HW_GPMI_CTRL1 bits. */
#define IMX23_GPMI_CTRL1_GPMI_MODE          (1U << 0)
#define IMX23_GPMI_CTRL1_ATA_IRQRDY_POL     (1U << 2)
#define IMX23_GPMI_CTRL1_DEV_RESET          (1U << 3)
#define IMX23_GPMI_CTRL1_BCH_MODE           (1U << 18)

/* HW_GPMI_ECCCTRL bits. */
#define IMX23_GPMI_ECCCTRL_ENABLE_ECC       (1U << 12)
#define IMX23_GPMI_ECCCTRL_ECC_CMD_DECODE   (0x0U << 13)
#define IMX23_GPMI_ECCCTRL_ECC_CMD_ENCODE   (0x1U << 13)
#define IMX23_GPMI_ECCCTRL_BCH_AUXONLY      0x100U
#define IMX23_GPMI_ECCCTRL_BCH_PAGE         0x1FFU

/* HW_GPMI_TIMING0 fields. */
#define IMX23_GPMI_TIMING0_ADDR_SETUP(n)    (((n) & 0xFFU) << 16)
#define IMX23_GPMI_TIMING0_DATA_HOLD(n)     (((n) & 0xFFU) << 8)
#define IMX23_GPMI_TIMING0_DATA_SETUP(n)    ((n) & 0xFFU)

/* HW_GPMI_TIMING1 fields. */
#define IMX23_GPMI_TIMING1_BUSY_TIMEOUT(n)  (((n) & 0xFFFFU) << 16)

/*===========================================================================*/
/* USB OTG controller (ci13xxx) + PHY.                                       */
/*===========================================================================*/

#define IMX23_DIGCTRL_BASE      0x8001C000U
#define IMX23_DIGCTRL_CTRL      (IMX23_DIGCTRL_BASE + 0x000U)
#define IMX23_DIGCTRL_CTRL_USB_CLKGATE      (1U << 2)   /* "usb" clock gate   */

/* USB PHY (0x8007C000). */
#define IMX23_USBPHY_PWD        (IMX23_USBPHY0_BASE + 0x00U)
#define IMX23_USBPHY_TX         (IMX23_USBPHY0_BASE + 0x10U)
#define IMX23_USBPHY_RX         (IMX23_USBPHY0_BASE + 0x20U)
#define IMX23_USBPHY_CTRL       (IMX23_USBPHY0_BASE + 0x30U)
#define IMX23_USBPHY_STATUS     (IMX23_USBPHY0_BASE + 0x40U)
#define IMX23_USBPHY_DEBUG      (IMX23_USBPHY0_BASE + 0x50U)
#define IMX23_USBPHY_PLL_SIC    (IMX23_USBPHY0_BASE + 0xA0U)
#define IMX23_USBPHY_CTRL_SFTRST            (1U << 31)
#define IMX23_USBPHY_CTRL_CLKGATE           (1U << 30)
#define IMX23_USBPHY_CTRL_ENAUTOSET_USBCLKS (1U << 26)
#define IMX23_USBPHY_CTRL_ENAUTOCLR_USBCLKGATE (1U << 25)
#define IMX23_USBPHY_CTRL_ENAUTOCLR_PHY_PWD (1U << 20)
#define IMX23_USBPHY_CTRL_ENAUTOCLR_CLKGATE (1U << 19)
#define IMX23_USBPHY_CTRL_ENAUTO_PWRON_PLL  (1U << 18)
#define IMX23_USBPHY_CTRL_ENUTMILEVEL3      (1U << 15)
#define IMX23_USBPHY_CTRL_ENUTMILEVEL2      (1U << 14)
#define IMX23_USBPHY_PLL_LOCK               (1U << 31)

/* USB controller capability registers (0x80080000 + 0x000..0x124). */
#define IMX23_USBC_ID           (IMX23_USBCTRL0_BASE + 0x000U)
#define IMX23_USBC_CAPLENGTH    (IMX23_USBCTRL0_BASE + 0x100U)
#define IMX23_USBC_DCCPARAMS    (IMX23_USBCTRL0_BASE + 0x124U)

/* USB controller operational registers (0x80080000 + 0x140..). */
#define IMX23_USBC_USBCMD       (IMX23_USBCTRL0_BASE + 0x140U)
#define IMX23_USBC_USBSTS       (IMX23_USBCTRL0_BASE + 0x144U)
#define IMX23_USBC_USBINTR      (IMX23_USBCTRL0_BASE + 0x148U)
#define IMX23_USBC_FRINDEX      (IMX23_USBCTRL0_BASE + 0x14CU)
#define IMX23_USBC_DEVICEADDR   (IMX23_USBCTRL0_BASE + 0x154U)
#define IMX23_USBC_ENDPTLISTADDR (IMX23_USBCTRL0_BASE + 0x158U)
#define IMX23_USBC_PORTSC1      (IMX23_USBCTRL0_BASE + 0x184U)
#define IMX23_USBC_OTGSC        (IMX23_USBCTRL0_BASE + 0x1A4U)
#define IMX23_USBC_USBMODE      (IMX23_USBCTRL0_BASE + 0x1A8U)
#define IMX23_USBC_ENDPTSETUPSTAT (IMX23_USBCTRL0_BASE + 0x1ACU)
#define IMX23_USBC_ENDPTPRIME   (IMX23_USBCTRL0_BASE + 0x1B0U)
#define IMX23_USBC_ENDPTFLUSH   (IMX23_USBCTRL0_BASE + 0x1B4U)
#define IMX23_USBC_ENDPTSTAT    (IMX23_USBCTRL0_BASE + 0x1B8U)
#define IMX23_USBC_ENDPTCOMPLETE (IMX23_USBCTRL0_BASE + 0x1BCU)
#define IMX23_USBC_ENDPTCTRL(n) (IMX23_USBCTRL0_BASE + 0x1C0U + (n) * 4U)

/* HW_USBC_USBCMD bits. */
#define IMX23_USBCMD_RUN        (1U << 0)
#define IMX23_USBCMD_RST        (1U << 1)
#define IMX23_USBCMD_FS2        (1U << 15)
#define IMX23_USBCMD_ITC(n)     (((n) & 0xFFU) << 16)

/* HW_USBC_USBSTS / HW_USBC_USBINTR bits. */
#define IMX23_USBSTS_UI         (1U << 0)
#define IMX23_USBSTS_UEI        (1U << 1)
#define IMX23_USBSTS_PCI        (1U << 2)
#define IMX23_USBSTS_URI        (1U << 6)
#define IMX23_USBSTS_SLI        (1U << 8)

/* HW_USBC_USBMODE values. */
#define IMX23_USBMODE_DEVICE    2U
#define IMX23_USBMODE_CM_MASK   0x3U

/* HW_USBC_ENDPTCTRLn bits. */
#define IMX23_ENDPTCTRL_TXE     (1U << 23)
#define IMX23_ENDPTCTRL_TXR     (1U << 22)
#define IMX23_ENDPTCTRL_TXT(t)  (((t) & 0x3U) << 18)
#define IMX23_ENDPTCTRL_RXE     (1U << 7)
#define IMX23_ENDPTCTRL_RXR     (1U << 6)
#define IMX23_ENDPTCTRL_RXT(t)  (((t) & 0x3U) << 2)

/* Endpoint prime/flush/complete: RX bit = ep, TX bit = ep+16. */
#define IMX23_EPT_RX(n)         (1U << (n))
#define IMX23_EPT_TX(n)         (1U << (16U + (n)))

/*===========================================================================*/
/* BCH ECC accelerator (0x8000A000).                                         */
/*===========================================================================*/

#define IMX23_BCH_CTRL          (IMX23_BCH_BASE + 0x00U)
#define IMX23_BCH_STATUS0       (IMX23_BCH_BASE + 0x10U)
#define IMX23_BCH_MODE          (IMX23_BCH_BASE + 0x20U)
#define IMX23_BCH_LAYOUTSELECT  (IMX23_BCH_BASE + 0x70U)
#define IMX23_BCH_FLASH0LAYOUT0 (IMX23_BCH_BASE + 0x80U)
#define IMX23_BCH_FLASH0LAYOUT1 (IMX23_BCH_BASE + 0x90U)
#define IMX23_BCH_VERSION       (IMX23_BCH_BASE + 0xE0U)

/* HW_BCH_CTRL bits. */
#define IMX23_BCH_CTRL_SFTRST           (1U << 31)
#define IMX23_BCH_CTRL_CLKGATE          (1U << 30)
#define IMX23_BCH_CTRL_COMPLETE_IRQ_EN  (1U << 8)
#define IMX23_BCH_CTRL_COMPLETE_IRQ     (1U << 0)

/* HW_BCH_STATUS0 fields. */
#define IMX23_BCH_STATUS0_BLK0(s)       (((s) >> 8) & 0xFFU)
#define IMX23_BCH_STATUS_BLK_OK         0x00U
#define IMX23_BCH_STATUS_BLK_ERASED     0xFFU
#define IMX23_BCH_STATUS_BLK_UNCORR     0xFEU

/* HW_BCH_FLASH0LAYOUT0 fields (i.MX23: ECC0 at bits 15:12, GF at bit 10). */
#define IMX23_BCH_LAYOUT0_NBLOCKS(n)    (((n) & 0xFFU) << 24)
#define IMX23_BCH_LAYOUT0_META_SIZE(n)  (((n) & 0xFFU) << 16)
#define IMX23_BCH_LAYOUT0_ECC0(n)       (((n) & 0xFU) << 12)
#define IMX23_BCH_LAYOUT0_GF14          (1U << 10)
#define IMX23_BCH_LAYOUT0_CHUNK0(n)     ((n) & 0x3FFU)

/* HW_BCH_FLASH0LAYOUT1 fields. */
#define IMX23_BCH_LAYOUT1_PAGE_SIZE(n)  (((n) & 0xFFFFU) << 16)
#define IMX23_BCH_LAYOUT1_ECCN(n)       (((n) & 0xFU) << 12)
#define IMX23_BCH_LAYOUT1_GF14          (1U << 10)
#define IMX23_BCH_LAYOUT1_CHUNKN(n)     ((n) & 0x3FFU)

#endif /* IMX23_H */
