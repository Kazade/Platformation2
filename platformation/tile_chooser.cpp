
#include "tile_chooser.h"
#include "kazbase/os/path.h"
#include "kazbase/string.h"
#include "kazbase/logging/logging.h"
#include "kglt/shortcuts.h"

namespace pn {

const float TILE_CHOOSER_WIDTH = 2.0;
const float TILE_CHOOSER_SPACING = 0.1;

TileChooser::TileChooser(kglt::Scene& scene):
    scene_(scene),
    current_selection_(0) {

    group_mesh_ = scene.new_mesh();
    kglt::Mesh& m = scene.mesh(group_mesh_);
    m.set_visible(false);

    kglt::Mesh& selected_outline = kglt::return_new_mesh(scene_);
    selected_outline.set_parent(&m);
    kglt::procedural::mesh::rectangle_outline(selected_outline, TILE_CHOOSER_WIDTH, TILE_CHOOSER_WIDTH);
    selected_outline.set_diffuse_colour(kglt::Colour(1.0, 0, 0, 1.0));
    selected_outline.move_to(0, 0, 0.1);

    slider_group_mesh_ = scene.new_mesh();
    kglt::Mesh& slider = scene.mesh(slider_group_mesh_);
    slider.set_parent(&m);

    //Attach the group mesh to the camera, so we are always relative to it

    m.set_parent(&scene.camera());

    scene_.signal_render_pass_started().connect(sigc::mem_fun(this, &TileChooser::pass_started_callback));
}

void TileChooser::pass_started_callback(kglt::Pass& pass) {
    //We need to do this in a signal so that we know when the frustum has been initialized
    if (scene_.camera().frustum().initialized()) {
        kglt::Mesh& m = scene_.mesh(group_mesh_);
        m.move_to(0, -(scene_.camera().frustum().near_height() / 2.0) + 1.25, 0);
        m.set_visible(true);
    }
}

void TileChooser::next() {
    if(current_selection_ >= entries_.size() - 1) {
        return;
    }
    current_selection_++;

    kglt::Mesh& slider = scene_.mesh(slider_group_mesh_);
    slider.move_to(-(TILE_CHOOSER_WIDTH + TILE_CHOOSER_SPACING) * current_selection_, 0.0, 0.0);
    update_hidden_tiles();
    signal_selection_changed_(entries_[current_selection_]);
}

void TileChooser::previous() {
    if(current_selection_ < 1) {
        return;
    }

    current_selection_--;

    kglt::Mesh& slider = scene_.mesh(slider_group_mesh_);
    slider.move_to(-(TILE_CHOOSER_WIDTH + TILE_CHOOSER_SPACING) * current_selection_, 0.0, 0.0);
    update_hidden_tiles();

    signal_selection_changed_(entries_[current_selection_]);
}

void TileChooser::add_directory(const std::string& tile_directory) {
    L_INFO("Adding tileset directory " + tile_directory);

    assert(os::path::is_dir(tile_directory));

    directories_.insert(tile_directory);

    std::vector<std::string> to_load;

    //FIXME: Should make recursive and store a relative path to the root
    for(std::string file: os::path::list_dir(tile_directory)) {
        if(str::ends_with(file, ".png")) {
            std::string abs_path = os::path::join(tile_directory, file);
            to_load.push_back(abs_path);
        }
    }

    int i = 0;

    kglt::Mesh& slider = scene_.mesh(slider_group_mesh_);

    for(std::string abs_path: to_load) {
        TileChooserEntry new_entry;

        new_entry.mesh_id = scene_.new_mesh();
        new_entry.texture_id = kglt::create_texture_from_file(scene_.window(), abs_path);
        new_entry.abs_path = abs_path;
        new_entry.directory = tile_directory;

        kglt::Mesh& m = scene_.mesh(new_entry.mesh_id);
        kglt::procedural::mesh::rectangle(m, TILE_CHOOSER_WIDTH, TILE_CHOOSER_WIDTH);
        m.apply_texture(new_entry.texture_id);
        m.set_user_data((TileChooser*)this);

        //Set the parent of this mesh to the slider group mesh
        m.set_parent(&slider);
        m.move_to(i * (TILE_CHOOSER_WIDTH + TILE_CHOOSER_SPACING), 0, 0);

        entries_.push_back(new_entry);
        update_hidden_tiles(); //FIXME: This is slow as arse

        ++i;
        signal_tile_loaded_((100.0 / float(to_load.size())) * float(i));


    }

    update_hidden_tiles();
    signal_locations_changed_(); //Fire off the locations changed signal
}

void TileChooser::remove_directory(const std::string& tile_directory) {
    assert(container::contains(directories_, tile_directory));

    /*
       FIXME: We should see if these tiles are in use, if they are we should
       give the option to cancel the remove. If the remove isn't cancelled
       then those tiles should be reset to have no texture. I guess...
    */

    //Delete the meshes relating to these entries
    for(TileChooserEntry& entry: entries_) {
        if(entry.directory == tile_directory) {
            scene_.delete_mesh(entry.mesh_id);
        }
    }

    //Remove all the entries that have this as the root directory
    entries_.erase(std::remove_if(
            entries_.begin(), entries_.end(),
            [=](const TileChooserEntry& entry) { return entry.directory == tile_directory; }
        ), entries_.end()
    );

    //Erase the directory itself
    directories_.erase(tile_directory);
    signal_locations_changed_(); //Fire off the locations changed signal
}

void TileChooser::update_hidden_tiles() {
    /**
       Basically, we want to hide the tiles that are more than 5
       tiles away from the current selection, and show the ones that
       are less or equal to that.
    */

    int32_t left = std::max((int32_t)0, int32_t(this->current_selection_) - 5);
    int32_t right = std::min((int32_t)entries_.size(), int32_t(this->current_selection_) + 5);

    for(uint32_t i = 0; i < entries_.size(); ++i) {
        kglt::Mesh& m = scene_.mesh(entries_[i].mesh_id);
        if(i >= left && i <= right) {
            m.set_visible(true);

            float a = 1.0; //1.0 - ((1.0 / 5.0) * (fabs(i - current_selection_)));
            m.set_diffuse_colour(kglt::Colour(1.0, 1.0, 1.0, a));
        } else {
            m.set_visible(false);
            m.set_diffuse_colour(kglt::Colour(1.0, 1.0, 1.0, 1.0));
        }
    }
}

}
