/* SPDX-License-Identifier: GPL-2.0 */
/*
 * fl2000_drm.h
 *
 * (C) Copyright 2017, Fresco Logic, Incorporated.
 * (C) Copyright 2018-2019, Artem Mygaiev
 */

#ifndef __FL2000_DRM_H__
#define __FL2000_DRM_H__

#define DEBUG 1

#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/i2c.h>
#include <linux/component.h>
#include <linux/regmap.h>
#include <linux/shmem_fs.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/time.h>
#include <drm/drm_drv.h>
#include <drm/drm_bridge.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_print.h>
#include <drm/drm_vblank.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_gem.h>
#include <drm/drm_of.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem_shmem_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_damage_helper.h>

#include "fl2000_registers.h"

#if defined(CONFIG_DEBUG_FS)

/* NOTE: it might be unsafe / insecure to use allow rw access everyone */
#include <linux/debugfs.h>
static const umode_t fl2000_debug_umode = 0666;

#endif /* CONFIG_DEBUG_FS */

/* Custom code for DRM bridge autodetection since there is no DT support */
#define I2C_CLASS_HDMI	(1<<9)

/**
 * fl2000_add_bitmask - Set bitmask for structure field
 *
 * @__mask: Variable to set mask to (assumed u32)
 * @__type: Structure type to use with bitfield (assumed size equal to u32)
 * @__field: Field to set mask for in the '__type' structure
 *
 * Sets bits to 1 in '__mask' variable that correspond to field '__field' of
 * structure type '__type'. Tested only with u32 data types
 */
#define fl2000_add_bitmask(__mask, __type, __field) \
({ \
	union { \
		__type __mask; \
		typeof (__mask) __val; \
	} __attribute__ ((aligned)) __data; \
	__data.__mask.__field = ~0; \
	(__mask) |= __data.__val; \
})


static inline int fl2000_urb_status(struct usb_device *usb_dev, struct urb *urb)
{
	int ret = 0;

	switch (urb->status) {
	/* All went well */
	case 0:
		break;

	/* URB was unlinked or device shutdown in progress, do nothing */
	case -ECONNRESET:
	case -ENOENT:
	case -ENODEV:
		ret = -1;
		break;

	/* Hardware or protocol errors - no recovery, report and do nothing */
	case -ESHUTDOWN:
	case -EPROTO:
	case -EILSEQ:
	case -ETIME:
		dev_err(&usb_dev->dev, "USB hardware unrecoverable error %d",
				urb->status);
		ret = -1;
		break;

	/* Stalled endpoint */
	case -EPIPE:
		dev_err(&usb_dev->dev, "Endpoint %d stalled",
				urb->ep->desc.bEndpointAddress);
		ret = usb_clear_halt(usb_dev, urb->pipe);
		if (ret != 0) {
			dev_err(&usb_dev->dev, "Cannot reset endpoint, error " \
					"%d", ret);
			ret = -1;
		}
		break;

	/* All the rest cases - just restart transfer */
	default:
		break;
	}

	return ret;
}



#endif /* __FL2000_DRM_H__ */
