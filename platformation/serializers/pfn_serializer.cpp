#include "kazbase/json/json.h"
#include "kazbase/xrange.h"

#include "pfn_serializer.h"

#include "../layer.h"

/**
    LEVEL FORMAT:

    "level" : {
        "width" : 40,
        "height" : 10
    },
    "textures" : [
        "tiles/thing/something/texture.png",
        "tiles/thing/something_else/texture.png"
    ],
    "layers" : [
        {
            "tiles" : [ 0, 1, 0, 1, 1, 1, 0 ... ] #Texture indexes from top left to bottom right
        }
    ]
*/

namespace pn {

PFNSerializer::~PFNSerializer() {

}

bool PFNSerializer::save_to(const std::string& filename) {
    json::JSON output;

    json::Node& level_dict = output.insert_dict("level");
    json::Node& tex_list = output.insert_array("textures");
    json::Node& layer_list = output.insert_array("layers");

    level_dict.insert_value("name").set(level().name());
    level_dict.insert_value("width").set_number(level().horizontal_tile_count());
    level_dict.insert_value("height").set_number(level().vertical_tile_count());

    for(std::string texture: level().active_textures()) {
        tex_list.append_value().set(texture);
    }

    for(uint32_t i: xrange(level().layer_count())) {
        Layer& layer = level().layer_at(i);

        json::Node& layer_dict = layer_list.append_dict();
        layer_dict.insert_value("name").set(layer.name());
        json::Node& tile_list = layer_dict.insert_array("tiles");

        uint32_t total = level().horizontal_tile_count() * level().vertical_tile_count();
        for(uint32_t j: xrange(total)) {
            json::Node& tile = tile_list.append_dict();
            tile.insert_value("position").set_number(j);
            tile.insert_value("texture").set_number(layer.tile_at(j).tile_image_id);
            //TODO: if breakable: tile.insert_value("breakable").set_bool(true);
        }
    }

    std::string content = json::dumps(output);

    std::ofstream file_out(filename.c_str());
    file_out << content;

    return true;
}

bool PFNSerializer::load_from(const std::string& filename) {
    assert(0 && "Not implemented");
}

}
