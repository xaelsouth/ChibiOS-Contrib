# EFR32FG23 SYSRTC-based system tick (SYSTICKv2) — the platform default.
# 32-bit counter, LF clock, runs in EM2/EM3/EM4. LETIMER0 stays free for
# LETIMERv1 (GPT). Core SysTick (SYSTICKv1) can be selected instead with
# EFR32_USE_SYSTICKV1=yes.
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/SYSTICKv2/hal_st_lld.c
PLATFORMINC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/SYSTICKv2
