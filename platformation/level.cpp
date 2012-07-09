#include "level.h"
#include "layer.h"

#include <glibmm/i18n.h>

namespace pn {

Level::Level(kglt::Scene& scene):
    scene_(scene),
    name_(_("Untitled")),
    active_layer_(0),
    horizontal_tile_count_(40),
    vertical_tile_count_(10) {

    add_layer();
}

uint32_t Level::horizontal_tile_count() const {
    return horizontal_tile_count_;
}

uint32_t Level::vertical_tile_count() const {
    return vertical_tile_count_;
}

uint32_t Level::layer_count() const {
    return layers_.size();
}

Layer& Level::layer_at(uint32_t idx) {
    return *layers_.at(idx);
}

void Level::add_layer() {
    layers_.push_back(Layer::ptr(new Layer(*this)));

    //Set the zindex
    layer_at(layer_count() - 1).set_zindex(layer_count());
    layer_at(layer_count() - 1).add_to_scene(scene_);

    signal_layers_changed_();
}

void Level::remove_layer(uint32_t idx) {        
    layer_at(idx).remove_from_scene(scene_);
    layers_.erase(layers_.begin() + idx);

    //Rebuild the zindex
    for(uint32_t i = 0; i < layer_count(); ++i) {
        layer_at(i).set_zindex(i);
    }

    set_active_layer(0);
    signal_layers_changed_();    
}

}
