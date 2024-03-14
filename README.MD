# System requirements

Both the overlay and the bridge were tested in the following environment:
* Raspberry Pi OS Lite
* Release date: October 10th 2023
* System: 64-bit
* Kernel version: 6.1
* Debian version: 12 (bookworm)
* Raspberry Pi: models 3B/B+, 4B and 5B

# Build instructions

1. Install Kernel Headers
```apt install raspberrypi-kernel-headers```
2. Compile the overlay and kernel module
```make```
3. Install the overlay and kernel module
```make install```
4. Manually add the overlay to /boot/config.txt
```dtoverlay=vc4-kms-dpi-custom```
5. Manually add the kernel module to /etc/modules-load.d/modules.conf
```rpi-dpidac```
6. Optionally you can modify /boot/firmware/timings.txt file with your custom timings

## Misc

* Check if kernel module is loaded
```lsmod | grep rpi_dpidac```
* Display module info
```modinfo rpi-dpidac```
* Load module manually
```modprobe rpi-dpidac```
or
```insmod /usr/lib/modules/$(uname -r)/extra/rpi-dpidac.ko.xz```
* Unload module manually
```modprobe -r rpi-dpidac```
* Get kernel bridge message status
```less /var/log/messages | grep RPI-DPIDAC```
or
```dmesg | grep RPI-DPIDAC```
