# SIGROUTEv1: generic hal_sigroute driver implemented on the
# Silicon Labs PRS (Peripheral Reflex System) peripheral.
ifeq ($(USE_SMART_BUILD),yes)

ifneq ($(findstring HAL_USE_SIGROUTE TRUE,$(HALCONF)),)
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/SIGROUTEv1/hal_sigroute_lld.c
endif

else
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/SIGROUTEv1/hal_sigroute_lld.c
endif

PLATFORMINC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/SIGROUTEv1
