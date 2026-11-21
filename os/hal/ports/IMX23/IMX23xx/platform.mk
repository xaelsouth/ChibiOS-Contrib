# List of all the i.MX23 platform files.
PLATFORMSRC_CONTRIB := \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/hal_lld.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/icoll.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/hal_st_lld.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/hal_sio_lld.c \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/hal_pal_lld.c

PLATFORMASM_CONTRIB := \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx/icoll_tramp.S

# Required include directories.
PLATFORMINC_CONTRIB := \
  $(CHIBIOS_CONTRIB)/os/hal/ports/IMX23/IMX23xx

# Shared variables
ALLCSRC   += $(PLATFORMSRC_CONTRIB)
ALLXASMSRC += $(PLATFORMASM_CONTRIB)
ALLINC    += $(PLATFORMINC_CONTRIB)
