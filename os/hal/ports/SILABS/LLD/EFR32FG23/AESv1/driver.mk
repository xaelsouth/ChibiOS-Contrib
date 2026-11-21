# EFR32FG23 RADIOAES crypto (AES) glue driver.
# Always listed; the C source is self-guarded by EFR32_CRYPTO_USE_AES1
# (mcuconf.h) and HAL_USE_CRY (halconf.h).
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/AESv1/hal_crypto_lld.c

PLATFORMINC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/AESv1
