#ifndef CANVAS_H
#define CANVAS_H

#include <gtkmm.h>
#include "gtkgl/gtk_gl_widget.h"

#include "kglt/window_base.h"
#include "kglt/kglt.h"

namespace pn {

class Canvas : public GtkGLWidget, public kglt::WindowBase {
public:
    Canvas(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);

    virtual void swap_buffers() {
        swap_gl_buffers();
    }

    virtual sigc::signal<void, kglt::KeyCode>& signal_key_down() { assert(0); }
    virtual sigc::signal<void, kglt::KeyCode>& signal_key_up() { assert(0); }
    virtual void set_title(const std::string& title) {}
    virtual void check_events() {}

    virtual void cursor_position(int32_t& mouse_x, int32_t& mouse_y) {
        get_pointer(mouse_x, mouse_y);
    }

    double ortho_width() const { return ortho_width_; }

private:
    void do_init();
    void do_resize(int width, int height);
    void do_render();

    kglt::SelectionRenderer::ptr selection_;

    double ortho_width_;
};

}

#endif // CANVAS_H
