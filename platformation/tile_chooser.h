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
    sigc::signal<void, float>& signal_tile_loaded() { return signal_tile_loaded_; }
    sigc::signal<void, TileChooserEntry>& signal_selection_changed() { return signal_selection_changed_; }

    void next();
    void previous();

    void set_selected_by_mesh_id(kglt::MeshID mesh_id) {
        uint32_t i = 0;
        for(TileChooserEntry entry: entries_) {
            if(entry.mesh_id == mesh_id) {
                break;
            }
            ++i;
        }

        assert(i != entries_.size());

        if(current_selection_ < i) {
            while(current_selection_ < i) {
                next();
            }
        } else if (current_selection_ > i) {
            while(current_selection_ > i) {
                previous();
            }
        } else {
            //It's already selected, but fire a changed signal anyway
            signal_selection_changed_(entries_[i]);
        }
    }

private:
    kglt::Scene& scene_;
    kglt::MeshID group_mesh_;
    kglt::MeshID slider_group_mesh_;

    std::set<std::string> directories_;
    std::vector<TileChooserEntry> entries_;

    sigc::signal<void> signal_locations_changed_;
    sigc::signal<void, float> signal_tile_loaded_;
    sigc::signal<void, TileChooserEntry> signal_selection_changed_;

    uint32_t current_selection_;

    void update_hidden_tiles();
};

}

#endif // TILE_CHOOSER_H
