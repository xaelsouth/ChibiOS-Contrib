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
 * @file    efr32_clock_cmds.c
 * @brief   xshell commands for the SILABS clock tree.
 *
 * @details "clocks" prints the flat list of clock points via
 *          halClockGetPointX(); "clocks w" prints the clock tree in a
 *          Linux clk debugfs style, with the currently selected sources
 *          and the on/off state of the oscillators read back from the
 *          CMU. Requires the port to define CLK_ARRAY_SIZE and
 *          CLK_POINT_NAMES (all SILABS EFR32 ports do).
 *
 * @addtogroup SILABS_XSHELL_CLOCKS
 * @{
 */

#include <string.h>
#include <stdio.h>

#include "hal.h"
#include "xshell.h"

#include "efr32_clock_cmds.h"

#if !defined(CLK_ARRAY_SIZE) || !defined(CLK_POINT_NAMES)
#error "efr32_clock_cmds.c requires the port to define CLK_ARRAY_SIZE and CLK_POINT_NAMES"
#endif

/*===========================================================================*/
/* Flat list.                                                                */
/*===========================================================================*/

static void clocks_flat(xshell_t *xshp) {
  static const char *names[CLK_ARRAY_SIZE] = CLK_POINT_NAMES;
  unsigned i;

  chprintf(xshp->stream, "%-14s %12s" XSHELL_NEWLINE_STR, "clock", "rate [Hz]");
  for (i = 0U; i < (unsigned)CLK_ARRAY_SIZE; i++) {
    chprintf(xshp->stream, "%-14s %12u" XSHELL_NEWLINE_STR,
             names[i], (unsigned)halClockGetPointX(i));
  }
}

/*===========================================================================*/
/* Tree view ("clocks w"), EFR32FG23 (Series 2).                             */
/*===========================================================================*/

#if defined(EFR32FG23X) || defined(__DOXYGEN__)

/**
 * @brief   One node of the FG23 clock tree.
 */
typedef struct {
  const char  *name;      /**< Node name.                                    */
  int          parent;    /**< Parent node index, -1 for roots.               */
  halclkpt_t   pt;        /**< Clock point for the rate, CLK_ARRAY_SIZE if
                               the node has no clock point of its own.       */
} efr32_clk_node_t;

/*
 * FG23 clock tree (Series 2):
 *
 *   HFRCO ──> HFRCODPLL ──┬─> SYSCLK ──> HCLK ──┬─> PCLK
 *                         │                     └─> RHCLK
 *                         ├─> EM01GRPACLK
 *                         └─> EM01GRPCCLK
 *   HFRCOEM23
 *   FSRCO ────────────────> SYSCLK (alternative source)
 *   HFXO ─────────────────> SYSCLK / EM01GRPACLK (alternative source)
 *   CLKIN0 ───────────────> SYSCLK (alternative source)
 *   LFXO ─┬─> EM23GRPACLK ──> LETIMER0, BURTC (via EM4GRPACLK)
 *   LFRCO ─┤
 *   ULFRCO┴
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

static const efr32_clk_node_t efr32_clk_tree[] = {
  /* name            parent              clock point              */
  { "HFRCODPLL",      -1,                 EFR32_CLK_HFRCODPLL   }, /* N_HFRCODPLL   */
  { "SYSCLK",         N_HFRCODPLL,        EFR32_CLK_SYSCLK      }, /* N_SYSCLK      */
  { "HCLK",           N_SYSCLK,           EFR32_CLK_HCLK        }, /* N_HCLK        */
  { "PCLK",           N_HCLK,             EFR32_CLK_PCLK        }, /* N_PCLK        */
  { "RHCLK",          N_HCLK,             EFR32_CLK_RHCLK       }, /* N_RHCLK       */
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
  { "ULFRCO",         -1,                 EFR32_CLK_ULFRCO      }  /* N_ULFRCO      */
};
#define EFR32_CLK_TREE_SIZE  (sizeof(efr32_clk_tree) / sizeof(efr32_clk_tree[0]))

/**
 * @brief   Returns "on"/"off" for an oscillator from its STATUS register.
 */
