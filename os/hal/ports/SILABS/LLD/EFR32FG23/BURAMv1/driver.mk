ifeq ($(USE_SMART_BUILD),yes)
ifneq ($(findstring HAL_USE_BURAM TRUE,$(HALCONF)),)
PLATFORMSRC_CONTRIB += ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/BURAMv1/hal_buram_lld.c
endif
else
PLATFORMSRC_CONTRIB += ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/BURAMv1/hal_buram_lld.c
endif

PLATFORMINC_CONTRIB += ${CHIBIOS_CONTRIB}/os/hal/ports/SILABS/LLD/EFR32FG23/BURAMv1