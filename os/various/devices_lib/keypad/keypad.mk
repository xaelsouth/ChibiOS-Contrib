# Generic matrix keypad device driver (devices_lib/keypad).
KEYPADSRC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/keypad/keypad.c
KEYPADINC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/keypad

ALLCSRC += $(KEYPADSRC)
ALLINC += $(KEYPADINC)
