# RAIL glue driver (emlib shims + radio IRQ wrappers).
# Always compiled; content guarded by EFR32_RAIL_USE_RAIL in hal_rail_lld.c.
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/RAILv1/hal_rail_lld.c

PLATFORMINC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/RAILv1
