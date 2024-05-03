# System requirements

Both the overlay and the bridge were tested in the following environment:
* Raspberry Pi OS Lite
* Release date: October 10th 2023
* Last updated: March 15th 2024
* System: 64-bit
* Kernel version: 6.6
* Debian version: 12 (bookworm)
* Raspberry Pi: models 3B/B+, 4B and 5B

# Build instructions

1. Clone Repo
```git clone https://github.com/forkymcforkface/rpi-dpidac```
2. Install Kernel Headers
```apt install raspberrypi-kernel-headers```
3. Compile the overlay and kernel module
```make```
4. Install the overlay and kernel module
```make install```

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
