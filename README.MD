# Build instructions Raspbian 32-bit

## dtbo

`dtc -@ -O dtb -o vc4-vga666-overlay.dtbo vc4-vga666-overlay.dts`

## rpi-dipac

1. Install Kernel Headers
`sudo apt install raspberrypi-kernel-headers`
2. Compile drm bridge (do not use sudo)
`make -f Makefile.cross ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- KERNELDIR=/lib/modules/$(uname -r)/build`
3. Copy .ko file into system dir
`sudo cp rpi-dpidac.ko /usr/lib/modules/$(uname -r)/kernel/drivers/gpu/drm/bridge`
NOTE: default /boot/timings.txt path is not mounted when module tries to read the same so changed path to /home/pi/RGB-Pi/data/system.dat
4. Rebuild module dependency list
`sudo depmod`
5. Add rpi-dpidac module to /etc/modules-load.d/modules.conf or /etc/modules.conf
`rpi-dpidac`

## Misc

1. Check if kernel module is loaded
`lsmod | grep rpi_dpidac`
2. Display module info
`modinfo rpi-dpidac`
3. Load module manually
`modprobe rpi-dpidac`
or
`insmod /usr/lib/modules/$(uname -r)/kernel/drivers/gpu/drm/bridge/rpi-dpidac.ko`
4. Unload module manually
`modprobe -r rpi-dpidac`
5. Get kernel boot messages to check info
`sudo less /var/log/messages | grep RPI-DPIDAC`
or
`dmesg | grep RPI-DPIDAC`