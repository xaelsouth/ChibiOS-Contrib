# emlib binding driver for Silicon Labs EFR32 — provides emlib→ChibiOS shims.
# Enable with EFR32_EMLIB_BINDING=TRUE in mcuconf.h.
# NOTE: When EFR32_RAIL_USE_RAIL=TRUE, the RAILv1 driver already provides
# these same shims, so this should NOT be enabled simultaneously.

ifeq ($(EFR32_EMLIB_BINDING),TRUE)
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/common/emlib_binding/emlib_chibios.c
PLATFORMINC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/common/emlib_binding
endif
