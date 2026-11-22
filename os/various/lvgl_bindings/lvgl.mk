# LVGL (ext/lvgl) graphics library files + ChibiOS bindings.
#
# lv_conf.h lives in this directory; LVGL's lv_conf_internal.h picks it up
# via __has_include("lv_conf.h") because this directory is on ALLINC.

LVGLDIR = $(CHIBIOS_CONTRIB)/ext/lvgl

# Library sources: root + the subtrees needed for core/SW-draw/widgets UI.
# (debugging, drivers, fs, image, libs are left out: their files self-guard
# on lv_conf.h options that are all 0 here, and skipping them keeps the
# build fast. Add them back if you enable the corresponding LV_USE_* .)
LVGLSRC_DIRS = $(LVGLDIR)/src/core \
               $(LVGLDIR)/src/display \
               $(LVGLDIR)/src/draw \
               $(LVGLDIR)/src/font \
               $(LVGLDIR)/src/fs \
               $(LVGLDIR)/src/image \
               $(LVGLDIR)/src/indev \
               $(LVGLDIR)/src/layouts \
               $(LVGLDIR)/src/misc \
               $(LVGLDIR)/src/osal \
               $(LVGLDIR)/src/stdlib \
               $(LVGLDIR)/src/themes \
               $(LVGLDIR)/src/tick \
               $(LVGLDIR)/src/widgets \
               $(LVGLDIR)/src/others \
               $(LVGLDIR)/src/logging
LVGLCSRC := $(wildcard $(LVGLDIR)/src/*.c) \
           $(foreach d,$(LVGLSRC_DIRS),$(shell find $(d) -name '*.c'))

LVGLBINDSRC = $(CHIBIOS_CONTRIB)/os/various/lvgl_bindings/lv_stdlib_tlsf.c

ALLCSRC += $(LVGLCSRC) $(LVGLBINDSRC)
ALLINC  += $(LVGLDIR) $(LVGLDIR)/include $(LVGLDIR)/src \
           $(CHIBIOS_CONTRIB)/os/various/lvgl_bindings
