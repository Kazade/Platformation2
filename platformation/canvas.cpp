#include "canvas.h"

namespace pn {

Canvas::Canvas(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder>& builder):
    GtkGLWidget(cobject),
    ortho_height_(2.0),
    active_instance_(nullptr) {

    signal_mesh_selected().connect(sigc::mem_fun(this, &Canvas::mesh_selected_callback));
}

void Canvas::do_render() {
    //scene().active_camera().move_to((ortho_width() / 2.0), 0, 0);
    update();
}

void Canvas::do_init() {
    L_DEBUG("Initializing the editor view");

    selection_ = kglt::SelectionRenderer::create();

    add_events(Gdk::SCROLL_MASK);

    scene().remove_all_passes();
    scene().add_pass(selection_);
    scene().add_pass(kglt::GenericRenderer::create());

    signal_scroll_event().connect(
        sigc::mem_fun(this, &Canvas::scroll_event_callback)
    );

    signal_button_press_event().connect(
        sigc::mem_fun(this, &Canvas::mouse_button_pressed_cb)
    );

    scene().render_options.texture_enabled = true;
    scene().pass(1).viewport().set_background_colour(kglt::Colour(0.2078, 0.494, 0.78, 0.5));

    L_DEBUG("Initializing the tile chooser");

    tile_chooser_.reset(new TileChooser(scene()));

    tile_chooser().signal_selection_changed().connect(
        sigc::mem_fun(this, &Canvas::tile_selection_changed_callback)
    );

    //Must happen after the canvas as been created
    level_.reset(new Level(scene()));

    signal_post_init_();
}

void Canvas::do_resize(int width, int height) {
    set_width(get_allocation().get_width());
    set_height(get_allocation().get_height());

    if(scene().pass_count() < 2) {
        return;
    }

    scene().pass(0).viewport().set_size(width, height);
    ortho_width_ = scene().active_camera().set_orthographic_projection_from_height(ortho_height_, double(width) / double(height));
    scene().pass(1).viewport().set_size(width, height);
    //scene().pass(1).renderer().set_orthographic_projection_from_height(15.0, double(width) / double(height));
}

bool Canvas::mouse_button_pressed_cb(GdkEventButton* event) {
    kglt::MeshID selected = selection_->selected_mesh();
    if(selected) {
        signal_mesh_selected_(selected);
    }

    return true;
}

void Canvas::mesh_selected_callback(kglt::MeshID mesh_id) {
    L_DEBUG("Mesh selected: " + boost::lexical_cast<std::string>(mesh_id));
    kglt::Mesh& m = scene().mesh(mesh_id);
    if(m.has_user_data()) {
        try {
            //Try and cast to a tile instance
            TileInstance* tile_instance = m.user_data<TileInstance*>();
            assert(tile_instance);

            //If this mesh is part of a tile_instance
            set_active_tile_instance(tile_instance);
        } catch(boost::bad_any_cast& e) {
            //If we can't then it's probably a tile chooser tile
            try {
                TileChooser* tile_chooser = m.user_data<TileChooser*>();
                tile_chooser->set_selected_by_mesh_id(mesh_id);
            } catch (boost::bad_any_cast& e) {
                L_WARN("Unknown user_data (not a TileChooser* or TileInstance*)");
            }
        }
    }
}

void Canvas::set_active_tile_instance(TileInstance* instance) {
    if(active_instance_) {
        kglt::Mesh& old_border = scene().mesh(active_instance_->border_mesh_id);
        old_border.move_to(0, 0, 0.1);
        old_border.set_diffuse_colour(kglt::Colour(0.8, 0.8, 0.8, 0.5));
    }

    active_instance_ = instance;
    kglt::Mesh& border = scene().mesh(active_instance_->border_mesh_id);
    border.set_diffuse_colour(kglt::Colour(0.0, 0.0, 1.0, 1.0));
    border.move_to(0, 0, 0.2);
}

void Canvas::tile_selection_changed_callback(TileChooserEntry entry) {
    if(active_instance_) {
        if(active_instance_->tile_chooser_entry_id) {
            level().decrease_texture_refcount(
                tile_chooser().entry_by_id(active_instance_->tile_chooser_entry_id).abs_path
            );
        }

        kglt::Mesh& mesh = scene().mesh(active_instance_->mesh_id);
        active_instance_->tile_chooser_entry_id = entry.id;
        mesh.apply_texture(entry.texture_id);
        mesh.set_diffuse_colour(kglt::Colour(1, 1, 1, 1));

        level().increase_texture_refcount(entry.abs_path);
    }
}

}
