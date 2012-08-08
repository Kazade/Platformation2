#include <glibmm/i18n.h>
#include "layer.h"
#include "level.h"
#include "user_data_types.h"

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
    scene.mesh(mesh_container_).move_to(-(float(parent_.horizontal_tile_count()) / 2.0f), 0.0f, 0.0f);

    for(uint32_t z = 0; z < parent_.vertical_tile_count(); ++z) {
        for(uint32_t x = 0; x < parent_.horizontal_tile_count(); ++x) {
            TileInstance& instance = tiles_[(z * parent_.horizontal_tile_count()) + x];
            kglt::MeshID new_mesh = scene.new_mesh();
            instance.mesh_id = new_mesh;
            instance.border_mesh_id = scene.new_mesh();

            //Generate a mesh for the tile
            kglt::Mesh& mesh = scene.mesh(new_mesh);
            kglt::procedural::mesh::rectangle(mesh, 1.0, 1.0, 0.5, 0.5);
            mesh.set_user_data(&instance); //Set the extra data on the mesh to point to this tile instance
            mesh.set_diffuse_colour(kglt::Colour(0, 0, 0, 0));

            //Generate a mesh for the border
            kglt::Mesh& border_mesh = scene.mesh(instance.border_mesh_id);
            kglt::procedural::mesh::rectangle_outline(border_mesh, 1.0, 1.0, 0.5, 0.5);

            border_mesh.set_parent(&mesh);
            //border_mesh.set_visible(false);

            mesh.move_to(float(x), float(z) - (float(parent_.vertical_tile_count()) / 2.0), -1.0 - (0.1 * (float) zindex()));
            border_mesh.move_to(0, 0, 0.01); //Move the border mesh slightly forward

            mesh.set_parent(&scene.mesh(mesh_container_));
        }
    }
}

void Layer::remove_from_scene(kglt::Scene& scene) {

}

}
