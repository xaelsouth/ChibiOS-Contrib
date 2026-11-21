# USB device networking (RNDIS gadget) files.
USBNETCSRC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/usb_net/usb_rndis.c
USBNETINC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/usb_net

ALLCSRC += $(USBNETCSRC)
ALLINC += $(USBNETINC)
