# Copyright 2012 Marshmallow Engine. All rights reserved.
# Copyright 2012 Guillermo A. Amaral B. (gamaral) <g@maral.me>.
#
# Redistribution and use in source and binary forms, with or without modification, are
# permitted provided that the following conditions are met:
#
#    1. Redistributions of source code must retain the above copyright notice, this list of
#       conditions and the following disclaimer.
#
#    2. Redistributions in binary form must reproduce the above copyright notice, this list
#       of conditions and the following disclaimer in the documentation and/or other materials
#       provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY MARSHMALLOW ENGINE ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
# FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MARSHMALLOW ENGINE OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# The views and conclusions contained in the software and documentation are those of the
# authors and should not be interpreted as representing official policies, either expressed
# or implied, of Marshmallow Engine.
#

# MARSHMALLOW
set(MARSHMALLOW_AUDIO_DUMMY         OFF   CACHE BOOL   "")
set(MARSHMALLOW_AUDIO_OPENAL        ON    CACHE BOOL   "")

set(MARSHMALLOW_GRAPHICS_TITLE     "BlackBerry Marshmallow" CACHE STRING "")
set(MARSHMALLOW_GRAPHICS_WIDTH     "1024" CACHE STRING "")
set(MARSHMALLOW_GRAPHICS_HEIGHT    "600"  CACHE STRING "")
set(MARSHMALLOW_GRAPHICS_DEPTH     "24"   CACHE STRING "")
set(MARSHMALLOW_GRAPHICS_FULLSCREEN ON    CACHE BOOL   "")

set(MARSHMALLOW_GRAPHICS_DUMMY      OFF   CACHE BOOL   "")
set(MARSHMALLOW_GRAPHICS_OPENGL     ON    CACHE BOOL   "")

set(MARSHMALLOW_OPENGL_EGL          ON    CACHE BOOL   "")
set(MARSHMALLOW_OPENGL_ES2          ON    CACHE BOOL   "")
set(MARSHMALLOW_OPENGL_BB           ON    CACHE BOOL   "")

set(MARSHMALLOW_CONTRIB_LIBPNG      OFF   CACHE BOOL   "")
set(MARSHMALLOW_CONTRIB_ZLIB        OFF   CACHE BOOL   "")

set(CMAKE_SKIP_BUILD_RPATH            TRUE  CACHE INTERNAL "")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE CACHE INTERNAL "")
set(CMAKE_BUILD_WITH_INSTALL_RPATH    TRUE  CACHE INTERNAL "")

