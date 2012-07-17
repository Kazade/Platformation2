
#include "tile_chooser.h"
#include "kazbase/os/path.h"
#include "kazbase/string.h"
#include "kazbase/logging/logging.h"
#include "kglt/shortcuts.h"

namespace pn {

TileChooser::TileChooser(kglt::Scene& scene):
    scene_(scene) {

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

    for(std::string abs_path: to_load) {
        TileChooserEntry new_entry;

        new_entry.mesh_id = scene_.new_mesh();
        new_entry.texture_id = kglt::create_texture_from_file(scene_.window(), abs_path);
        new_entry.abs_path = abs_path;
        new_entry.directory = tile_directory;

        kglt::Mesh& m = scene_.mesh(new_entry.mesh_id);
        kglt::procedural::mesh::rectangle(m, 1.0, 1.0);
        m.apply_texture(new_entry.texture_id);

        entries_.push_back(new_entry);

        ++i;
        signal_tile_loaded_((100.0 / float(to_load.size())) * float(i));
    }

    signal_locations_changed_(); //Fire off the locations changed signal
}

void TileChooser::remove_directory(const std::string& tile_directory) {
    assert(container::contains(directories_, tile_directory));

    //Remove all the entries that have this as the root directory
    entries_.erase(std::remove_if(
            entries_.begin(), entries_.end(),
            [=](const TileChooserEntry& entry) { return entry.directory == tile_directory; }
        ), entries_.end()
    );

    //Erase the directory itself
    directories_.erase(tile_directory);
}

}
