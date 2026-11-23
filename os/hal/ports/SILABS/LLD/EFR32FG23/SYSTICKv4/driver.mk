# EFR32FG23 BURTC-based system tick (SYSTICKv4).
# Alternative to SYSTICKv3 (LETIMER0): 32-bit counter, EM4-capable.
# Shares BURTC with BURTCv1 (RTC calendar driver): SYSTICKv4 owns the
# BURTC interrupt vector and forwards RTC events to rtc_lld_serve_interrupt().
# Include this INSTEAD of SYSTICKv3/driver.mk in platform.mk
# (select with EFR32_USE_SYSTICKV4=yes).
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/SYSTICKv4/hal_st_lld.c
PLATFORMINC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/SYSTICKv4
