#ifndef TILE_CHOOSER_H
#define TILE_CHOOSER_H

#include <map>
#include <set>
#include <tr1/memory>
#include <string>

#include "kglt/kglt.h"

namespace pn {

struct TileChooserEntry {
    kglt::TextureID texture_id;
    kglt::MeshID mesh_id;
    std::string directory;
    std::string abs_path;
};

class TileChooser {
public:
    typedef std::tr1::shared_ptr<TileChooser> ptr;

    TileChooser(kglt::Scene& scene);
    void add_directory(const std::string& tile_directory);
    void remove_directory(const std::string& tile_directory);

    std::set<std::string> directories() const { return directories_; }

    sigc::signal<void>& signal_locations_changed() { return signal_locations_changed_; }

private:
    kglt::Scene& scene_;

    std::set<std::string> directories_;
    std::vector<TileChooserEntry> entries_;

    sigc::signal<void> signal_locations_changed_;
};

}

#endif // TILE_CHOOSER_H
