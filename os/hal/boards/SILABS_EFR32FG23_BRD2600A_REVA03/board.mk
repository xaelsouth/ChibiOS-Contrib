# List of all the board related files.
BOARDSRC = ${CHIBIOS_CONTRIB}/os/hal/boards/SILABS_EFR32FG23_BRD2600A_REVA03/board.c

# Required include directories
BOARDINC = ${CHIBIOS_CONTRIB}/os/hal/boards/SILABS_EFR32FG23_BRD2600A_REVA03

# Shared variables
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)
