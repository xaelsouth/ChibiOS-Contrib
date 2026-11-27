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
 * @file    efr32_clocks.c
 * @brief   EFR32FG23 clock tree inspection helpers.
 *
 * @details Port-side implementation of the clock listing used by the
 *          xshell "clocks" command (LLD/xshell/efr32_clock_cmds.c), but
 *          usable from any code with a BaseSequentialStream:
 *          - efr32_clock_points_print()  flat list of clock points + rates
 *          - efr32_clock_tree_print()    selected parents, rates and
 *                                        enabled bus-clock gates
 *
 * @addtogroup EFR32FG23_CLOCKS
 * @{
 */

#include "hal.h"
#include "chprintf.h"
#include "efr32_clocks.h"

/*===========================================================================*/
/* Flat list of clock points.                                                */
/*===========================================================================*/

/**
 * @brief   Prints clock points, selected parents and bus gate counts.
 *
 * @param[in] ssp       output stream
 */
void efr32_clock_points_print(BaseSequentialStream *ssp) {
  efr32_clock_tree_print(ssp);
}

void cli_clock_points_print(BaseSequentialStream *ssp);
void cli_clock_points_print(BaseSequentialStream *ssp) {
  efr32_clock_points_print(ssp);
}
/*===========================================================================*/
/* Consumer registry.                                                        */
/*===========================================================================*/

static uint8_t efr32_clk_consumers[CLK_ARRAY_SIZE];

void efr32_clock_consumer_acquire(halclkpt_t pt) {

  osalSysLock();
  if ((unsigned)pt < (unsigned)CLK_ARRAY_SIZE) {
    efr32_clk_consumers[pt]++;
  }
  osalSysUnlock();
}

void efr32_clock_consumer_release(halclkpt_t pt) {

  osalSysLock();
  if (((unsigned)pt < (unsigned)CLK_ARRAY_SIZE) && (efr32_clk_consumers[pt] > 0U)) {
    efr32_clk_consumers[pt]--;
  }
  osalSysUnlock();
}

unsigned efr32_clock_consumers(halclkpt_t pt) {

  return ((unsigned)pt < (unsigned)CLK_ARRAY_SIZE) ? efr32_clk_consumers[pt] : 0U;
}

/*
 * Clock points that stop in EM2/EM3 — a consumer on any of these blocks
 * EM2.  The LF/EM23/EM4 domain points keep running in EM2.
 */
bool efr32_clock_pt_blocks_em2(halclkpt_t pt) {

  switch (pt) {
  case EFR32_CLK_HFRCOEM23:
  case EFR32_CLK_EM23GRPACLK:
  case EFR32_CLK_EM4GRPACLK:
  case EFR32_CLK_LFXO:
  case EFR32_CLK_LFRCO:
  case EFR32_CLK_ULFRCO:
    return false;
  default:
    return true;
  }
}

bool efr32_em2_allowed(void) {
  unsigned i;
  bool allowed = true;

  osalSysLock();
  for (i = 0U; i < (unsigned)CLK_ARRAY_SIZE; i++) {
    if ((efr32_clk_consumers[i] > 0U) && efr32_clock_pt_blocks_em2((halclkpt_t)i)) {
      allowed = false;
      break;
    }
  }
  osalSysUnlock();
  return allowed;
}

/*===========================================================================*/
/* cli hooks (os/various/cli/clock_cmds.c): users column + lowpower command. */
/*                                                                           */
/* These are strong definitions overriding the weak defaults in the       */
/* generic cli layer.  They live here because they wrap the port-private  */
/* consumer registry; the registry queries themselves are public          */
/* (efr32_clock_consumers / efr32_clock_pt_blocks_em2 / efr32_em2_allowed)*/
/* and the EM2 entry primitive is efr32_enter_em2() from hal_lld.c, so an */
/* application that wants different behaviour (e.g. enter EM3, custom     */
/* sequence) can define EFR32_CLI_PM_HOOKS_IN_APP in its mcuconf.h and     */
/* provide its own cli_* hooks in the application instead.                */
/*===========================================================================*/
#if !defined(EFR32_CLI_PM_HOOKS_IN_APP) || defined(__DOXYGEN__)

const char *cli_pm_lowpower_name(void);
const char *cli_pm_lowpower_name(void) {

  /* On Series 2 the low-power mode entered by the "lowpower" command
     is EM2. */
  return "EM2";
}

unsigned cli_clock_users(halclkpt_t pt);
unsigned cli_clock_users(halclkpt_t pt) {

  return efr32_clock_consumers(pt);
}

bool cli_pm_point_blocks_lowpower(halclkpt_t pt);
bool cli_pm_point_blocks_lowpower(halclkpt_t pt) {

  return efr32_clock_pt_blocks_em2(pt);
}

bool cli_pm_lowpower_allowed(void);
bool cli_pm_lowpower_allowed(void) {

  return efr32_em2_allowed();
}

void cli_pm_lowpower_enter_once(void);
void cli_pm_lowpower_enter_once(void) {

  /* The actual entry sequence (SLEEPDEEP + WFI) is chip-level code and
     lives in hal_lld.c as efr32_enter_em2(). */
  efr32_enter_em2();
}

#endif /* !EFR32_CLI_PM_HOOKS_IN_APP */

/*===========================================================================*/
/* Clock relationships and bus gate diagnostics.                            */
/*===========================================================================*/

/**
 * @brief   One node of the FG23 clock tree.
 */
typedef struct {
  const char  *name;      /**< Node name.                                    */
  int          parent;    /**< Parent node index, -1 for roots.              */
  halclkpt_t   pt;        /**< Clock point for the rate.                     */
} efr32_clk_node_t;

/*
 * FG23 clock tree (Series 2):
 *
 *   HFRCO --> HFRCODPLL --+--> SYSCLK --> HCLK --> PCLK --> LSPCLK
 *                         |        +--> RHCLK
 *                         +--> EM01GRPACLK
 *                         +--> EM01GRPCCLK
 *   HFRCOEM23
 *   FSRCO ----------------> SYSCLK (alternative source)
 *   HFXO -----------------> SYSCLK / EM01GRPACLK (alternative source)
 *   CLKIN0 ---------------> SYSCLK (alternative source)
 *   LFXO --+-> EM23GRPACLK / EM4GRPACLK (BURTC)
 *   LFRCO -+
 *   ULFRCO-+
 */

/* Node indices in efr32_clk_tree[]. */
#define N_HFRCODPLL   0U
#define N_SYSCLK      1U
#define N_HCLK        2U
#define N_PCLK        3U
#define N_RHCLK       4U
#define N_HFRCOEM23   5U
#define N_FSRCO       6U
#define N_HFXO        7U
#define N_CLKIN0      8U
#define N_EM01GRPACLK 9U
#define N_EM01GRPCCLK 10U
#define N_EM23GRPACLK 11U
#define N_EM4GRPACLK  12U
#define N_LFXO        13U
#define N_LFRCO       14U
#define N_ULFRCO      15U
#define N_LSPCLK      16U

static const efr32_clk_node_t efr32_clk_tree[] = {
  /* name            parent              clock point              */
  { "HFRCODPLL",      -1,                 EFR32_CLK_HFRCODPLL   }, /* N_HFRCODPLL   */
  { "SYSCLK",         N_HFRCODPLL,        EFR32_CLK_SYSCLK      }, /* N_SYSCLK      */
  { "HCLK",           N_SYSCLK,           EFR32_CLK_HCLK        }, /* N_HCLK        */
  { "PCLK",           N_HCLK,             EFR32_CLK_PCLK        }, /* N_PCLK        */
  { "RHCLK",          N_SYSCLK,           EFR32_CLK_RHCLK       }, /* N_RHCLK       */
  { "HFRCOEM23",      -1,                 EFR32_CLK_HFRCOEM23   }, /* N_HFRCOEM23   */
  { "FSRCO",          -1,                 EFR32_CLK_FSRCO       }, /* N_FSRCO       */
  { "HFXO",           -1,                 EFR32_CLK_HFXO        }, /* N_HFXO        */
  { "CLKIN0",         -1,                 EFR32_CLK_CLKIN0      }, /* N_CLKIN0      */
  { "EM01GRPACLK",    N_HFRCODPLL,        EFR32_CLK_EM01GRPACLK }, /* N_EM01GRPACLK */
  { "EM01GRPCCLK",    N_HFRCODPLL,        EFR32_CLK_EM01GRPCCLK }, /* N_EM01GRPCCLK */
  { "EM23GRPACLK",    -1,                 EFR32_CLK_EM23GRPACLK }, /* N_EM23GRPACLK */
  { "EM4GRPACLK",     N_EM23GRPACLK,      EFR32_CLK_EM4GRPACLK  }, /* N_EM4GRPACLK  */
  { "LFXO",           -1,                 EFR32_CLK_LFXO        }, /* N_LFXO        */
  { "LFRCO",          -1,                 EFR32_CLK_LFRCO       }, /* N_LFRCO       */
  { "ULFRCO",         -1,                 EFR32_CLK_ULFRCO      }, /* N_ULFRCO      */
  { "LSPCLK",         N_PCLK,             EFR32_CLK_PCLK        }  /* N_LSPCLK      */
};
#define EFR32_CLK_TREE_SIZE  (sizeof(efr32_clk_tree) / sizeof(efr32_clk_tree[0]))

/**
 * @brief   Returns "on"/"off" for an oscillator from its STATUS register.
 * @note    The peripheral bus clock (CMU_CLKEN0 bit) must be checked first:
 *          reading the STATUS register of an oscillator whose bus clock is
 *          disabled causes a bus fault on Series 2.
 */
static const char *osc_state(uint32_t clken_bit, const volatile uint32_t *status,
                             uint32_t rdy_mask) {

  if ((CMU->CLKEN0 & clken_bit) == 0U) {
    return "status unavailable";
  }
  return ((*status & rdy_mask) != 0U) ? "on" : "off";
}

/**
 * @brief   Returns the name of the SYSCLK source selected in CMU_SYSCLKCTRL.
 */
static const char *sysclk_source(void) {

  switch (CMU->SYSCLKCTRL & _CMU_SYSCLKCTRL_CLKSEL_MASK) {
  case _CMU_SYSCLKCTRL_CLKSEL_FSRCO:     return "FSRCO";
  case _CMU_SYSCLKCTRL_CLKSEL_HFRCODPLL: return "HFRCODPLL";
  case _CMU_SYSCLKCTRL_CLKSEL_HFXO:      return "HFXO";
  case _CMU_SYSCLKCTRL_CLKSEL_CLKIN0:    return "CLKIN0";
  default:                               return "?";
  }
}

/**
 * @brief   Returns the name of the EM23/EM4 group A clock source.
 */
static const char *lf_source(uint32_t sel) {
  switch (sel) {
  case _CMU_EM23GRPACLKCTRL_CLKSEL_LFXO:   return "LFXO";
  case _CMU_EM23GRPACLKCTRL_CLKSEL_LFRCO:  return "LFRCO";
  case _CMU_EM23GRPACLKCTRL_CLKSEL_ULFRCO: return "ULFRCO";
  case 0U:                                 return "disabled";
  default:                                 return "?";
  }
}

static const char *hf_group_source(uint32_t sel) {
  switch (sel) {
  case _CMU_EM01GRPACLKCTRL_CLKSEL_HFRCODPLL:   return "HFRCODPLL";
  case _CMU_EM01GRPACLKCTRL_CLKSEL_HFXO:        return "HFXO";
  case _CMU_EM01GRPACLKCTRL_CLKSEL_FSRCO:       return "FSRCO";
  case _CMU_EM01GRPACLKCTRL_CLKSEL_HFRCOEM23:   return "HFRCOEM23";
  case _CMU_EM01GRPACLKCTRL_CLKSEL_HFRCODPLLRT: return "HFRCODPLLRT";
  case _CMU_EM01GRPACLKCTRL_CLKSEL_HFXORT:      return "HFXORT";
  case 0U:                                      return "disabled";
  default:                                      return "?";
  }
}

static const char *node_parent(unsigned i) {
  switch (i) {
  case N_SYSCLK: return sysclk_source();
  case N_EM01GRPACLK:
    return hf_group_source(CMU->EM01GRPACLKCTRL & _CMU_EM01GRPACLKCTRL_CLKSEL_MASK);
  case N_EM01GRPCCLK:
    return hf_group_source(CMU->EM01GRPCCLKCTRL & _CMU_EM01GRPCCLKCTRL_CLKSEL_MASK);
  case N_EM23GRPACLK:
    return lf_source(CMU->EM23GRPACLKCTRL & _CMU_EM23GRPACLKCTRL_CLKSEL_MASK);
  case N_EM4GRPACLK:
    return lf_source(CMU->EM4GRPACLKCTRL & _CMU_EM4GRPACLKCTRL_CLKSEL_MASK);
  default:
    return efr32_clk_tree[i].parent < 0 ? "-" :
           efr32_clk_tree[efr32_clk_tree[i].parent].name;
  }
}

/**
 * @brief   Extra run-time info string for a tree node.
 */
static void node_info(unsigned i, char *buf, size_t buflen) {

  buf[0] = '\0';

  switch (i) {
  case N_HFRCOEM23:
    chsnprintf(buf, buflen, "%s", osc_state(CMU_CLKEN0_HFRCOEM23, &HFRCOEM23->STATUS, _HFRCO_STATUS_RDY_MASK));
    break;
  case N_HFXO:
    chsnprintf(buf, buflen, "%s", osc_state(CMU_CLKEN0_HFXO0, &HFXO0->STATUS, _HFXO_STATUS_RDY_MASK));
    break;
  case N_LFXO:
    chsnprintf(buf, buflen, "%s", osc_state(CMU_CLKEN0_LFXO, &LFXO->STATUS, _LFXO_STATUS_RDY_MASK));
    break;
  case N_LFRCO:
    chsnprintf(buf, buflen, "%s", osc_state(CMU_CLKEN0_LFRCO, &LFRCO->STATUS, _LFRCO_STATUS_RDY_MASK));
    break;
  case N_ULFRCO:
    chsnprintf(buf, buflen, "%s", osc_state(CMU_CLKEN0_ULFRCO, &ULFRCO->STATUS, _ULFRCO_STATUS_RDY_MASK));
    break;
  default:
    break;
  }
}

/* CLKEN bits enable bus interfaces, not the peripheral functional clocks.
   Some bits control interfaces on more than one bus. */
static int bus_gate_count(unsigned i, uint32_t en0, uint32_t en1) {
  uint32_t mask0, mask1, bits;
  unsigned count = 0U;

  switch (i) {
  case N_HCLK:
    mask0 = CMU_CLKEN0_LDMA | CMU_CLKEN0_RADIOAES;
    mask1 = CMU_CLKEN1_SMU | CMU_CLKEN1_ICACHE0 | CMU_CLKEN1_RFECA0 |
            CMU_CLKEN1_RFECA1 | CMU_CLKEN1_DMEM | CMU_CLKEN1_SEMAILBOXHOST |
            CMU_CLKEN1_ECAIFADC;
    break;
  case N_PCLK:
    mask0 = CMU_CLKEN0_LDMA | CMU_CLKEN0_LDMAXBAR | CMU_CLKEN0_GPCRC |
            CMU_CLKEN0_TIMER0 | CMU_CLKEN0_TIMER1 | CMU_CLKEN0_TIMER2 |
            CMU_CLKEN0_TIMER3 | CMU_CLKEN0_TIMER4 | CMU_CLKEN0_USART0 |
            CMU_CLKEN0_I2C1 | CMU_CLKEN0_SYSCFG | CMU_CLKEN0_DPLL0 |
            CMU_CLKEN0_HFRCO0 | CMU_CLKEN0_FSRCO | CMU_CLKEN0_LFRCO |
            CMU_CLKEN0_LFXO | CMU_CLKEN0_ULFRCO | CMU_CLKEN0_GPIO |
            CMU_CLKEN0_PRS | CMU_CLKEN0_BURAM | CMU_CLKEN0_BURTC |
            CMU_CLKEN0_SYSRTC0 | CMU_CLKEN0_DCDC;
    mask1 = CMU_CLKEN1_HOSTMAILBOX | CMU_CLKEN1_LCD | CMU_CLKEN1_KEYSCAN |
            CMU_CLKEN1_ICACHE0 | CMU_CLKEN1_EUSART1 | CMU_CLKEN1_EUSART2 |
            CMU_CLKEN1_DMEM;
    break;
  case N_LSPCLK:
    mask0 = CMU_CLKEN0_IADC0 | CMU_CLKEN0_AMUXCP0 | CMU_CLKEN0_LETIMER0 |
            CMU_CLKEN0_WDOG0 | CMU_CLKEN0_I2C0 | CMU_CLKEN0_HFRCOEM23 |
            CMU_CLKEN0_HFXO0 | CMU_CLKEN0_LESENSE;
    mask1 = CMU_CLKEN1_WDOG1 | CMU_CLKEN1_ACMP0 | CMU_CLKEN1_ACMP1 |
            CMU_CLKEN1_VDAC0 | CMU_CLKEN1_PCNT0 | CMU_CLKEN1_EUSART0;
    break;
  case N_RHCLK:
    mask0 = 0U;
    mask1 = CMU_CLKEN1_AGC | CMU_CLKEN1_MODEM | CMU_CLKEN1_RFCRC |
            CMU_CLKEN1_FRC | CMU_CLKEN1_PROTIMER | CMU_CLKEN1_RAC |
            CMU_CLKEN1_SYNTH | CMU_CLKEN1_RFSCRATCHPAD |
            CMU_CLKEN1_RFMAILBOX | CMU_CLKEN1_BUFC | CMU_CLKEN1_RFECA0 |
            CMU_CLKEN1_RFECA1;
    break;
  default:
    return -1;
  }
  bits = en0 & mask0;
  while (bits != 0U) {
    bits &= bits - 1U;
    count++;
  }
  bits = en1 & mask1;
  while (bits != 0U) {
    bits &= bits - 1U;
    count++;
  }
  return (int)count;
}

static void print_node(BaseSequentialStream *ssp, unsigned i,
                       uint32_t en0, uint32_t en1) {
  char info[24], gates[12];
  halfreq_t rate = halClockGetPointX(efr32_clk_tree[i].pt);
  int count = bus_gate_count(i, en0, en1);

  if (i == N_LSPCLK)
    rate /= 2U;
  node_info(i, info, sizeof(info));
  if (count < 0)
    chsnprintf(gates, sizeof(gates), "-");
  else
    chsnprintf(gates, sizeof(gates), "%u", (unsigned)count);
  chprintf(ssp, "%-14s %-14s %12u %6s", efr32_clk_tree[i].name,
           node_parent(i), (unsigned)rate, gates);
  if (info[0] != '\0')
    chprintf(ssp, "  %s", info);
  chprintf(ssp, "\r\n");
}

/**
 * @brief   Strong override of the generic "clocks w" hook.
 */
void cli_clock_tree_print(BaseSequentialStream *ssp);
void cli_clock_tree_print(BaseSequentialStream *ssp) {

  efr32_clock_tree_print(ssp);
}

/**
 * @brief   Prints the FG23 clock points and their selected parents.
 * @details Rates come from halClockGetPointX(); sources, oscillator state
 *          and mapped bus gate enables are read back from hardware.
 *
 * @param[in] ssp       output stream
 */
void efr32_clock_tree_print(BaseSequentialStream *ssp) {
  uint32_t en0 = CMU->CLKEN0, en1 = CMU->CLKEN1;
  chprintf(ssp, "FG23 clocks (nominal rates; mapped bus gates; - = not counted):\r\n");
  chprintf(ssp, "%-14s %-14s %12s %6s\r\n",
           "clock", "parent", "rate [Hz]", "gates");
  for (unsigned i = 0U; i < (unsigned)EFR32_CLK_TREE_SIZE; i++)
    print_node(ssp, i, en0, en1);
}

/** @} */