static const char *osc_state(volatile uint32_t *status, uint32_t rdy_mask) {

  return ((*status & rdy_mask) != 0U) ? "on" : "off";
}

/**
 * @brief   Returns the name of the SYSCLK source selected in CMU_SYSCLKCTRL.
 */
static const char *sysclk_source(void) {

  switch (CMU->SYSCLKCTRL & _CMU_SYSCLKCTRL_CLKSEL_MASK) {
  case _CMU_SYSCLKCTRL_CLKSEL_FSRCO:     return "fsrco";
  case _CMU_SYSCLKCTRL_CLKSEL_HFRCODPLL: return "hfrcodpll";
  case _CMU_SYSCLKCTRL_CLKSEL_HFXO:      return "hfxo";
  case _CMU_SYSCLKCTRL_CLKSEL_CLKIN0:    return "clkin0";
  default:                               return "?";
  }
}

/**
 * @brief   Returns the name of the EM23/EM4 group A clock source.
 */
static const char *em23grpaclk_source(void) {

  switch (CMU->EM23GRPACLKCTRL & _CMU_EM23GRPACLKCTRL_CLKSEL_MASK) {
  case _CMU_EM23GRPACLKCTRL_CLKSEL_LFXO:   return "lfxo";
  case _CMU_EM23GRPACLKCTRL_CLKSEL_LFRCO:  return "lfrco";
  case _CMU_EM23GRPACLKCTRL_CLKSEL_ULFRCO: return "ulfrco";
  default:                                 return "none";
  }
}

/**
 * @brief   Extra run-time info string for a tree node.
 */
static void node_info(unsigned i, char *buf, size_t buflen) {

  buf[0] = '\0';

  switch (i) {
  case N_SYSCLK:
    snprintf(buf, buflen, "src=%s", sysclk_source());
    break;
  case N_HFRCOEM23:
    snprintf(buf, buflen, "%s", osc_state(&HFRCOEM23->STATUS, _HFRCO_STATUS_RDY_MASK));
    break;
  case N_FSRCO:
    snprintf(buf, buflen, "always-on");  /* FSRCO has no STATUS register */
    break;
  case N_HFXO:
    snprintf(buf, buflen, "%s", osc_state(&HFXO0->STATUS, _HFXO_STATUS_RDY_MASK));
    break;
  case N_EM23GRPACLK:
    snprintf(buf, buflen, "src=%s", em23grpaclk_source());
    break;
  case N_LFXO:
    snprintf(buf, buflen, "%s", osc_state(&LFXO->STATUS, _LFXO_STATUS_RDY_MASK));
    break;
  case N_LFRCO:
    snprintf(buf, buflen, "%s", osc_state(&LFRCO->STATUS, _LFRCO_STATUS_RDY_MASK));
    break;
  case N_ULFRCO:
    snprintf(buf, buflen, "%s", osc_state(&ULFRCO->STATUS, _ULFRCO_STATUS_RDY_MASK));
    break;
  default:
    break;
  }
}

/**
 * @brief   Prints one tree node with indentation.
 */
static void print_node(xshell_t *xshp, unsigned i, const char *prefix, bool last) {
  char info[24];
  halfreq_t rate;

  node_info(i, info, sizeof(info));
  rate = (efr32_clk_tree[i].pt < (halclkpt_t)CLK_ARRAY_SIZE)
         ? halClockGetPointX(efr32_clk_tree[i].pt) : 0U;

  chprintf(xshp->stream, "%s%s%-14s %10u Hz", prefix, last ? "`-- " : "|-- ",
           efr32_clk_tree[i].name, (unsigned)rate);
  if (info[0] != '\0') {
    chprintf(xshp->stream, "  (%s)", info);
  }
  chprintf(xshp->stream, XSHELL_NEWLINE_STR);
}

