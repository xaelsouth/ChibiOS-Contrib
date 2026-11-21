# List of all the board related files.
BOARDSRC = ${CHIBIOS_CONTRIB}/os/hal/boards/IMX23_OLIMEX_IMX233/board.c

# Required include directories
BOARDINC = ${CHIBIOS_CONTRIB}/os/hal/boards/IMX23_OLIMEX_IMX233

# Shared variables
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)
