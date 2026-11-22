# Optional system tick driver: ARM Cortex-M core SysTick.
# NOT included by any platform.mk by default — select with
#   make EFR32_USE_SYSTICKV1=yes
# (see EFR32FG23/platform.mk). Supports both OSAL_ST_MODE_PERIODIC and
# OSAL_ST_MODE_FREERUNNING. Stops in EM2/EM3 (clocked from HCLK).

PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/SYSTICKv1/hal_st_lld.c

PLATFORMINC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/SYSTICKv1
