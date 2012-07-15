/*
 * Copyright 2011-2012 Marshmallow Engine. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this list of
 *      conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice, this list
 *      of conditions and the following disclaimer in the documentation and/or other materials
 *      provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY MARSHMALLOW ENGINE ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MARSHMALLOW ENGINE OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Marshmallow Engine.
 */

#include "graphics/viewport.h"

/*!
 * @file
 *
 * @author Guillermo A. Amaral B. (gamaral) <g@maral.me>
 */

#include "core/logger.h"

#include "event/eventmanager.h"
#include "event/keyboardevent.h"
#include "event/quitevent.h"

#include "graphics/painter.h"
#include "graphics/transform.h"

#include <X11/X.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>
#include <X11/Xutil.h>

#include "headers.h"
#ifdef MARSHMALLOW_OPENGL_GLES2
#  include <EGL/egl.h>
#else
#  include <GL/glx.h>
#endif
#include "extensions.h"

#include <cmath>
#include <cstring>
#include <list>

MARSHMALLOW_NAMESPACE_USE
using namespace Graphics::OpenGL;
using namespace Graphics;

/******************************************************************************/

namespace
{
	struct ViewportData {
		Window        window;
		Display      *display;
		Math::Size2i  wsize;
		int           screen;
#ifdef MARSHMALLOW_OPENGL_GLES2
		EGLContext    egl_context;
		EGLDisplay    egl_display;
		EGLSurface    egl_surface;
#else
		GLXContext    context;
#endif
		Atom          wm_delete;
		Transform     camera;
		float         radius2;
		Math::Size2f  size;
		Math::Size2f  scaled_size;
		bool          fullscreen;
		bool          loaded;
	} s_data;

	void UpdateCamera(void);
	void HandleKeyEvent(XKeyEvent &key);

	void InitializeViewport(void)
	{
		s_data.camera.setRotation(.0f);
		s_data.camera.setScale(Math::Size2f::Identity());
		s_data.camera.setTranslation(Math::Point2::Zero());

#ifdef MARSHMALLOW_OPENGL_GLES2
		s_data.egl_context = EGL_NO_CONTEXT;
		s_data.egl_display = EGL_NO_DISPLAY;
		s_data.egl_surface = EGL_NO_SURFACE;
#else
		s_data.context = 0;
#endif

		s_data.display = 0;
		s_data.fullscreen = false;
		s_data.loaded = false;
		s_data.screen = 0;
		s_data.size.zero();
		s_data.window = 0;
		s_data.wm_delete = 0;
		s_data.wsize[0] = s_data.wsize[1] = 0;
	}

