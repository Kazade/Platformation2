#include "canvas.h"

namespace pn {

Canvas::Canvas(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder>& builder):
    GtkGLWidget(cobject) {

    selection_.reset(new kglt::SelectionRenderer(scene()));

    scene().remove_all_passes();
    scene().add_pass(selection_);
    scene().add_pass(kglt::Renderer::ptr(new kglt::GenericRenderer(scene())));
}

void Canvas::do_render() {
    update();
}

void Canvas::do_init() {
    scene().render_options.backface_culling_enabled = false;
    scene().render_options.texture_enabled = true;
    //scene().render_options.wireframe_enabled = true;
    scene().pass().viewport().set_background_colour(kglt::Colour(0.2078, 0.494, 0.78, 0.5));

    L_DEBUG("Initializing the editor view");
}

void Canvas::do_resize(int width, int height) {
    set_width(get_allocation().get_width());
    set_height(get_allocation().get_height());

    scene().pass(0).viewport().set_size(width, height);
    scene().pass(0).renderer().set_orthographic_projection_from_height(15.0, double(width) / double(height));
    scene().pass(1).viewport().set_size(width, height);
    scene().pass(1).renderer().set_orthographic_projection_from_height(15.0, double(width) / double(height));
}

}
