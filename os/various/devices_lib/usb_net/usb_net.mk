# USB device networking (RNDIS gadget + lwIP glue) files.
USBNETCSRC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/usb_net/usb_rndis.c \
             $(CHIBIOS_CONTRIB)/os/various/devices_lib/usb_net/usb_net_lwip.c
USBNETINC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/usb_net

ALLCSRC += $(USBNETCSRC)
ALLINC += $(USBNETINC)