	bool
	CreateWindow(uint16_t w, uint16_t h, uint8_t d, uint8_t r, bool f, bool v)
	{
		MMUNUSED(r);
		MMUNUSED(v);

#ifdef MARSHMALLOW_OPENGL_GLES2
		MMUNUSED(r);
		s_data.egl_context = EGL_NO_CONTEXT;
		s_data.egl_display = EGL_NO_DISPLAY;
		s_data.egl_surface = EGL_NO_SURFACE;
#else
		s_data.context = 0;
#endif

		s_data.display   = 0;
		s_data.loaded    = false;
		s_data.screen    = 0;
		s_data.window    = 0;
		s_data.wm_delete = 0;

		s_data.fullscreen = f;
		s_data.wsize[0] = w;
		s_data.wsize[1] = h;

		/*** X11 ***/

		/* open display */
		if (!(s_data.display = XOpenDisplay(0))) {
			MMERROR("Unable to open X Display.");
			return(false);
		}

		/* get root window */
		Window l_rwindow = DefaultRootWindow(s_data.display);

		XVisualInfo l_vinfo;
		if (!XMatchVisualInfo(s_data.display, XDefaultScreen(s_data.display), d, TrueColor, &l_vinfo)) {
			MMERROR("Failed to find an appropriate visual.");
			return(false);
		}
		s_data.screen = l_vinfo.screen;

		XSync(s_data.display, true);

		/* window attributes */
		XSetWindowAttributes l_swattr;
		memset(&l_swattr, 0, sizeof(l_swattr));
		l_swattr.border_pixel = 0;
		l_swattr.colormap = XCreateColormap(s_data.display, l_rwindow, l_vinfo.visual, AllocNone);
		l_swattr.event_mask =
			ButtonPressMask    |
			ButtonReleaseMask  |
			KeyPressMask       |
			KeyReleaseMask     |
			PointerMotionMask  |
			StructureNotifyMask;

		/*
		 * ** FULLSCREEN MODE **
		 */
		if (s_data.fullscreen) {

			/* default window size to display size */
			s_data.wsize[0] = DisplayWidth(s_data.display,  s_data.screen);
			s_data.wsize[1] = DisplayHeight(s_data.display, s_data.screen);

			/* create a fullscreen window */
			l_swattr.override_redirect = true;
			s_data.window = XCreateWindow
			   (s_data.display,
			    l_rwindow,
			    0, 0, s_data.wsize[0], s_data.wsize[1],
			    1,
			    l_vinfo.depth,
			    InputOutput,
			    l_vinfo.visual,
			    CWColormap|CWBorderPixel|CWEventMask|CWOverrideRedirect,
			    &l_swattr);
			XMapRaised(s_data.display, s_data.window);

			/* notify window manager */
			Atom l_wm_state   = XInternAtom(s_data.display, "_NET_WM_STATE", false);
			Atom l_fullscreen = XInternAtom(s_data.display, "_NET_WM_STATE_FULLSCREEN", false);

			XEvent l_event;
			memset(&l_event, 0, sizeof(l_event));
			
			l_event.type                 = ClientMessage;
			l_event.xclient.window       = s_data.window;
			l_event.xclient.message_type = l_wm_state;
			l_event.xclient.format       = 32;
			l_event.xclient.data.l[0]    = 1;
			l_event.xclient.data.l[1]    = static_cast<long>(l_fullscreen);
			XSendEvent(s_data.display, l_rwindow, false, SubstructureNotifyMask, &l_event );
		}

		/*
		 * ** WINDOW MODE **
		 */
		else {
			s_data.window = XCreateWindow
			   (s_data.display,
			    l_rwindow,
			    (DisplayWidth(s_data.display,  s_data.screen) - s_data.wsize[0]) / 2,
			    (DisplayHeight(s_data.display, s_data.screen) - s_data.wsize[1]) / 2,
			    s_data.wsize[0], s_data.wsize[1],
			    1,
			    l_vinfo.depth,
			    InputOutput,
			    l_vinfo.visual,
			    CWColormap|CWBorderPixel|CWEventMask,
			    &l_swattr);
			XMapRaised(s_data.display, s_data.window);

			/* set size hints */
			XSizeHints *l_size_hints;
			if(!(l_size_hints = XAllocSizeHints())) {
				MMERROR("Unable to allocate window size hints.");
				return(false);
			}
			l_size_hints->flags = PMinSize|PMaxSize;
			l_size_hints->min_width = s_data.wsize[0];
			l_size_hints->min_height = s_data.wsize[1];
			l_size_hints->max_width = s_data.wsize[0];
			l_size_hints->max_height = s_data.wsize[1];
			XSetWMNormalHints(s_data.display, s_data.window, l_size_hints);
			XFree(l_size_hints);
		}
		XkbSetDetectableAutoRepeat(s_data.display, true, 0);

#ifdef MARSHMALLOW_VIEWPORT_GRAB_INPUT
		/* take pointer and keyboard */
		XWarpPointer(s_data.display, None, s_data.window, 0, 0, 0, 0, 0, 0);
		XGrabKeyboard(s_data.display, s_data.window, true, GrabModeAsync,
		    GrabModeAsync, CurrentTime);
		XGrabPointer(s_data.display, s_data.window, true,
		    ButtonPressMask, GrabModeAsync, GrabModeAsync,
		    s_data.window, None, CurrentTime);
#endif

		XSync(s_data.display, true);

		/* set window title */
		XTextProperty l_window_name;
		static uint8_t l_window_title[] = MARSHMALLOW_BUILD_TITLE;
		l_window_name.value    = l_window_title;
		l_window_name.encoding = XA_STRING;
		l_window_name.format   = 8;
		l_window_name.nitems   = strlen(MARSHMALLOW_BUILD_TITLE);
		XSetWMName(s_data.display, s_data.window, &l_window_name);

		/* catch window manager delete event */
		s_data.wm_delete = XInternAtom(s_data.display, "WM_DELETE_WINDOW", false);
		XSetWMProtocols(s_data.display, s_data.window, &s_data.wm_delete, 1);

		/*** GL ***/

#ifdef MARSHMALLOW_OPENGL_GLES2
		s_data.egl_display = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(s_data.display));
		if (s_data.egl_display == EGL_NO_DISPLAY) {
			MMERROR("EGL: No display was found.");
			return(false);
		}
		
