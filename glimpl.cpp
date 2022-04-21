// vim: set ts=4 sw=4 sts=4 et:

#include "glimpl.h"

#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

#include <GL/gl.h>
#include <X11/Xlib.h>

#include <glm/glm.hpp>

std::ostream& debug = std::cout;

struct WindowCoordinate {
    int x;
    int y;
    float z;
    
    const glm::ivec2& as_ivec2() const {
        return *reinterpret_cast<const glm::ivec2*>(this);
    }
};

inline std::ostream&
operator<<(std::ostream& stream, const glm::vec4& v) {
    stream << "vec4(" << v.x << ',' << v.y << ',' << v.z << ',' << v.w << ')';
    return stream;
}

inline std::ostream&
operator<<(std::ostream& stream, const glm::vec3& v) {
    stream << "vec3(" << v.x << ',' << v.y << ',' << v.z << ')';
    return stream;
}

inline std::ostream&
operator<<(std::ostream& stream, const WindowCoordinate& c) {
    stream << "winc(" << c.x << ',' << c.y << ',' << c.z << ')';
    return stream;
}

namespace {
const GLenum GL_BEGIN_END_INACTIVE = -1;

class Color {
    float r_;
    float g_;
    float b_;
    float a_;
    unsigned int packed_;
public:
    Color() : r_(), g_(), b_(), a_(), packed_() {}
    Color(float r, float g, float b, float a)
        : r_(r), g_(g), b_(b), a_(a) {
        pack();
    }

    float red() const { return r_; }
    float green() const { return g_; }
    float blue() const { return b_; }
    float alpha() const { return a_; }
    unsigned int packed() const { return packed_; }

    void set(float r, float g, float b, float a) {
        r_ = r;
        g_ = g;
        b_ = b;
        a_ = a;
        pack();
    }
private:
    void pack() {
        packed_ = std::lround(a_ * 255.0f) << 24 |
                  std::lround(r_ * 255.0f) << 16 |
                  std::lround(g_ * 255.0f) << 8 |
                  std::lround(b_ * 255.0f);
    }
};

struct GLContext {
    // X
    Display* display;
	Window window;
    XImage* image;
    GC gc;
    unsigned int* pixels;

    GLenum error;

    struct ViewportState {
        GLint x;
        GLint y;
        GLsizei w;
        GLsizei h;
        GLint cx;
        GLint cy;

        ViewportState(GLint width, GLint height)
            : x(), y(), w(width), h(height), cx(w / 2), cy(h / 2) {
        }
        ViewportState() = delete;
    } viewport;

    glm::mat4 model_view_matrix;
    glm::mat4 projection_matrix;
    glm::mat4 combined_matrix;

    Color clear_color;

    // vertex attributes
    Color color;

    // Begin/End
    glm::vec4 vertices[3];
    int vertex_count;
    GLenum begin_end_state;

