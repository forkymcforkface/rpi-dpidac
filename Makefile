ifneq (${KERNELRELEASE},)
	obj-m += rpi-dpidac.o
else
	KERNELDIR        ?= /lib/modules/$(shell uname -r)/build
	MODULE_DIR       ?= $(shell pwd)
	ARCH             ?= $(shell uname -i)
	CROSS_COMPILE    ?=
	INSTALL_MOD_PATH ?= /
	CONFIG_FILE      ?= /boot/firmware/config.txt
	MODULES_CONF     ?= /etc/modules-load.d/modules.conf
	OVERLAYS_DIR     ?= /boot/firmware/overlays
endif

all: modules

modules:
	${MAKE} ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" -C ${KERNELDIR} M="${MODULE_DIR}" modules

modules_install:
	${MAKE} ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" INSTALL_MOD_PATH="${INSTALL_MOD_PATH}" -C ${KERNELDIR} M="${MODULE_DIR}" modules_install
	@echo "dtoverlay=vc4-vga666,mode6" >> ${CONFIG_FILE}
	@echo "dtoverlay=audremap,pins_18_19" >> ${CONFIG_FILE}
	@grep -qxF 'i2c-dev' ${MODULES_CONF} || echo 'i2c-dev' >> ${MODULES_CONF}
	@grep -qxF 'rpi-dpidac' ${MODULES_CONF} || echo 'rpi-dpidac' >> ${MODULES_CONF}
	@cp vc4-vga666.dtbo ${OVERLAYS_DIR}

clean:
	rm -f *.o *.ko *.mod.c .*.o .*.ko .*.mod.c *.mod .*.cmd *~
	rm -f Module.symvers Module.markers modules.order
	rm -rf .tmp_versions

uninstall:
	rm -f ${INSTALL_MOD_PATH}/lib/modules/$(shell uname -r)/kernel/drivers/${MODULE_DIR}/rpi-dpidac.ko
	depmod -a
	sed -i 's/^dtoverlay=vc4-vga666,mode6/#dtoverlay=vc4-vga666,mode6/' ${CONFIG_FILE}
	sed -i 's/^dtoverlay=audremap,pins_18_19/#dtoverlay=audremap,pins_18_19/' ${CONFIG_FILE}
	sed -i '/^i2c-dev$/d' ${MODULES_CONF}
	sed -i '/^rpi-dpidac$/d' ${MODULES_CONF}
	rm -f ${OVERLAYS_DIR}/vc4-vga666.dtbo
