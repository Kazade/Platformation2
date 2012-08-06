#include "canvas.h"

namespace pn {

Canvas::Canvas(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder>& builder):
    GtkGLWidget(cobject) {

    selection_.reset(new kglt::SelectionRenderer());

    scene().remove_all_passes();
    scene().add_pass(selection_);
    scene().add_pass(kglt::Renderer::ptr(new kglt::GenericRenderer()));

    signal_button_press_event().connect(
        sigc::mem_fun(this, &Canvas::mouse_button_pressed_cb)
    );
}

void Canvas::do_render() {
    scene().camera().move_to((ortho_width() / 2.0), 0, 0);
    update();
}

void Canvas::do_init() {
    scene().render_options.texture_enabled = true;
    scene().pass().viewport().set_background_colour(kglt::Colour(0.2078, 0.494, 0.78, 0.5));

    L_DEBUG("Initializing the editor view");
}

void Canvas::do_resize(int width, int height) {
    set_width(get_allocation().get_width());
    set_height(get_allocation().get_height());

    scene().pass(0).viewport().set_size(width, height);
    ortho_width_ = scene().active_camera().set_orthographic_projection_from_height(15.0, double(width) / double(height));
    scene().pass(1).viewport().set_size(width, height);
    //scene().pass(1).renderer().set_orthographic_projection_from_height(15.0, double(width) / double(height));
}

bool Canvas::mouse_button_pressed_cb(GdkEventButton* event) {
    kglt::MeshID selected = selection_->selected_mesh();
    if(selected) {
        signal_mesh_selected_(selected);
    }
}

}
