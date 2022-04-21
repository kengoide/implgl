// vim: set ts=4 sw=4 sts=4 et:

#include <X11/Xlib.h>

void*
kgl_CreateContext(Display* display, Window window);

void
kgl_DestroyContext(void* context);
