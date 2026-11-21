# Network services (telnetd, ...) files.
NETSVCSRC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/net_services/telnetd.c \
            $(CHIBIOS_CONTRIB)/os/various/devices_lib/net_services/tftpd_lfs.c \
            $(CHIBIOS_CONTRIB)/os/various/devices_lib/net_services/httpsd.c
NETSVCINC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/net_services

ALLCSRC += $(NETSVCSRC)
ALLINC += $(NETSVCINC)
