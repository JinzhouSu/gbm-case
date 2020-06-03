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
 *	Author: Jinzhou Su
 *
 */

#include <errno.h>
#include <inttypes.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

#define MAX_DRM_DEVICES 64
//get KMS resources, including crtc id, encoder id and connector id.
static int get_drm_resource(struct drm_resource *res)
{
    int ret = -ENODEV;
    drmModeRes *resources;
    uint32_t i;

    if (!res)
        return -EINVAL;

    resources = drmModeGetResources(res->device_fd);
    if (!res)
        return -ENODEV;

    for (i = 0; i < resources->count_connectors; i++) {
        drmModeConnector *connector;
        drmModeEncoder *encoder;

        connector = drmModeGetConnector(res->device_fd, resources->connectors[i]);
        if (!connector)
            continue;

        if (connector->connection != DRM_MODE_CONNECTED) {
            drmModeFreeConnector(connector);
            continue;
        }

        encoder = drmModeGetEncoder(res->device_fd, connector->encoder_id);
        if (!encoder) {
            drmModeFreeConnector(connector);
            continue;
        }

        res->connector_id = resources->connectors[i];
        res->mode = connector->modes[0];
        res->crtc_id = encoder->crtc_id;
        drmModeFreeEncoder(encoder);
        drmModeFreeConnector(connector);
        ret = 0;
        break;
    }

    for (i = 0; i < resources->count_crtcs; i++) {
        drmModeCrtc *crtc;

        crtc = drmModeGetCrtc(res->device_fd, resources->crtcs[i]);
        if (!crtc)
            continue;

        if (crtc->crtc_id == res->crtc_id) {
            drmModeFreeCrtc(crtc);
            break;
        }

        drmModeFreeCrtc(crtc);
    }

    drmModeFreeResources(resources);
    return ret;
}

int find_drm_device_res(struct drm_resource *res)
{
	drmDevicePtr devices[MAX_DRM_DEVICES] = { NULL };
	int num_devices, fd = -1;

	num_devices = drmGetDevices2(0, devices, MAX_DRM_DEVICES);
	if (num_devices < 0) {
		printf("drmGetDevices2 failed: %s\n", strerror(-num_devices));
		return -1;
	}

	for (int i = 0; i < num_devices; i++) {
		drmDevicePtr device = devices[i];
		int ret;

		if (!(device->available_nodes & (1 << DRM_NODE_PRIMARY)))
			continue;
		/* OK, it's a primary device. If we can get the
		 * drmModeResources, it means it's also a
		 * KMS-capable device.
		 */
		fd = open(device->nodes[DRM_NODE_PRIMARY], O_RDWR);
		if (fd < 0)
			continue;

		res->device_fd = fd;
		ret = get_drm_resource(res);
		if (!ret)
			break;
		close(fd);
		fd = -1;
		res->device_fd = 0;
	}
	drmFreeDevices(devices, num_devices);

	if (fd < 0)
		printf("no drm device found!\n");
	return 0;
}

static int match_config_to_visual(EGLDisplay egl_display,
               EGLint visual_id,
               EGLConfig *configs,
               int count)
{
    int i;
    for(i = 0; i < count; ++i) {
        EGLint id;

        if (!eglGetConfigAttrib(egl_display,
                configs[i], EGL_NATIVE_VISUAL_ID,
                &id))
            continue;

        if (id == visual_id)
            return i;
    }

    return -1;
}

static int egl_choose_config(EGLDisplay egl_display, const EGLint *attribs,
                  EGLint visual_id, EGLConfig *config_out)
{
    EGLint count = 0;
    EGLint matched = 0;
    EGLConfig *configs;
    int config_index = -1;

    if (!eglGetConfigs(egl_display, NULL, 0, &count) || count < 1) {
        printf("No EGL configs to choose from.\n");
        return -1;
    }
    configs = malloc(count * sizeof *configs);
    if (!configs)
        return -1;

    if (!eglChooseConfig(egl_display, attribs, configs,
                  count, &matched) || !matched) {
        printf("No EGL configs with appropriate attributes.\n");
        goto out;
    }

    if (!visual_id)
        config_index = 0;

    if (config_index == -1)
        config_index = match_config_to_visual(egl_display,
			visual_id, configs,matched);
    if (config_index != -1)
        *config_out = configs[config_index];

out:
    free(configs);
    if (config_index == -1)
        return -1;

    return 0;
}

int egl_init(struct egl_resource* egl_res, struct gbm_resource* gbm_res)
{
	EGLint major, minor;

    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE,
    };

    static const EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SAMPLES, 0,
        EGL_NONE,
    };

    egl_res->display = eglGetDisplay((EGLNativeDisplayType)gbm_res->device);
	if (!egl_res->display){
		printf("failed to create EGL display!\n");
		return -1;
	}

    if (!eglInitialize(egl_res->display, &major, &minor)) {
        printf("failed to initialize EGL\n");
        return -1;
    }
    printf("EGL %d.%d\n", major, minor);
	printf("EGL_VENDOR: %s\n", eglQueryString(egl_res->display,  EGL_VENDOR));
	printf("EGL_VERSION: %s\n", eglQueryString(egl_res->display,  EGL_VERSION));

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        printf("failed to bind to OpenGL ES API\n");
        return -1;
    }

    if (egl_choose_config(egl_res->display, config_attribs, GBM_FORMAT_XRGB8888, &egl_res->config)) {
        printf( "failed to choose EGL configuration\n");
        return -1;
    }

    egl_res->context = eglCreateContext(egl_res->display, egl_res->config, EGL_NO_CONTEXT,
                   context_attribs);
    if (!egl_res->context) {
        printf("failed to create EGL context\n");
        return -1;
    }

    egl_res->surface = eglCreateWindowSurface(egl_res->display, egl_res->config, 
			(EGLNativeWindowType)gbm_res->surface, NULL);
    if (!egl_res->surface) {
        printf("failed to create EGL window\n");
        return -1;
    }                                                                        

	eglMakeCurrent(egl_res->display, egl_res->surface, egl_res->surface, egl_res->context); 

	return 0;
}

