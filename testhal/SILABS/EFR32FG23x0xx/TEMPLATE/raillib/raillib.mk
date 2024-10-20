# Required platform files.
GECKO_SDK = $(CHIBIOS_CONTRIB)/ext/gecko_sdk

# Below are RAIL lib binding for ChibiOS.
RAILLIBSRC = \
            raillib/raillib_chibios.c

# Below are imports from Radio Configurator
RAILLIBSRC += raillib/autogen/rail_config.c
RAILLIBSRC += raillib/autogen/sl_rail_util_callbacks.c
#RAILLIBSRC += raillib/autogen/sl_rail_util_init.c

# Below are imports from Gecko SDK
RAILLIBSRC += $(GECKO_SDK)/platform/radio/rail_lib/plugin/pa-conversions/pa_conversions_efr32.c
RAILLIBSRC += $(GECKO_SDK)/platform/radio/rail_lib/plugin/rail_util_protocol/sl_rail_util_protocol.c

RAILLIBINC = \
        raillib/autogen \
        raillib/config

RAILLIBINC += \
        $(GECKO_SDK)/platform/radio/rail_lib/plugin/pa-conversions \
        $(GECKO_SDK)/platform/radio/rail_lib/plugin/rail_util_callbacks \
        $(GECKO_SDK)/platform/radio/rail_lib/plugin/rail_util_callbacks/config \
        $(GECKO_SDK)/platform/radio/rail_lib/plugin/rail_util_protocol \
        $(GECKO_SDK)/platform/radio/rail_lib/plugin/rail_util_protocol/config \
        $(GECKO_SDK)/platform/radio/rail_lib/protocol/ble \
        $(GECKO_SDK)/platform/radio/rail_lib/protocol/ieee802154 \
        $(GECKO_SDK)/platform/radio/rail_lib/protocol/sidewalk \
        $(GECKO_SDK)/platform/radio/rail_lib/protocol/wmbus \
        $(GECKO_SDK)/platform/radio/rail_lib/protocol/zwave

# Shared variables
ALLCSRC += $(RAILLIBSRC)
ALLINC  += $(RAILLIBINC)