		if (!eglInitialize(s_data.egl_display, 0, 0)) {
			MMERROR("EGL: Initialization failed.");
			return(false);
		}
		
		EGLint l_attr[] = {
			EGL_BUFFER_SIZE,     d,
			EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL_NONE,            EGL_NONE
		};

		EGLint l_config_count;
		EGLConfig l_config;
		if (!eglChooseConfig(s_data.egl_display, l_attr, &l_config, 1,
		                     &l_config_count))
			MMERROR("EGL: No config was chosen. EGLERROR=" << eglGetError());

		s_data.egl_surface =
		    eglCreateWindowSurface(s_data.egl_display, l_config,
		                           (EGLNativeWindowType)s_data.window,
		                           0);
		if (s_data.egl_surface == EGL_NO_SURFACE) {
			MMERROR("EGL: No surface was created. EGLERROR=" << eglGetError());
			return(false);
		}

		EGLint l_ctxattr[] = {
			EGL_CONTEXT_CLIENT_VERSION, 2,
			EGL_NONE,                   EGL_NONE
		};
		s_data.egl_context = eglCreateContext(s_data.egl_display, l_config, EGL_NO_CONTEXT, l_ctxattr);
		if (s_data.egl_context == EGL_NO_CONTEXT) {
			MMERROR("EGL: Context was created. EGLERROR=" << eglGetError());
			return(false);
		}

		if (!eglMakeCurrent(s_data.egl_display, s_data.egl_surface, s_data.egl_surface, s_data.egl_context)) {
			MMERROR("EGL: Failed to switch current context. EGLERROR=" << eglGetError());
			return(false);
		}

		/* extensions */

		InitializeExtensions(eglQueryString(s_data.egl_display, EGL_EXTENSIONS));
#else
		/* create context */

		if (!(s_data.context = glXCreateContext(s_data.display, &l_vinfo, 0, GL_TRUE))) {
			MMERROR("GLX: Failed to create context!");
			return(false);
		}

		if (!glXMakeCurrent(s_data.display, s_data.window, s_data.context)) {
			MMERROR("GLX: Failed to make context current!");
			return(false);
		}

		if (!glXIsDirect(s_data.display, s_data.context)) {
			MMERROR("GLX: Context doesn't support direct rendering.");
			return(false);
		}

		/* extensions */

		InitializeExtensions(glXQueryExtensionsString(s_data.display, s_data.screen));
#endif

		/* vsync */

		if (HasExtension("GLX_SGI_swap_control"))
			glSwapInterval(v ? 1 : 0);

		/* initialize context */

		glViewport(0, 0, s_data.wsize[0], s_data.wsize[1]);

		if(glGetError() != GL_NO_ERROR) {
			MMERROR("GLX: Failed during initialization.");
			return(false);
		}

		/* set viewport size */

#if MARSHMALLOW_VIEWPORT_LOCK_WIDTH
		s_data.size[0] = MARSHMALLOW_VIEWPORT_WIDTH;
		s_data.size[1] = (MARSHMALLOW_VIEWPORT_WIDTH * static_cast<float>(s_data.wsize[1])) /
			static_cast<float>(s_data.wsize[0]);

#else
		s_data.size[0] = (MARSHMALLOW_VIEWPORT_HEIGHT * static_cast<float>(s_data.wsize[0])) /
			static_cast<float>(s_data.wsize[1]);
		s_data.size[1] = MARSHMALLOW_VIEWPORT_HEIGHT;
#endif

		Viewport::SetCamera(s_data.camera);

