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

    bool mouse_button_pressed_cb(GdkEventButton* event);

    sigc::signal<void, kglt::MeshID>& signal_mesh_selected() { return signal_mesh_selected_; }


    bool scroll_event_callback(GdkEventScroll* scroll_event) {
        L_DEBUG("Scroll event received");

        GdkModifierType modifiers = gtk_accelerator_get_default_mod_mask();

        if((scroll_event->state & modifiers) == GDK_CONTROL_MASK) {
            if(scroll_event->direction == GDK_SCROLL_UP || scroll_event->direction == GDK_SCROLL_DOWN) {
                if(scroll_event->direction == GDK_SCROLL_UP) {
                    ortho_height_ -= 0.2;
                } else {
                    ortho_height_ += 0.2;
                }
                ortho_width_ = scene().active_camera().set_orthographic_projection_from_height(
                    ortho_height_, double(width()) / double(height())
                );                
                return true;
            }
        }

        return false;
    }


private:
    void do_init();
    void do_resize(int width, int height);
    void do_render();

    kglt::SelectionRenderer::ptr selection_;

    double ortho_width_;
    double ortho_height_;

    sigc::signal<void, kglt::MeshID> signal_mesh_selected_;    
};

}

#endif // CANVAS_H
