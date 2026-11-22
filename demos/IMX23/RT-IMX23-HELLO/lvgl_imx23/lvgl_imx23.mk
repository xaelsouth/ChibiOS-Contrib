# LVGL port layer for the i.MX23 LCDIF + PXP (demo-local, project-relative
# paths; this .mk is included from the demo Makefile).
LVGLIMX23SRC = lvgl_imx23/imx23_lvgl.c
LVGLIMX23INC = lvgl_imx23

ALLCSRC += $(LVGLIMX23SRC)
ALLINC += $(LVGLIMX23INC)
