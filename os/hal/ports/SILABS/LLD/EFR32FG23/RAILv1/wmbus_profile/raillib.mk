# wMBus radio profile for EFR32FG23 — pre-generated RAIL channel configs.
# Include this from your app Makefile when using RAILv1 + wMBus:
#   include $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/RAILv1/wmbus_profile/raillib.mk
#
# NOTE: raillib_chibios.c is NOT compiled here — IRQ handlers and emlib shims
# are provided by RAILv1 (hal_rail_lld.c). This profile only provides the
# radio-configurator autogen files + RAIL plugin sources.

GECKO_SDK = $(CHIBIOS_CONTRIB)/ext/gecko_sdk
WMBUS_PROFILE = $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/RAILv1/wmbus_profile

# Autogen files from Radio Configurator (wMBus T/C/S modes)
WMBUSPROFSRC =        $(WMBUS_PROFILE)/autogen/rail_config.c        $(WMBUS_PROFILE)/autogen/sl_rail_util_callbacks.c

# RAIL plugin sources from Gecko SDK
WMBUSPROFSRC += $(GECKO_SDK)/platform/radio/rail_lib/plugin/pa-conversions/pa_conversions_efr32.c
WMBUSPROFSRC += $(GECKO_SDK)/platform/radio/rail_lib/plugin/rail_util_protocol/sl_rail_util_protocol.c

WMBUSPROFINC =        $(WMBUS_PROFILE)/autogen        $(WMBUS_PROFILE)/config        $(GECKO_SDK)/platform/radio/rail_lib/plugin/pa-conversions        $(GECKO_SDK)/platform/radio/rail_lib/plugin/pa-conversions/efr32xg23/config        $(GECKO_SDK)/platform/radio/rail_lib/plugin/rail_util_callbacks        $(GECKO_SDK)/platform/radio/rail_lib/plugin/rail_util_callbacks/config        $(GECKO_SDK)/platform/radio/rail_lib/plugin/rail_util_protocol        $(GECKO_SDK)/platform/radio/rail_lib/plugin/rail_util_protocol/config        $(GECKO_SDK)/platform/radio/rail_lib/protocol/ble        $(GECKO_SDK)/platform/radio/rail_lib/protocol/ieee802154        $(GECKO_SDK)/platform/radio/rail_lib/protocol/sidewalk        $(GECKO_SDK)/platform/radio/rail_lib/protocol/wmbus        $(GECKO_SDK)/platform/radio/rail_lib/protocol/zwave

# Shared variables — append to app's CSRC/INCDIR
ALLCSRC += $(WMBUSPROFSRC)
ALLINC  += $(WMBUSPROFINC)
