#include <glibmm/i18n.h>
#include "layer.h"
#include "level.h"

namespace pn {

Layer::Layer(Level& parent):
    parent_(parent),
    name_(_("Untitled")),
    mesh_container_(0) {

    resize(parent.horizontal_tile_count(), parent.vertical_tile_count());


}

void Layer::resize(uint32_t new_width, uint32_t new_height) {
    //FIXME: Should preserve existing tiles
    tiles_.resize(new_width * new_height, TileInstance());
}

void Layer::add_to_scene(kglt::Scene& scene) {
    mesh_container_ = scene.new_mesh();

    for(uint32_t z = 0; z < parent_.vertical_tile_count(); ++z) {
        for(uint32_t x = 0; x < parent_.horizontal_tile_count(); ++x) {
            TileInstance& instance = tiles_[(z * parent_.horizontal_tile_count()) + x];
            kglt::MeshID new_mesh = scene.new_mesh();
            instance.mesh_id = new_mesh;
            instance.border_mesh_id = scene.new_mesh();

            kglt::Mesh& mesh = scene.mesh(new_mesh);
            kglt::procedural::mesh::rectangle(mesh, 1.0, 1.0);

            kglt::Mesh& border_mesh = scene.mesh(instance.border_mesh_id);
            kglt::procedural::mesh::rectangle_outline(border_mesh, 1.0, 1.0);

            border_mesh.set_parent(&mesh);

            mesh.move_to(float(x), float(z) - (float(parent_.vertical_tile_count()) / 2.0), -1.0 - (0.1 * (float) zindex()));
            border_mesh.move_to(0, 0, 0.01);
        }
    }
}

void Layer::remove_from_scene(kglt::Scene& scene) {

}

}
