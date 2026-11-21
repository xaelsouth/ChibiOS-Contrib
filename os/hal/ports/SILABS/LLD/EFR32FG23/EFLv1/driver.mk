# Enable with EFR32_EFL_USE_EFL1=TRUE (halconf_community.h) and HAL_USE_EFL=TRUE (halconf.h).
ifeq ($(USE_SMART_BUILD),yes)
ifneq ($(findstring EFR32_EFL_USE_EFL1 TRUE,$(HALCONF)),)
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/EFLv1/hal_efl_lld.c
endif
else
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/EFLv1/hal_efl_lld.c
endif

PLATFORMINC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/EFLv1
