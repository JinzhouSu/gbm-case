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

#include <GL/gl.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "common.h"
 
static void page_flip_handler(int fd, unsigned int frame,
          unsigned int sec, unsigned int usec, void *data)
{
    /* suppress 'unused parameter' warnings */
    (void)fd, (void)frame, (void)sec, (void)usec;

    int *waiting_for_flip = data;
    *waiting_for_flip = 0;
}

static int start_to_play(struct egl_resource *egl_res, struct gbm_resource *gbm_res,
				struct drm_resource *drm_res) {
	struct gbm_bo *bo;
	unsigned int frame_id = 0;
	unsigned int gbm_bo_handle = 0;
	unsigned int bo_stride = 0;
	fd_set fds;
    int err = 0;
    int ret = 0;
    int i = 0;

	drmEventContext evctx = {
		.version = 2,
		.page_flip_handler = page_flip_handler,
	};
	
	eglSwapBuffers(egl_res->display, egl_res->surface);
	bo = gbm_surface_lock_front_buffer(gbm_res->surface);
	if (!bo){
		printf("failed to get gbm surface bo!\n");
		err = -1;
		return err;
	}
    gbm_bo_handle = gbm_bo_get_handle(bo).u32;
	bo_stride = gbm_bo_get_stride(bo);

    err = drmModeAddFB(drm_res->device_fd, drm_res->mode.hdisplay, drm_res->mode.vdisplay, 
			24, 32, bo_stride, gbm_bo_handle, &frame_id);
	if (err < 0){
		printf("Add FB failed!\n");
		if (bo)
			gbm_surface_release_buffer(gbm_res->surface, bo);
		return err;
	}

	err = drmModeSetCrtc(drm_res->device_fd, drm_res->crtc_id, frame_id, 0, 0,
			&drm_res->connector_id, 1, &drm_res->mode);
	if (err){
		printf("set crtc failed!\n");
		goto out_play;
	}

	for (i = 0; i < 600; i++){
		struct gbm_bo *next_bo;
		int waiting_for_flip = 1;

		glClearColor(i/600.0f, 0.0, 1.0f-i/600.0f, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		eglSwapBuffers(egl_res->display, egl_res->surface);
	    next_bo = gbm_surface_lock_front_buffer(gbm_res->surface);
	    gbm_bo_handle = gbm_bo_get_handle(next_bo).u32;
	    bo_stride = gbm_bo_get_stride(next_bo);
		err = drmModeAddFB(drm_res->device_fd, drm_res->mode.hdisplay, drm_res->mode.vdisplay, 
				24, 32, bo_stride, gbm_bo_handle, &frame_id);
		if (err < 0){
			printf("Add FB failed!\n");
			goto out_play;
		}

		err = drmModePageFlip(drm_res->device_fd, drm_res->crtc_id, frame_id,
                DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip);
        if (err) {
            printf("failed to queue page flip\n");
			goto out_play;
        }

		err = drmModeSetCrtc(drm_res->device_fd, drm_res->crtc_id, frame_id, 0, 0,
				&drm_res->connector_id, 1, &drm_res->mode);
		if (err){
			printf("failed to set crtc\n");
			goto out_play;
		}

        while (waiting_for_flip) {
            FD_ZERO(&fds);
            FD_SET(0, &fds);
            FD_SET(drm_res->device_fd, &fds);

            ret = select(drm_res->device_fd + 1, &fds, NULL, NULL, NULL);
            if (ret < 0) {
                printf("select err: %d\n", err);
                return ret;
            } else if (ret == 0) {
                printf("select timeout!\n");
                return -1;
            } else if (FD_ISSET(0, &fds)) {
                printf("user interrupted!\n");
                return 0;
            }
            drmHandleEvent(drm_res->device_fd, &evctx);
        }
		
		gbm_surface_release_buffer(gbm_res->surface, bo);
		bo = next_bo;
	}

	if (bo) {
		drmModeRmFB(drm_res->device_fd, frame_id);
		gbm_surface_release_buffer(gbm_res->surface, bo);
	}
 
	return err;

out_play:
	if (bo) {
		drmModeRmFB(drm_res->device_fd, frame_id);
		gbm_surface_release_buffer(gbm_res->surface, bo);
	}
	return err;
}
 
static void clean_up(struct egl_resource* egl_res, struct gbm_resource* gbm_res) {
	eglDestroySurface (egl_res->display, egl_res->surface);
	gbm_surface_destroy (gbm_res->surface);
	eglDestroyContext (egl_res->display, egl_res->context);
	eglTerminate (egl_res->display);
	gbm_device_destroy (gbm_res->device);
}
 
 
int egl_swap() 
{
	struct drm_resource *drm_res = NULL;
	struct gbm_resource *gbm_res = NULL;
	struct egl_resource *egl_res = NULL;
	int err = 0;

	drm_res = calloc(1, sizeof(*drm_res));
	if (!drm_res){
		err = -1;
		goto err_handle1;
	}

	gbm_res = calloc(1, sizeof(*gbm_res));
	if (!gbm_res){
		err = -1;
		goto err_handle1;
	}

	egl_res = calloc(1, sizeof(*egl_res));
	if (!egl_res){
		err = -1;
		goto err_handle1;
	}
	// Get DRM resources
	err = find_drm_device_res(drm_res);
	if (err) {
		printf("get device resource failed!\n");
		goto err_handle1;
	}

	err = drmSetMaster(drm_res->device_fd);
	if (err < 0){
		printf("Set master failed!\n");
		goto err_handle1;
	}

	//reserve previous Crtc status
	drm_res->crtc = drmModeGetCrtc(drm_res->device_fd, drm_res->crtc_id);

	/* Create EGL Context using GBM */
	gbm_res->device = gbm_create_device(drm_res->device_fd);
	if (!gbm_res->device){
		printf("failed to create GBM device!\n");
		err = -1;
		goto err_handle2;
	}

	gbm_res->surface = gbm_surface_create(gbm_res->device, drm_res->mode.hdisplay, drm_res->mode.vdisplay, 
									GBM_BO_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT|GBM_BO_USE_RENDERING);
	if (!gbm_res->surface){
		printf("failed to create gbm surface!\n");
		err = -1;
		goto err_handle2;
	}

	err = egl_init(egl_res, gbm_res);
	if (err){
		printf("failed to call egl_init!\n");
		goto err_handle2;
	}

	err = start_to_play(egl_res, gbm_res, drm_res);
	if (err){
		printf("failed to play!\n");
		goto err_handle2;
	}

	sleep(2);

err_handle2:

	clean_up(egl_res, gbm_res);

	if(drm_res->crtc){
		drmModeSetCrtc(drm_res->device_fd, drm_res->crtc->crtc_id, drm_res->crtc->buffer_id,
				drm_res->crtc->x, drm_res->crtc->y, &drm_res->connector_id, 1, &drm_res->mode);
		drmModeFreeCrtc(drm_res->crtc);
	}

	drmDropMaster(drm_res->device_fd);
	close(drm_res->device_fd);

err_handle1:
	if (egl_res)
		free(egl_res);
	if (gbm_res)
		free(gbm_res);
	if (drm_res)
		free(drm_res);

	return err;
}
