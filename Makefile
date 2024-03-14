ifneq (${KERNELRELEASE},)
	obj-m += rpi-dpidac.o
else
	KERNELDIR        ?= /lib/modules/$(shell uname -r)/build
	MODULE_DIR       ?= $(shell pwd)
	ARCH             ?= arm64
	INSTALL_MOD_PATH ?=
endif

all:
	${MAKE} ARCH="${ARCH}" -C ${KERNELDIR} M="${MODULE_DIR}" modules
	dtc -@ -O dtb -o vc4-kms-dpi-custom.dtbo vc4-kms-dpi-custom.dts

install:
	${MAKE} ARCH="${ARCH}" INSTALL_MOD_PATH="${INSTALL_MOD_PATH}" -C ${KERNELDIR} M="${MODULE_DIR}" modules_install
	depmod
	cp vc4-kms-dpi-custom.dtbo /boot/overlays/firmware
	@if [ -f timings.txt ]; then \
		echo "cp timings.txt /boot"; \
		cp timings.txt /boot; \
	fi
	
uninstall:
	rm -f ${INSTALL_MOD_PATH}/lib/modules/$(shell uname -r)/extra/rpi-dpidac.ko*
	depmod
	@if [ -f /boot/overlays/firmware/vc4-kms-dpi-custom.dtbo ]; then \
		echo "rm /boot/overlays/firmware/vc4-kms-dpi-custom.dtbo"; \
		rm /boot/overlays/firmware/vc4-kms-dpi-custom.dtbo; \
	fi
	@if [ -f /boot/firmware/timings.txt ]; then \
		echo "rm /boot/firmware/timings.txt"; \
		rm /boot/firmware/timings.txt; \
	fi

clean:
	${MAKE} -C ${KERNELDIR} M="${MODULE_DIR}" clean
	@if [ -f vc4-kms-dpi-custom.dtbo ]; then \
		echo "rm vc4-kms-dpi-custom.dtbo"; \
		rm vc4-kms-dpi-custom.dtbo; \
	fi

