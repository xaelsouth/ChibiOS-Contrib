SYSTICKv4 — BURTC as the ChibiOS system tick (EFR32FG23)
=========================================================

Alternative to the default SYSTICKv3 (LETIMER0, 24-bit, EM2-capable):

  - BURTC CNT is a full 32-bit up-counter -> no software extension needed
    (contrast SYSTICKv3, which extends the 24-bit LETIMER via the UF IRQ)
  - BURTC runs from EM4GRPACLK and keeps counting in EM4 (deepest sleep)
  - single compare channel (COMP) used as the kernel tick alarm

Select with EFR32_USE_SYSTICKV4=yes in the project Makefile (platform.mk
then includes SYSTICKv4/driver.mk instead of SYSTICKv3/driver.mk).
FREERUNNING only (OSAL_ST_MODE_PERIODIC is not supported).

BURTC is shared with the BURTCv1 RTC calendar driver: SYSTICKv4 owns the
BURTC interrupt vector (EFR32_ST_OWNS_BURTC) and forwards RTC events to
rtc_lld_serve_interrupt(); BURTCv1 must not define its own handler then.
CNT is the shared time base for both the systick and the calendar, so
rtcSetTime() under EFR32_ST_OWNS_BURTC does NOT clear CNT (that would yank
the systick to 0) — it rebases the BURAM calendar anchor instead
(_rtc_lld_store_time). Only the single COMP channel is truly shared
between the kernel alarm and the RTC alarm.

Hardware note (DK2600A, EFR32FG23B010F512IM48), RESOLVED: an earlier
observation of "CNT measures only ~8 Hz" was a measurement artifact —
with CFG.DEBUGRUN=0 (reset default) the BURTC halts whenever the debugger
halts the CPU (RM 16.3.3), so CNT deltas taken across debugger halts are
bogus. Measured properly (CPU running, or DEBUGRUN=1 even while halted)
with EM4GRPACLKCTRL=LFXO and CFG CNTPRESC=DIV1, CNT advances at the full
32768 Hz (verified: 61491 counts over 2 s; 65553 over 2 s with DEBUGRUN=1
while halted). BURTC_PRECNT is not involved at DIV1: the 15-bit pre-counter
only runs when CNTPRESC>0, dividing BURTCCLK by 2^CNTPRESC for CNT (RM
16.3.4). Both SYSTICKv4 and BURTCv1 now set DEBUGRUN in CFG so debugger
measurements are valid. The driver structure follows
emlib BURTC_Init: CFG is only writable while EN=0 (writing it enabled
bus-faults); COMP/CNT are async registers and must be written while
EN=1 with SYNCBUSY waits.
