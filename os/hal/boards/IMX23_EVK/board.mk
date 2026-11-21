# List of all the board related files.
BOARDSRC = ${CHIBIOS_CONTRIB}/os/hal/boards/IMX23_EVK/board.c

# Required include directories
BOARDINC = ${CHIBIOS_CONTRIB}/os/hal/boards/IMX23_EVK

# Shared variables
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)
