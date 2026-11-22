# LVGL port layer for the i.MX23 LCDIF + PXP (demo-local (lvgl/), project-relative
# paths; this .mk is included from the demo Makefile).
LVGLPORTSRC = lvgl/imx23_lvgl.c
LVGLPORTINC = lvgl

ALLCSRC += $(LVGLPORTSRC)
ALLINC += $(LVGLPORTINC)
