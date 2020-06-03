#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice (including the next
#  paragraph) shall be included in all copies or substantial portions of the
#  Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#  SOFTWARE.
#
project(
  'gbm-case',
  'c',
  version : '0.0.1',
  license : 'MIT',
  meson_version : '>= 0.47',
  default_options : ['buildtype=debug','c_std=gnu99']
)

if get_option('c_std') != 'gnu99'
  error('c_std must be gnu99')
endif

sources = files(
  'common.c',
  'kms-swap.c',
  'egl-swap.c',
  'gbm-main.c',
)

cc = meson.get_compiler('c')
dep_m = cc.find_library('m', required : false)
dep_threads = dependency('threads')
dep_libdrm = dependency('libdrm', version : '>=2.4.71')
dep_gbm = dependency('gbm', version : '>=13.0')
dep_egl = dependency('egl')
dep_gles2 = dependency('glesv2')
dep_libpng = dependency('libpng', required : false)

if dep_libpng.found()
  add_project_arguments('-DHAVE_LIBPNG', language : 'c')
endif


dep_common = [dep_m, dep_threads, dep_libdrm, dep_gbm, dep_egl, dep_gles2, dep_libpng]

executable('gbm-main', sources, dependencies : dep_common, install : true)
