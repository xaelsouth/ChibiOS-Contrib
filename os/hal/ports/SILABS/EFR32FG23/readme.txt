EFR32FG23 port — mcuconf.h contract and update rule
====================================================

RULE: every mcuconf.h that uses this port MUST be kept in sync with
hal_lld.h. Whenever hal_lld.h adds, renames or tightens a configuration
macro or a compile-time #error check (clock sources, prescalers, voltage
scaling, flash wait states, SYSTICK selection, ...), update ALL mcuconf.h
files under:

    ChibiOS-Contrib/demos/SILABS/*/cfg/mcuconf.h
    ChibiOS-Contrib/testhal/SILABS/*/*/mcuconf.h
    application/*/cfg/mcuconf.h        (projects using this port)

and verify by building every one of those projects (see "How to check"
below). A new check that fires in only one project is a sign the others
were not updated.

Why: hal_lld.h derives everything (EFR32_SYSCLK, EFR32_HCLK, EFR32_PCLK,
EFR32_HFRCOEM23CLK, EFR32_MSC_READCTRL_MODE, EFR32_EM01/23_VSCALE) from the
mcuconf.h defines and validates them with #error at compile time. The
checks only work if every project carries the required defines.

Current contract (hal_lld.h), all enforced at compile time:
  - EFR32_EM01VSCALE_SEL / EFR32_EM23VSCALE_SEL: voltage scale for EM0/EM1
    and EM2/EM3 (EFR32_EM01/23_VSCALESEL_0V9/1V0/1V1 = 0.9/1.0/1.1 V).
    EM01 must be >= EM23. HCLK <= 80 MHz always; HCLK > 40 MHz requires
    EM01 = 1V1; if HFRCOEM23 is the EM2/EM3 HF clock and
    EFR32_HFRCOEM23CLK > 40 MHz, EM23 must be 1V1.
  - EFR32_SYSCLKSEL + EFR32_HPRE/EFR32_PPRE/EFR32_RHPRE + EFR32_HFRCOPRE:
    SYSCLK source and prescalers. EFR32_HCLK = SYSCLK / HPRE.
  - EFR32_HFRCODPLL_FREQ: optional override of the 19 MHz default PLL
    frequency (guarded by #ifndef in hal_lld.h — set it in mcuconf.h to
    change the clock).
  - Flash wait states are NOT set manually: EFR32_MSC_READCTRL_MODE is
    derived from EFR32_HCLK (WS0 <= 40 MHz, WS1 <= 80 MHz, WS2 safe) and
    programmed into MSC->READCTRL by efr32_set_flash_wait_states() at boot.
    #error if WS0 is selected above 40 MHz. Override only via
    EFR32_MSC_READCTRL_MODE if you really know better.
  - EFR32_HFRCOEM23_ENABLED: whether HFRCOEM23 may keep HCLK alive in
    EM2/EM3.
  - EFR32_ST_USE_TIMER / EFR32_USE_SYSTICKVx: system tick source
    (SYSTICKv1 = core SysTick, v2 = SYSRTC, v3 = LETIMER0 default;
    v1/v2 selected per-test in the Makefile, not in mcuconf.h).

How to check all projects at once:

    cd <repo>/src
    for d in $(find ChibiOS-Contrib/demos/SILABS ChibiOS-Contrib/testhal/SILABS \
                    application -name Makefile | xargs -n1 dirname | sort -u); do
      make -C "$d" -j8 > /tmp/bo.log 2>&1 && echo "OK   $d" || { echo "FAIL $d"; grep -E 'error:|#error' /tmp/bo.log | head -3; }
    done

(Latest audit: 35/35 build OK. Previously broken demos fixed:
RT-EFR32FG23B010F512-DK2600A — chconf.h CH_CFG_SAFETY_CHECK_HOOK, halconf.h
bumped to VER_9_1, Makefile uses contrib hal.mk + streams.mk;
RT-EFR32FG14P-SLWSTK6061B — startup mk gecko SDK paths, halconf VER_9_1,
chconf hook, FG14P port de-emlib'ed (raw CMU registers, escape hatch).)

Note: application/ trees are not git repositories; keep their cfg/mcuconf.h
in sync manually when the port contract changes.

Rules for port/LLD code (SILABS):
  - NO emlib in os/hal/ports/SILABS (EFR32FG14P, EFR32FG23, LLD/*): raw
    register access only, using the device headers from
    ext/gecko_sdk/platform/Device/SiliconLabs/<family>/Include and the
    system_efr32*.c from .../Source (CMSIS-style device support, allowed).
    The only exception is LLD/EFR32FG23/RAILv1, which wraps the Gecko RAIL
    library (sl_rail_*), not emlib.
  - When adding a new LLD driver under LLD/EFR32FG23/, add the matching
    EFR32_HAS_* / capability macros to EFR32FG23/efr32_registry.h so the
    port documents the real hardware (counts per ext/gecko_sdk device
    header *_COUNT macros) and drivers can #if on them.
