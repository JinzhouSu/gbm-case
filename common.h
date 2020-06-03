/*
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 *   Author: Jinzhou Su
 *
 */

#ifndef _DRM_COMMON_H
#define _DRM_COMMON_H

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <EGL/egl.h> 
#include <gbm.h>

struct drm_resource{
	int device_fd;
	uint32_t crtc_id;
	uint32_t encoder_id;
	uint32_t connector_id;
	drmModeModeInfo mode;
    drmModeCrtcPtr crtc;
};

struct egl_resource{
	EGLDisplay display;
	EGLContext context;
	EGLSurface surface;
    EGLConfig config;
};

struct gbm_resource{
	struct gbm_device *device;
	struct gbm_surface *surface;
	uint32_t format;
    int width;
	int height;	
};

int find_drm_device_res(struct drm_resource *res);
int egl_init(struct egl_resource* egl_res, struct gbm_resource* gbm_res);
#endif /* _DRM_COMMON_H */