		return(s_data.loaded = true);
	}

	void
	DestroyWindow(void)
	{
		if (s_data.display) {
#ifdef MARSHMALLOW_OPENGL_GLES2
			eglMakeCurrent(s_data.egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			eglDestroyContext(s_data.egl_display, s_data.egl_context);
			eglDestroySurface(s_data.egl_display, s_data.egl_surface);
			eglTerminate(s_data.egl_display);
#else
			glXMakeCurrent(s_data.display, None, 0);
			if (s_data.context)
			    glXDestroyContext(s_data.display, s_data.context);
#endif
			if (s_data.window)
			    XDestroyWindow(s_data.display, s_data.window);
			XCloseDisplay(s_data.display);
		}

#ifdef MARSHMALLOW_OPENGL_GLES2
		s_data.egl_context = EGL_NO_CONTEXT;
		s_data.egl_display = EGL_NO_DISPLAY;
		s_data.egl_surface = EGL_NO_SURFACE;
#else
		s_data.context = 0;
#endif
		s_data.display = 0;
		s_data.loaded = false;
		s_data.screen = 0;
		s_data.window = 0;
		s_data.wm_delete = 0;
	}

	void
	UpdateCamera(void)
	{
		/* calculate scaled viewport size */
		s_data.scaled_size[0] = s_data.size[0] / s_data.camera.scale().width();
		s_data.scaled_size[1] = s_data.size[1] / s_data.camera.scale().height();

		/* calculate magnitude and pass it off as radius squared */
		s_data.radius2 = powf(s_data.scaled_size[0] / 2.f, 2.f) +
		                 powf(s_data.scaled_size[1] / 2.f, 2.f);
	}

	void
	HandleKeyEvent(XKeyEvent &key)
	{
		typedef std::list<Event::KBKeys> KeyList;
		static KeyList s_keys_pressed;

		Event::KBKeys l_key = Event::KEY_NONE;
		Event::KBActions l_action =
		    (key.type == KeyPress ? Event::KeyPressed : Event::KeyReleased);

		switch (XLookupKeysym(&key, 0)) {
		case XK_0:            l_key = Event::KEY_0; break;
		case XK_1:            l_key = Event::KEY_1; break;
		case XK_2:            l_key = Event::KEY_2; break;
		case XK_3:            l_key = Event::KEY_3; break;
		case XK_4:            l_key = Event::KEY_4; break;
		case XK_5:            l_key = Event::KEY_5; break;
		case XK_6:            l_key = Event::KEY_6; break;
		case XK_7:            l_key = Event::KEY_7; break;
		case XK_8:            l_key = Event::KEY_8; break;
		case XK_9:            l_key = Event::KEY_9; break;
		case XK_a:            l_key = Event::KEY_A; break;
		case XK_b:            l_key = Event::KEY_B; break;
		case XK_c:            l_key = Event::KEY_C; break;
		case XK_d:            l_key = Event::KEY_D; break;
		case XK_e:            l_key = Event::KEY_E; break;
		case XK_f:            l_key = Event::KEY_F; break;
		case XK_g:            l_key = Event::KEY_G; break;
		case XK_h:            l_key = Event::KEY_H; break;
		case XK_i:            l_key = Event::KEY_I; break;
		case XK_j:            l_key = Event::KEY_J; break;
		case XK_k:            l_key = Event::KEY_K; break;
		case XK_l:            l_key = Event::KEY_L; break;
		case XK_m:            l_key = Event::KEY_M; break;
		case XK_n:            l_key = Event::KEY_N; break;
		case XK_o:            l_key = Event::KEY_O; break;
		case XK_p:            l_key = Event::KEY_P; break;
		case XK_q:            l_key = Event::KEY_Q; break;
		case XK_r:            l_key = Event::KEY_R; break;
		case XK_s:            l_key = Event::KEY_S; break;
		case XK_t:            l_key = Event::KEY_T; break;
		case XK_u:            l_key = Event::KEY_U; break;
		case XK_v:            l_key = Event::KEY_V; break;
		case XK_w:            l_key = Event::KEY_W; break;
		case XK_x:            l_key = Event::KEY_X; break;
		case XK_y:            l_key = Event::KEY_Y; break;
		case XK_z:            l_key = Event::KEY_Z; break;
		case XK_Alt_L:        l_key = Event::KEY_ALT_L; break;
		case XK_Alt_R:        l_key = Event::KEY_ALT_R; break;
		case XK_BackSpace:    l_key = Event::KEY_BACKSPACE; break;
		case XK_Break:        l_key = Event::KEY_BREAK; break;
		case XK_Caps_Lock:    l_key = Event::KEY_CAPS_LOCK; break;
		case XK_Clear:        l_key = Event::KEY_CLEAR; break;
		case XK_Control_L:    l_key = Event::KEY_CONTROL_R; break;
		case XK_Control_R:    l_key = Event::KEY_CONTROL_L; break;
		case XK_Delete:       l_key = Event::KEY_DELETE; break;
		case XK_Down:         l_key = Event::KEY_DOWN; break;
		case XK_End:          l_key = Event::KEY_END; break;
		case XK_Escape:       l_key = Event::KEY_ESCAPE; break;
		case XK_Help:         l_key = Event::KEY_HELP; break;
		case XK_Home:         l_key = Event::KEY_HOME; break;
		case XK_Insert:       l_key = Event::KEY_INSERT; break;
		case XK_Left:         l_key = Event::KEY_LEFT; break;
		case XK_Menu:         l_key = Event::KEY_MENU; break;
		case XK_Meta_L:       l_key = Event::KEY_META_L; break;
		case XK_Meta_R:       l_key = Event::KEY_META_R; break;
		case XK_Num_Lock:     l_key = Event::KEY_NUM_LOCK; break;
		case XK_Page_Down:    l_key = Event::KEY_PAGE_DOWN; break;
		case XK_Page_Up:      l_key = Event::KEY_PAGE_UP; break;
		case XK_Pause:        l_key = Event::KEY_PAUSE; break;
		case XK_Print:        l_key = Event::KEY_PRINT; break;
		case XK_Return:       l_key = Event::KEY_RETURN; break;
		case XK_Right:        l_key = Event::KEY_RIGHT; break;
		case XK_Scroll_Lock:  l_key = Event::KEY_SCROLL_LOCK; break;
		case XK_Shift_L:      l_key = Event::KEY_SHIFT_L; break;
		case XK_Shift_R:      l_key = Event::KEY_SHIFT_R; break;
		case XK_Tab:          l_key = Event::KEY_TAB; break;
		case XK_Up:           l_key = Event::KEY_UP; break;
		case XK_backslash:    l_key = Event::KEY_BACKSLASH; break;
		case XK_bracketleft:  l_key = Event::KEY_BRACKET_LEFT; break;
		case XK_bracketright: l_key = Event::KEY_BRACKET_RIGHT; break;
		case XK_equal:        l_key = Event::KEY_EQUAL; break;
		case XK_greater:      l_key = Event::KEY_GREATER; break;
		case XK_less:         l_key = Event::KEY_LESS; break;
		case XK_quotedbl:     l_key = Event::KEY_DBLQUOTE; break;
		case XK_semicolon:    l_key = Event::KEY_SEMICOLON; break;
		case XK_space:        l_key = Event::KEY_SPACE; break;
		case XK_F1:           l_key = Event::KEY_F1; break;
		case XK_F2:           l_key = Event::KEY_F2; break;
		case XK_F3:           l_key = Event::KEY_F3; break;
		case XK_F4:           l_key = Event::KEY_F4; break;
		case XK_F5:           l_key = Event::KEY_F5; break;
		case XK_F6:           l_key = Event::KEY_F6; break;
		case XK_F7:           l_key = Event::KEY_F7; break;
		case XK_F8:           l_key = Event::KEY_F8; break;
		case XK_F9:           l_key = Event::KEY_F9; break;
		case XK_F10:          l_key = Event::KEY_F10; break;
		case XK_F11:          l_key = Event::KEY_F11; break;
		case XK_F12:          l_key = Event::KEY_F12; break;
		case XK_F13:          l_key = Event::KEY_F13; break;
		case XK_F14:          l_key = Event::KEY_F14; break;
		case XK_F15:          l_key = Event::KEY_F15; break;
		case XK_KP_0:         l_key = Event::KEY_K0; break;
		case XK_KP_1:         l_key = Event::KEY_K1; break;
		case XK_KP_2:         l_key = Event::KEY_K2; break;
		case XK_KP_3:         l_key = Event::KEY_K3; break;
		case XK_KP_4:         l_key = Event::KEY_K4; break;
		case XK_KP_5:         l_key = Event::KEY_K5; break;
		case XK_KP_6:         l_key = Event::KEY_K6; break;
		case XK_KP_7:         l_key = Event::KEY_K7; break;
		case XK_KP_8:         l_key = Event::KEY_K8; break;
		case XK_KP_9:         l_key = Event::KEY_K9; break;
		case XK_KP_Decimal:   l_key = Event::KEY_KDECIMAL; break;
		case XK_KP_Divide:    l_key = Event::KEY_KDIVIDE; break;
		case XK_KP_Multiply:  l_key = Event::KEY_KMULTIPLY; break;
		default: MMWARNING("Unknown key pressed!");
		}

		bool l_key_pressed = false;
		KeyList::const_iterator l_pressed_key_i;
		for (l_pressed_key_i = s_keys_pressed.begin();
		     l_pressed_key_i != s_keys_pressed.end();
		     ++l_pressed_key_i)
			if (*l_pressed_key_i == l_key) {
				l_key_pressed = true;
				break;
			}
		
		if (( l_key_pressed && l_action != Event::KeyPressed)
		 || (!l_key_pressed && l_action == Event::KeyPressed)) {
			Event::SharedEvent event(new Event::KeyboardEvent(l_key, l_action));
			Event::EventManager::Instance()->queue(event);

			if (l_key_pressed) s_keys_pressed.remove(l_key);
			else s_keys_pressed.push_front(l_key);
		}
	}
} // namespace

