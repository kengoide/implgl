// vim: set ts=4 sw=4 sts=4 et:

#include <cassert>
#include <cstdio>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <GL/glut.h>

#include "glimpl.h"

#define LOG std::printf

typedef void (*DisplayFunc)();

struct GlutWindow {
	Window window;
	DisplayFunc display_func;
	void* gl;

	GlutWindow(Window& window, void* gl)
		: window(window), gl(gl), display_func(nullptr) {
	}
};

namespace {
int window_x, window_y;
int window_width, window_height;
unsigned int display_mode;
GlutWindow* windows[2];
unsigned int current_window;
unsigned int next_window_id;

inline GlutWindow* current() { return windows[current_window]; }

Display* display;
int screen;
Atom wm_delete_window;
}

FGAPI void FGAPIENTRY
glutInit(int* argc, char** argv) {
	window_x = 320;
	window_y = 240;
	window_width = 320;
	window_height = 240;
	display_mode = 0;

	windows[0] = reinterpret_cast<GlutWindow*>(0x00c0ffee);
	windows[1] = nullptr;
	current_window = 0;
	next_window_id = 1;

	display = nullptr;
	screen = 0;
    wm_delete_window = 0;
}

FGAPI void FGAPIENTRY
glutInitWindowPosition(int x, int y) {
	window_x = x;
	window_y = y;
}

FGAPI void FGAPIENTRY
glutInitWindowSize(int width, int height) {
	window_width = width;
	window_height = height;
}

FGAPI void FGAPIENTRY
glutInitDisplayMode(unsigned int displayMode) {
	display_mode = displayMode;
}

FGAPI int FGAPIENTRY
glutCreateWindow(const char* title) {
	assert((!display && !screen && !wm_delete_window) ||
           (display && screen && wm_delete_window));

    /* open connection with the server */
	if (!display) {
		display = XOpenDisplay(NULL);
		if (!display) {
			return 0;
		}
		screen = DefaultScreen(display);
        wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
	}

    Window window = XCreateSimpleWindow(
		display, RootWindow(display, screen), window_x, window_y, window_width,
		window_height, 1, BlackPixel(display, screen), WhitePixel(display, screen)
	);
    XStoreName(display, window, title);

	Atom atom = XInternAtom(display, "WM_WINDOW_ROLE", False);
	XChangeProperty(display, window, atom, XA_STRING, 8, PropModeReplace,
					reinterpret_cast<const unsigned char*>("pop-up"), 6);
    XSelectInput(display, window, ExposureMask | KeyPressMask);
    XSetWMProtocols(display, window, &wm_delete_window, 1);
    XMapWindow(display, window);
	
    void* gl = kgl_CreateContext(display, window);
    if (!gl) {
        LOG("Could not create OpenGL context!\n");
        return 0;
    }
	windows[next_window_id] = new GlutWindow(window, gl);
	current_window = next_window_id;
	return next_window_id++;
}

FGAPI void FGAPIENTRY
glutMainLoop() {
    XEvent event;
    std::puts("Hey!");
    for (;;) {
        XNextEvent(display, &event);
        if (event.type == Expose) {
            current()->display_func();
        } else if (event.type == ClientMessage) {
            if ((Atom)event.xclient.data.l[0] == wm_delete_window) {
                std::puts("break!");
                break;
            }
        }
        ///* exit on key press */
        //if (event.type == KeyPress)
        //    break;
    }
    kgl_DestroyContext(current()->gl);
    XCloseDisplay(display);
    std::puts("Closed!");
}

FGAPI void FGAPIENTRY
glutDisplayFunc(DisplayFunc fn) {
	current()->display_func = fn;
}
