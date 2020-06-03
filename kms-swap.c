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
 *  Author: Jinzhou Su  
 *
 */

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include "common.h"

int kms_swap()
{
	struct drm_resource *res = NULL;
	struct gbm_device *g_device = NULL;
	struct gbm_bo *bo = NULL;
	unsigned int frame_id = 0;
	unsigned int gbm_bo_handle = 0;
	unsigned int bo_stride = 0;
	int err = 0; 
	int h = 0;
	int v = 0;
	int count = 0;
	void *ptr = NULL;
	const uint32_t colors[2] = {0xff0000ff, 0xffff0000};

	res = calloc(1, sizeof(*res));
	if (!res){
		return -1;
	}

	err = find_drm_device_res(res);
	if (err){
		printf("get device resource failed!\n");
		goto err_handle1;
	}

	err = drmSetMaster(res->device_fd);
	if (err < 0){
		printf("Set master failed!\n");
		goto err_handle1;
	}

	res->crtc = drmModeGetCrtc(res->device_fd, res->crtc_id);

	g_device = gbm_create_device(res->device_fd);
	if (!g_device){
		printf("failed to create GBM device\n");
		err = -1;
		goto err_handle2;
	}

	bo = gbm_bo_create(g_device, res->mode.hdisplay, res->mode.vdisplay, 
			DRM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_LINEAR);
	if(!bo){
		printf("failed to create GBM buffer object\n");
		err = -1;
		goto err_handle2;
	}
	gbm_bo_handle = gbm_bo_get_handle(bo).u32;
	bo_stride = gbm_bo_get_stride(bo);

	err = drmModeAddFB(res->device_fd, res->mode.hdisplay, res->mode.vdisplay, 24, 32, bo_stride,
			gbm_bo_handle, &frame_id);
	if (err < 0){
		printf("Add FB failed!\n");
		goto err_handle2;
	}

	while (1) {
		void *data = NULL;
		ptr = gbm_bo_map(bo, 0, 0, res->mode.hdisplay, res->mode.vdisplay,
				GBM_BO_TRANSFER_READ_WRITE, &bo_stride, &data);
		if (ptr == NULL){
			printf("failed to map GBM buffer object!\n");
			err = -1;
			goto err_handle2;
		} else {
			for(h = 0; h < res->mode.vdisplay; h++){
				uint32_t *pixels = ptr + h * bo_stride;
				for(v = 0; v < res->mode.hdisplay; v++)
					*(pixels+v) = colors[count & 1];
			}
			gbm_bo_unmap(bo, data);
		}
		err = drmModeSetCrtc(res->device_fd, res->crtc_id, frame_id, 0, 0,
				&res->connector_id, 1, &res->mode);
		if (err < 0){
			printf("set Crtc failed!\n");
			goto err_handle2;
		}
		sleep(2);
		count++;
		if (count == 4)
			break;
	}

err_handle2:
	if(bo != NULL)
		gbm_bo_destroy(bo);

	if (g_device)
		gbm_device_destroy(g_device);

	if(res->crtc){
		drmModeSetCrtc(res->device_fd, res->crtc->crtc_id, res->crtc->buffer_id, 
			res->crtc->x, res->crtc->y, &res->connector_id, 1, &res->mode);
		drmModeFreeCrtc(res->crtc);
	}

	drmDropMaster(res->device_fd);

	close(res->device_fd);
err_handle1:
	if (res != NULL)
		free(res);

	return err;
}