/***************************************************************** Extensions */

PFNPROC
OpenGL::glGetProcAddress(const char *f)
{
#ifdef MARSHMALLOW_OPENGL_GLES2
	return(eglGetProcAddress(f));
#else
	return(glXGetProcAddress(reinterpret_cast<const GLubyte *>(f)));
#endif
}

/******************************************************************************/
bool
Viewport::Initialize(uint16_t w, uint16_t h, uint8_t d, uint8_t r, bool f, bool v)
{
	InitializeViewport();

	if (!CreateWindow(w, h, d, r, f, v)) {
		DestroyWindow();
		return(false);
	}

	Painter::Initialize();
	return(true);
}

void
Viewport::Finalize(void)
{
	Painter::Finalize();
	DestroyWindow();
}

bool
Viewport::Redisplay(uint16_t w, uint16_t h, uint8_t d, uint8_t r, bool f, bool v)
{
	DestroyWindow();

	if(!CreateWindow(w, h, d, r, f, v)) {
		DestroyWindow();
		return(false);
	}
	return(true);
}

void
Viewport::Tick(void)
{
	XEvent e;

	while(XPending(s_data.display)) {
		XNextEvent(s_data.display, &e);

		switch(e.type) {
		case ClientMessage: {
			XClientMessageEvent &client = e.xclient;
			if (static_cast<Atom>(client.data.l[0]) == s_data.wm_delete) {
				Event::QuitEvent l_event(-1);
				Event::EventManager::Instance()->dispatch(l_event);
			}
		} break;

		case KeyPress:
		case KeyRelease: HandleKeyEvent(e.xkey); break;

		case ButtonPress:
		case ButtonRelease:
			/* TODO: Send Events */
		break;

		case MotionNotify:
			/* TODO: Send Events */
			break;

		default: MMINFO("Unknown viewport event received."); break;
		}
	}
}

void
Viewport::SwapBuffer(void)
{
#ifdef MARSHMALLOW_OPENGL_GLES2
	eglSwapBuffers(s_data.egl_display, s_data.egl_surface);
#else
	glXSwapBuffers(s_data.display, s_data.window);
#endif
	Painter::Reset();
}

const Graphics::Transform &
Viewport::Camera(void)
{
	return(s_data.camera);
}

void
Viewport::SetCamera(const Graphics::Transform &camera)
{
	s_data.camera = camera;
	UpdateCamera();
}

float
Viewport::Radius2(void)
{
	return(s_data.radius2);
}

const Math::Size2f &
Viewport::ScaledSize(void)
{
	return(s_data.scaled_size);
}

const Math::Size2f &
Viewport::Size(void)
{
	return(s_data.size);
}

const Math::Size2i &
Viewport::WindowSize(void)
{
	return(s_data.wsize);
}

const Core::Type &
Viewport::Type(void)
{
	static const Core::Type s_type("GLX");
	return(s_type);
}

