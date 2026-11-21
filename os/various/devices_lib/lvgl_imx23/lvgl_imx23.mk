# LVGL port layer for the i.MX23 LCDIF + PXP (devices_lib/lvgl_imx23).
LVGLIMX23SRC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/lvgl_imx23/imx23_lvgl.c
LVGLIMX23INC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/lvgl_imx23

ALLCSRC += $(LVGLIMX23SRC)
ALLINC += $(LVGLIMX23INC)
