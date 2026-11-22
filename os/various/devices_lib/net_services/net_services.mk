# Network services (telnetd, ...) files.
# net_shell.c (lwIP shell commands) lives in os/various/cli, it is CLI not
# a service.
NETSVCSRC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/net_services/telnetd.c \
            $(CHIBIOS_CONTRIB)/os/various/devices_lib/net_services/tftpd_lfs.c \
            $(CHIBIOS_CONTRIB)/os/various/cli/net_shell.c \
            $(CHIBIOS_CONTRIB)/os/various/devices_lib/net_services/sio_sio.c
NETSVCINC = $(CHIBIOS_CONTRIB)/os/various/devices_lib/net_services \
            $(CHIBIOS_CONTRIB)/os/various/cli

ALLCSRC += $(NETSVCSRC)
ALLINC += $(NETSVCINC)
