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

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/dma-buf.h>
#include <drm_fourcc.h>
#include <gbm.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
/** Specified options strings for getopt */ 
static const char options[]   = "hbek";
int kms_swap();
int egl_swap();

int main(int argc, char *argv[])
{
    int c;          /* Character received from getopt */
	int err = 0;

	while ((c = getopt(argc, argv, options)) != -1) {
		switch (c) {
		case 'h' :
			printf("help function!\n");
			break;
		case 'b' :
			printf("basic test function\n");
		//	gbm_basic();
			break;
		case 'k' :
			printf("kms swap funciont\n");
			err = kms_swap();
			if (err)
				printf("kms swap failed!\n");
			else
				printf("kms swap succeed\n");
			break;
		case 'e' :
		    printf("egl function\n");
			err = egl_swap();
			if (err)
				printf("egl swap failed!\n");
			else
				printf("egl swap succeed\n");
			break;
		default :
		    printf("all function\n");	
			break;
		}
	}
	return 0;
}
