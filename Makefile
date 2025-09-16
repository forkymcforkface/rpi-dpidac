ifneq (${KERNELRELEASE},)
	obj-m += rpi-dpidac.o
else
	KERNELDIR        ?= /lib/modules/$(shell uname -r)/build
	MODULE_DIR       ?= $(shell pwd)
	ifeq ($(shell dpkg-architecture -qDEB_HOST_ARCH),arm64)
		ARCH         ?= arm64
	else
		ARCH         ?= arm
	endif
	INSTALL_MOD_PATH ?=
endif

all:
	${MAKE} ARCH="${ARCH}" -C ${KERNELDIR} M="${MODULE_DIR}" modules
	dtc -@ -O dtb -o vc4-kms-dpi-custom.dtbo vc4-kms-dpi-custom.dts

install:
	${MAKE} ARCH="${ARCH}" INSTALL_MOD_PATH="${INSTALL_MOD_PATH}" -C ${KERNELDIR} M="${MODULE_DIR}" modules_install
	depmod
	@if [ -d /boot/firmware/overlays ]; then \
		cp vc4-kms-dpi-custom.dtbo /boot/firmware/overlays; \
	fi
	@if ! grep -q '^dtoverlay=vc4-kms-dpi-custom' /boot/firmware/config.txt 2>/dev/null; then \
		echo "dtoverlay=vc4-kms-dpi-custom" | sudo tee -a /boot/firmware/config.txt >/dev/null; \
	fi
	@if ! grep -q '^rpi-dpidac' /etc/modules-load.d/modules.conf 2>/dev/null; then \
		echo "rpi-dpidac" | sudo tee -a /etc/modules-load.d/modules.conf >/dev/null; \
	fi
	-@modprobe rpi-dpidac || true

uninstall:
	-@find ${INSTALL_MOD_PATH}/lib/modules/$(shell uname -r) -name 'rpi-dpidac.ko*' -delete || true
	depmod
	@if [ -f /boot/firmware/overlays/vc4-kms-dpi-custom.dtbo ]; then \
		echo "rm /boot/firmware/overlays/vc4-kms-dpi-custom.dtbo"; \
		rm /boot/firmware/overlays/vc4-kms-dpi-custom.dtbo; \
	fi

clean:
	${MAKE} -C ${KERNELDIR} M="${MODULE_DIR}" clean
	@if [ -f vc4-kms-dpi-custom.dtbo ]; then \
		echo "rm vc4-kms-dpi-custom.dtbo"; \
		rm vc4-kms-dpi-custom.dtbo; \
	fi
