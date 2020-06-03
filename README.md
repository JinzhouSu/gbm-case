# gbm-case

gbm-cases
---------------------------------------

gbm-cases is a little program for GBM tests. Generic Buffer Management (GBM) is an 
API that provides a mechanism for allocating buffers for graphics rendering tied to 
Mesa. GBM is intended to be used as a native platform for EGL on drm or openwfd. 
The handle it creates can be used to initialize EGL and to create render target buffers.

Compling
--------

To set up meson:

	meson builddir/

The use ninja to build:

	ninja -C builddir/
