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

Hardware note (DK2600A, EFR32FG23B010F512IM48): with EM4GRPACLKCTRL=LFXO
and CFG CNTPRESC=DIV1, the BURTC CNT measures only ~8 Hz (not 32768 Hz).
The same slow rate is observed with the existing BURTCv1 RTC driver, so
this is a chip-level EM4 clock path question (needs reference-manual
follow-up), not specific to this driver.  The driver structure follows
emlib BURTC_Init: CFG is only writable while EN=0 (writing it enabled
bus-faults); COMP/CNT are async registers and must be written while
EN=1 with SYNCBUSY waits.
