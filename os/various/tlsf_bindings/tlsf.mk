# TLSF constant-time allocator (ext/tlsf) + bindings.
# NDEBUG disables tlsf.c's <assert.h> checks (ChibiOS uses its own
# osalDbgAssert, which is unaffected by NDEBUG).
TLSFSRC = $(CHIBIOS_CONTRIB)/ext/tlsf/tlsf.c \
          $(CHIBIOS_CONTRIB)/os/various/tlsf_bindings/tlsf_bindings.c

TLSFINC = $(CHIBIOS_CONTRIB)/ext/tlsf \
          $(CHIBIOS_CONTRIB)/os/various/tlsf_bindings

DDEFS += -DNDEBUG

ALLCSRC += $(TLSFSRC)
ALLINC  += $(TLSFINC)
