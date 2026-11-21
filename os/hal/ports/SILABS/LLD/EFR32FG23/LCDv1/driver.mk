# LCD segment controller driver.
# Always compiled; content guarded by EFR32_LCD_USE_LCD in hal_lcd_lld.c.
PLATFORMSRC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/LCDv1/hal_lcd_lld.c

PLATFORMINC_CONTRIB += $(CHIBIOS_CONTRIB)/os/hal/ports/SILABS/LLD/EFR32FG23/LCDv1
