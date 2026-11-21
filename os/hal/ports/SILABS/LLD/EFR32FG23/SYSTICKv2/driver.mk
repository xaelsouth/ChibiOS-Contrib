# EFR32FG23 SYSRTC-based system tick (SYSTICKv2).
# Alternative to SYSTICKv3 (LETIMER0): 32-bit counter, EM4-capable.
# Include this INSTEAD of SYSTICKv3/driver.mk in platform.mk.
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/SYSTICKv2/hal_st_lld.c
PLATFORMINC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/SYSTICKv2
