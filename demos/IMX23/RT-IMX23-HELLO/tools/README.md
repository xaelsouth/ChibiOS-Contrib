# Host tools for the i.MX23 bootstream

Prebuilt x86-64 Linux binaries from imx-kobs-ng (kobs-ng):

    elftosb    ELF/binary -> .sb bootstream compiler (C++, needs libstdc++;
               the Makefile invokes it with `env -u LD_LIBRARY_PATH` so a
               foreign libstdc++ on LD_LIBRARY_PATH cannot break it)
    sb_loader  pushes an .sb to the i.MX23 ROM over USB HID recovery mode
               (`sb_loader -d -p hid file.sb`)

Source: https://github.com/NXPmicro/imx-kobs (kobs-ng). Rebuild with the
system g++ if the binaries don't run on your host.
