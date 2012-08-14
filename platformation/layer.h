#ifndef LAYER_H
#define LAYER_H

#include <string>
#include <tr1/memory>

#include <kglt/kglt.h>
#include "tile_chooser.h"

namespace pn {

class Level;
class Layer;

struct TileInstance {
    TileInstance():
        tile_chooser_entry_id(0),
        tile_image_id(-1) {

    }

    TileChooserEntryID tile_chooser_entry_id;
    int32_t tile_image_id;
    kglt::MeshID mesh_id;
    kglt::MeshID border_mesh_id;
};

class Layer {
public:
    typedef std::tr1::shared_ptr<Layer> ptr;

    Layer(Level& parent);
    std::string name() const { return name_; }

    void add_to_scene(kglt::Scene& scene);
    void remove_from_scene(kglt::Scene& scene);

    void set_zindex(int32_t zindex) { zindex_ = zindex; }
    int32_t zindex() const { return zindex_; }

    void resize(uint32_t new_width, uint32_t new_height);

    TileInstance& tile_at(uint32_t idx) { return tiles_.at(idx); }
private:
    Level& parent_;

    std::string name_;
    int32_t zindex_;

    std::vector<TileInstance> tiles_;

    kglt::MeshID mesh_container_;
};

}

#endif // LAYER_H