	GLContext(Display* display, Window window, XImage* image, GC gc)
        : display(display), window(window), image(image), gc(gc),
          pixels(reinterpret_cast<unsigned int*>(image->data)),
          error(GL_NO_ERROR), viewport(image->width, image->height),
          model_view_matrix(), projection_matrix(),
          combined_matrix(), clear_color(),
          color(1.0f, 1.0f, 1.0f, 1.0f), vertices(), vertex_count(),
          begin_end_state(GL_BEGIN_END_INACTIVE) {
	}
};

GLContext* context;

inline float
clamp(GLclampf n) {
    return std::min(std::max(n, 0.0f), 1.0f);
}

void
error(GLenum error) {
    context->error = error;
    std::cerr << "gl-error: " << std::hex << error << std::endl;
}

int
orient_2d(const glm::ivec2& a, const glm::ivec2& b, const glm::ivec2& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void
rasterize_triangle(const WindowCoordinate& a,
                   const WindowCoordinate& b,
                   const WindowCoordinate& c) {
    XImage* image = context->image;

    int min_x = std::min(a.x, std::min(b.x, c.x));
    int max_x = std::max(a.x, std::max(b.x, c.x));
    int min_y = std::min(a.y, std::min(b.y, c.y));
    int max_y = std::max(a.y, std::max(b.y, c.y));

    min_x = std::max(min_x, 0);
    max_x = std::min(max_x, image->width - 1);
    min_y = std::max(min_y, 0);
    max_y = std::min(max_y, image->height - 1);

    const glm::ivec2& va = a.as_ivec2();
    bool ccw = orient_2d(va, b.as_ivec2(), c.as_ivec2()) >= 0;
    //debug << "ccw?:" << ccw << std::endl;
    const glm::ivec2& vb = (ccw ? b : c).as_ivec2();
    const glm::ivec2& vc = (ccw ? c : b).as_ivec2();

    glm::ivec2 p;
    for (p.y = min_y; p.y < max_y; ++p.y) {
        for (p.x = min_x; p.x < max_x; ++p.x) {
            int side0 = orient_2d(vb, vc, p);
            int side1 = orient_2d(vc, va, p);
            int side2 = orient_2d(va, vb, p);
            if (side0 >= 0 && side1 >= 0 && side2 >= 0) {
                unsigned int y = (image->height - 1) - p.y;
                context->pixels[y * image->width + p.x] = context->color.packed();
            }
        }
    }
}

inline glm::vec3
divide_by_w(const glm::vec4& v) {
    return glm::vec3(v.x / v.w, v.y / v.w, v.z / v.w);
}

inline WindowCoordinate
viewport_transform(const glm::vec3& v) {
    const GLContext::ViewportState& viewport = context->viewport;
    //std::cout << viewport.w << std::endl;
    WindowCoordinate c = {
        std::lround(v.x * (viewport.w / 2)) + viewport.cx,
        std::lround(v.y * (viewport.h / 2)) + viewport.cy,
        v.z /* FIXME: depth transform */
    };
    return c;
}

void
draw_triangle(const glm::vec4& a, const glm::vec4& b, const glm::vec4& c) {
    glm::vec4 clip_a(context->combined_matrix * a);
    glm::vec4 clip_b(context->combined_matrix * b);
    glm::vec4 clip_c(context->combined_matrix * c);

    glm::vec3 dev_a(divide_by_w(clip_a));
    glm::vec3 dev_b(divide_by_w(clip_b));
    glm::vec3 dev_c(divide_by_w(clip_c));

    WindowCoordinate win_a(viewport_transform(dev_a));
    WindowCoordinate win_b(viewport_transform(dev_b));
    WindowCoordinate win_c(viewport_transform(dev_c));

    std::cout << "object=" << a << ',' << b << ',' << c << std::endl;
    std::cout << "  clip=" << clip_a << ',' << clip_b << ',' << clip_c << std::endl;
    std::cout << "device=" << dev_a << ',' << dev_b << ',' << dev_c << std::endl;
    std::cout << "window=" << win_a << ',' << win_b << ',' << win_c << std::endl;

    rasterize_triangle(win_a, win_b, win_c);
}
}

void
glClear(GLbitfield mask) {
    if (mask & GL_COLOR_BUFFER_BIT) {
        for (int i = 0; i < context->image->width * context->image->height; ++i) {
            context->pixels[i] = context->clear_color.packed();
        }
    }
}

void
glClearColor(GLclampf red, GLclampf green,
			 GLclampf blue, GLclampf alpha) {
    red = clamp(red);
    green = clamp(green);
    blue = clamp(blue);
    alpha = clamp(alpha);

    context->clear_color.set(red, green, blue, alpha);
}

void
glFlush() {
    XImage* image = context->image;
    XPutImage(context->display, context->window, context->gc, image,
              0, 0, 0, 0, image->width, image->height);
}

void
glBegin(GLenum mode) {
    assert(mode == GL_TRIANGLES);
    assert(context->begin_end_state == GL_BEGIN_END_INACTIVE);
    if (context->begin_end_state != GL_BEGIN_END_INACTIVE) {
        error(GL_INVALID_OPERATION);
        return;
    }

    context->begin_end_state = mode;
}

void
glVertex2f(GLfloat x, GLfloat y) {
    glm::vec4* vertices = context->vertices;
    glm::vec4& v = vertices[context->vertex_count];
    v.x = x;
    v.y = y;
    v.z = 0.0f;
    v.w = 1.0f;
    if (++context->vertex_count == 3) {
        draw_triangle(vertices[0], vertices[1], vertices[2]);
        context->vertex_count = 0;
    }
}

void
glEnd() {
    assert(context->begin_end_state != GL_BEGIN_END_INACTIVE);

    context->begin_end_state = GL_BEGIN_END_INACTIVE;
    context->vertex_count = 0;
}

void*
kgl_CreateContext(Display* display, Window window) {
    XWindowAttributes a;
    Status s = XGetWindowAttributes(display, window, &a);
    if (!s) {
        return nullptr;
    }
    assert(a.depth == 24 || a.depth == 32);

    unsigned int* pixels = new unsigned int[a.width * a.height];
    XImage* image = XCreateImage(display, CopyFromParent, a.depth, ZPixmap,
                                 0, reinterpret_cast<char*>(pixels),
                                 a.width, a.height, 32, 0);
    if (!image) {
        return nullptr;
    }

    GLContext* new_context = new GLContext(display, window, 
                                           image, XDefaultGC(display, 0));
    context = new_context;
    return new_context;
}

void
kgl_DestroyContext(void* context) {
    delete static_cast<GLContext*>(context);
}
