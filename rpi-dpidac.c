/*
 * Copyright (C) 2018 Hugh Cole-Baker
 *
 * Hugh Cole-Baker <sigmaris@gmail.com>
 *  * cpasjuste
 * rTomas (RTA) <ruben.tomas.alonso@gmail.com>
 * Kevin *forkymcforkface
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>

#include <video/display_timing.h>
#include <video/of_display_timing.h>
#include <video/videomode.h>
#include <uapi/linux/media-bus-format.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_bridge.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_print.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_edid.h>

// ~ 20 timings line + comments
#define READ_SIZE_MAX 2048
#define LINE_SIZE_MAX 256

static char read_buf[READ_SIZE_MAX];
static const char *timings_path = "/opt/rgbpi/ui/data/timings.dat";

struct dpidac {
    struct drm_bridge bridge;
    struct drm_connector connector;
    struct display_timings *timings;
};

enum ModeIds {
  p320x240 = 0,
  p1920x240,
  i640x480,
  p640x480,
  ModeCount,
};

static struct videomode modes[ModeCount] = {

    // 240p@60 : 320 1 4 30 46 240 1 4 5 14 0 0 0 60 0 6400000 1
    {
        .pixelclock = 6400000,
        .hactive = 320,
        .hfront_porch = 4,
        .hsync_len = 30,
        .hback_porch = 46,
        .vactive = 240,
        .vfront_porch = 4,
        .vsync_len = 5,
        .vback_porch = 14,
        .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW
    },
    // 1920x240p@60 : 1920 1 80 184 312 240 1 1 3 16 0 0 0 60 0 38937600 1
    {
        .pixelclock = 38937600,
        .hactive = 1920,
        .hfront_porch = 80,
        .hsync_len = 184,
        .hback_porch = 312,
        .vactive = 240,
        .vfront_porch = 1,
        .vsync_len = 3,
        .vback_porch = 16,
        .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW
    },

    // 480i@60 : 640 1 24 64 104 480 1 3 6 34 0 0 0 60 1 13054080 1
    {
      .pixelclock = 13054080,
      .hactive = 640,
      .hfront_porch = 24,
      .hsync_len = 64,
      .hback_porch = 104,
      .vactive = 480,
      .vfront_porch = 3,
      .vsync_len = 6,
      .vback_porch = 34,
      .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW | DISPLAY_FLAGS_INTERLACED
    },
    // 480p@60 : 640 1 24 96 48 480 1 11 2 32 0 0 0 60 0 25452000 1
    {
      .pixelclock = 25452000,
      .hactive = 640,
      .hfront_porch = 24,
      .hsync_len = 96,
      .hback_porch = 48,
      .vactive = 480,
      .vfront_porch = 11,
      .vsync_len = 2,
      .vback_porch = 32,
      .flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW
    }
};

static struct drm_display_mode *dpidac_display_mode_from_timings(struct drm_connector *connector, const char *line) {
    int ret, hsync, vsync, interlace, ratio;
    struct drm_display_mode *mode = NULL;
    struct videomode vm;

    if (line != NULL) {
        memset(&vm, 0, sizeof(vm));
        ret = sscanf(line, "%d %d %d %d %d %d %d %d %d %d %*s %*s %*s %*s %d %ld %d",
                     &vm.hactive, &hsync, &vm.hfront_porch, &vm.hsync_len, &vm.hback_porch,
                     &vm.vactive, &vsync, &vm.vfront_porch, &vm.vsync_len, &vm.vback_porch,
                     &interlace, &vm.pixelclock, &ratio);
        if (ret != 13) {
            printk(KERN_WARNING "[RPI-DPIDAC]: malformed mode requested, skipping (%s)\n", line);
            return NULL;
        }

        // setup flags
        vm.flags = interlace ? DISPLAY_FLAGS_INTERLACED : 0;
        vm.flags |= hsync ? DISPLAY_FLAGS_HSYNC_LOW : DISPLAY_FLAGS_HSYNC_HIGH;
        vm.flags |= vsync ? DISPLAY_FLAGS_VSYNC_LOW : DISPLAY_FLAGS_VSYNC_HIGH;

        // create/init display mode, convert from video mode
        mode = drm_mode_create(connector->dev);
        if (mode == NULL) {
            printk(KERN_WARNING "[RPI-DPIDAC]: drm_mode_create failed, skipping (%s)\n", line);
            return NULL;
        }

        drm_display_mode_from_videomode(&vm, mode);

        return mode;
    }

    return NULL;
}

int dpidac_load_timings(struct drm_connector *connector) {
    struct file *fp = NULL;
    ssize_t read_size = 0;
    size_t cursor = 0;
    char line[LINE_SIZE_MAX];
    size_t line_start = 0;
    size_t line_len = 0;
    struct drm_display_mode *mode = NULL;
    int mode_count = 0;

    printk(KERN_INFO "[RPI-DPIDAC]: loading timings from file...\n");

    fp = filp_open(timings_path, O_RDONLY, 0);
    if (IS_ERR(fp) || !fp) {
        printk(KERN_WARNING "[RPI-DPIDAC]: timings file not found, skipping custom modes loading...\n");
        return 0;
    }

    read_size = kernel_read(fp, &read_buf, READ_SIZE_MAX, &fp->f_pos);
    if (read_size <= 0) {
        filp_close(fp, NULL);
        printk(KERN_WARNING "[RPI-DPIDAC]: empty timings file found, skipping custom modes loading...\n");
        return 0;
    }
    filp_close(fp, NULL);

    for (cursor = 0; cursor < read_size; cursor++) {
        line[cursor - line_start] = read_buf[cursor];
        line_len++;
        if (line_len >= LINE_SIZE_MAX || read_buf[cursor] == '\n' || read_buf[cursor] == '\0') {
            if (line_len > 32 && line[0] != '#') {
                line[line_len - 1] = '\0';
                if ((mode = dpidac_display_mode_from_timings(connector, line)) != NULL) {
                    mode->type = mode_count ? DRM_MODE_TYPE_DRIVER : DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
                    printk(KERN_INFO "[RPI-DPIDAC]: \t" DRM_MODE_FMT, DRM_MODE_ARG(mode));
                    drm_mode_probed_add(connector, mode);
                    mode_count++;
                }
            }
            line_start += line_len;
            line_len = 0;
            memset(line, 0, 128);
        }
    }

    printk(KERN_INFO "[RPI-DPIDAC]: %i custom modes loaded\n", mode_count);

    return mode_count;
}

static inline struct dpidac *drm_bridge_to_dpidac(struct drm_bridge *bridge) {
    return container_of(bridge, struct dpidac, bridge);
}

static inline struct dpidac *drm_connector_to_dpidac(struct drm_connector *connector) {
    return container_of(connector, struct dpidac, connector);
}

static int dpidac_apply_module_mode(struct drm_connector *connector, int modeId, bool preferred) {
    struct drm_device *dev = connector->dev;
    struct drm_display_mode *mode = drm_mode_create(dev);
    struct videomode vmcopy;
    struct videomode *vm = &modes[modeId];
    vmcopy.vback_porch = vm->vback_porch;
    vmcopy.vfront_porch = vm->vfront_porch;
    vmcopy.hback_porch = vm->hback_porch;
    vmcopy.hfront_porch = vm->hfront_porch;
    vmcopy.flags = vm->flags;
    vmcopy.hactive = vm->hactive;
    vmcopy.hsync_len = vm->hsync_len;
    vmcopy.pixelclock = vm->pixelclock;
    vmcopy.vactive = vm->vactive;
    vmcopy.vsync_len = vm->vsync_len;

    drm_display_mode_from_videomode(&vmcopy, mode);
    mode->type = DRM_MODE_TYPE_DRIVER;
    if (preferred)
        mode->type |= DRM_MODE_TYPE_PREFERRED;

    //mode->flags |= (DRM_MODE_FLAG_CSYNC | DRM_MODE_FLAG_NCSYNC);

    drm_mode_set_name(mode);
    printk(KERN_INFO "[RPI-DPIDAC]: \t" DRM_MODE_FMT, DRM_MODE_ARG(mode));
    drm_mode_probed_add(connector, mode);

    return 1;
}

static int dpidac_get_modes(struct drm_connector *connector) {
    int mode_count = 0;

    mode_count = dpidac_load_timings(connector);

    if (!mode_count) {
        printk(KERN_INFO "[RPI-DPIDAC]: Loading timings from bridge...\n");
        mode_count += dpidac_apply_module_mode(connector, p320x240, true);
        mode_count += dpidac_apply_module_mode(connector, p1920x240, false);
        mode_count += dpidac_apply_module_mode(connector, i640x480, false);
        mode_count += dpidac_apply_module_mode(connector, p640x480, false);
        printk(KERN_INFO "[RPI-DPIDAC]: %i default modes loaded\n", mode_count);
    }

    return mode_count;
}

static const struct drm_connector_helper_funcs dpidac_con_helper_funcs = {
        .get_modes    = dpidac_get_modes,
};

static enum drm_connector_status dpidac_connector_detect(struct drm_connector *connector, bool force) {
    return connector_status_connected;
}

static const struct drm_connector_funcs dpidac_con_funcs = {
        .detect           = dpidac_connector_detect,
        .fill_modes       = drm_helper_probe_single_connector_modes,
        .destroy          = drm_connector_cleanup,
        .reset            = drm_atomic_helper_connector_reset,
        .atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
        .atomic_destroy_state   = drm_atomic_helper_connector_destroy_state,
};

static int dpidac_attach(struct drm_bridge *bridge, enum drm_bridge_attach_flags flags) {
    struct dpidac *dpi = drm_bridge_to_dpidac(bridge);
    u32 bus_format = MEDIA_BUS_FMT_RGB888_1X24;
    int ret;

    if (!bridge->encoder) {
        DRM_ERROR("Missing encoder\n");
        return -ENODEV;
    }

    drm_connector_helper_add(&dpi->connector,
                             &dpidac_con_helper_funcs);
    ret = drm_connector_init(bridge->dev, &dpi->connector,
                             &dpidac_con_funcs, DRM_MODE_CONNECTOR_VGA);
    if (ret) {
        DRM_ERROR("Failed to initialize connector\n");
        return ret;
    }

    ret = drm_display_info_set_bus_formats(&dpi->connector.display_info,
                                           &bus_format, 1);
    if (ret) {
        DRM_ERROR("Failed to set bus format\n");
        return ret;
    }

    dpi->connector.interlace_allowed = 1;
    dpi->connector.doublescan_allowed = 1;

    drm_connector_attach_encoder(&dpi->connector,
                                 bridge->encoder);

    return 0;
}

static const struct drm_bridge_funcs dpidac_bridge_funcs = {
        .attach        = dpidac_attach,
};

static int dpidac_probe(struct platform_device *pdev) {
    struct dpidac *dpi;

    dpi = devm_kzalloc(&pdev->dev, sizeof(*dpi), GFP_KERNEL);
    if (!dpi)
        return -ENOMEM;
    platform_set_drvdata(pdev, dpi);

    dpi->bridge.funcs = &dpidac_bridge_funcs;
    dpi->bridge.of_node = pdev->dev.of_node;

    drm_bridge_add(&dpi->bridge);

    printk(KERN_INFO "[RPI-DPIDAC]: module probed\n");

    return 0;
}

static int dpidac_remove(struct platform_device *pdev) {
    struct dpidac *dpi = platform_get_drvdata(pdev);
    drm_bridge_remove(&dpi->bridge);

    printk(KERN_INFO "[RPI-DPIDAC]: module removed\n");

    return 0;
}

static const struct of_device_id dpidac_match[] = {
        {.compatible = "raspberrypi,dpidac"},
        {},
};
MODULE_DEVICE_TABLE(of, dpidac_match);

static struct platform_driver dpidac_driver = {
        .probe  = dpidac_probe,
        .remove = dpidac_remove,
        .driver = {
                .name           = "rpi-dpidac",
                .of_match_table = dpidac_match,
        },
};

module_platform_driver(dpidac_driver);

MODULE_AUTHOR("Hugh Cole-Baker and cpasjuste and rTomas (RTA)");
MODULE_DESCRIPTION("Raspberry Pi DPI DAC bridge driver");
MODULE_LICENSE("GPL");