static void clocks_tree(xshell_t *xshp) {
  char prefix[64];
  unsigned i;

  chprintf(xshp->stream, "FG23 clock tree:" XSHELL_NEWLINE_STR);

  for (i = 0U; i < (unsigned)EFR32_CLK_TREE_SIZE; i++) {
    /* Build the indentation prefix from the parent chain. */
    int parent = efr32_clk_tree[i].parent;
    unsigned depth = 0U;
    bool last = true;
    unsigned j;

    prefix[0] = '\0';

    /* A node is "last" if no later sibling shares its parent. */
    for (j = i + 1U; j < (unsigned)EFR32_CLK_TREE_SIZE; j++) {
      if (efr32_clk_tree[j].parent == parent) {
        last = false;
        break;
      }
    }

    /* Indent: walk parents, prepend "    " or "|   ". */
    while (parent >= 0 && depth < 15U) {
      bool parent_last = true;
      unsigned k;
      for (k = (unsigned)parent + 1U; k < (unsigned)EFR32_CLK_TREE_SIZE; k++) {
        if (efr32_clk_tree[k].parent == efr32_clk_tree[parent].parent) {
          parent_last = false;
          break;
        }
      }
      memmove(prefix + 4, prefix, sizeof(prefix) - 4);
      memcpy(prefix, parent_last ? "    " : "|   ", 4);
      parent = efr32_clk_tree[parent].parent;
      depth++;
    }

    if (efr32_clk_tree[i].parent < 0) {
      chprintf(xshp->stream, "%-14s %10u Hz", efr32_clk_tree[i].name,
               (unsigned)((efr32_clk_tree[i].pt < (halclkpt_t)CLK_ARRAY_SIZE)
                          ? halClockGetPointX(efr32_clk_tree[i].pt) : 0U));
      {
        char info[24];
        node_info(i, info, sizeof(info));
        if (info[0] != '\0') {
          chprintf(xshp->stream, "  (%s)", info);
        }
      }
      chprintf(xshp->stream, XSHELL_NEWLINE_STR);
    }
    else {
      print_node(xshp, i, prefix, last);
    }
  }
}

#else
/* Non-FG23 ports: "clocks w" falls back to the flat list. */
static void clocks_tree(xshell_t *xshp) {

  chprintf(xshp->stream, "clock tree not defined for this chip, flat list:" XSHELL_NEWLINE_STR);
  clocks_flat(xshp);
}
#endif /* defined(EFR32FG23X) */

/*===========================================================================*/
/* Commands.                                                                 */
/*===========================================================================*/

/**
 * @brief   "clocks" command: list clock points, "clocks w" for the tree.
 */
void cmd_clocks(xshell_t *xshp, int argc, char *argv[], char *envp[]) {

  (void)envp;

  if (argc == 2 && strcmp(argv[1], "w") == 0) {
    clocks_tree(xshp);
    return;
  }

  if (argc != 1) {
    xshellUsage(xshp, "clocks [w]");
    return;
  }

  clocks_flat(xshp);
}

#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
/**
 * @brief   "clock" command: switch clock configuration at run-time.
 */
void cmd_clock(xshell_t *xshp, int argc, char *argv[], char *envp[]) {
  bool result;
  const halclkcfg_t *ccp;

  (void)envp;

  if (argc != 2) {
    xshellUsage(xshp, "clock reset|default");
    return;
  }

  if (strcmp(argv[1], "reset") == 0) {
    chprintf(xshp->stream, XSHELL_NEWLINE_STR "Switching to post-reset clock settings: ");
    ccp = &hal_clkcfg_reset;
  }
  else if (strcmp(argv[1], "default") == 0) {
    chprintf(xshp->stream, XSHELL_NEWLINE_STR "Switching to default mcuconf.h clock settings: ");
    ccp = &hal_clkcfg_default;
  }
  else {
    xshellUsage(xshp, "clock reset|default");
    return;
  }

  /* Time for the serial TX buffer to flush. */
  chThdSleepMilliseconds(10);

  /* Switching clocks. */
  result = halClockSwitchMode(ccp);

  /* Printing result. */
  if (result) {
    chprintf(xshp->stream, "failed" XSHELL_NEWLINE_STR);
  }
  else {
    chprintf(xshp->stream, "done" XSHELL_NEWLINE_STR);
  }
}
#endif /* defined(HAL_LLD_USE_CLOCK_MANAGEMENT) */

/** @} */
