#ifndef LEVEL_H
#define LEVEL_H

#include <string>
#include <vector>
#include <cstdint>
#include <sigc++/sigc++.h>

#include <tr1/memory>

#include <kglt/kglt.h>

namespace pn {

class Layer;

class Level {
public:
    typedef std::tr1::shared_ptr<Level> ptr;

    Level(kglt::Scene& scene);

    void set_active_layer(uint32_t active) { active_layer_ = active; }
    uint32_t active_layer() const { return active_layer_; }
    void set_name(const std::string& name) { name_ = name; }
    std::string name() const { return name_; }

    uint32_t layer_count() const;
    Layer& layer_at(uint32_t idx);
    void add_layer();
    void remove_layer(uint32_t idx);

    sigc::signal<void>& signal_layers_changed() {
        return signal_layers_changed_;
    }

    uint32_t horizontal_tile_count() const;
    uint32_t vertical_tile_count() const;

    void increase_texture_refcount(const std::string& texture_path) {
        if(texture_ref_count_.find(texture_path) != texture_ref_count_.end()) {
            texture_ref_count_[texture_path]++;
        } else {
            texture_ref_count_[texture_path] = 1;
        }
    }

    void decrease_texture_refcount(const std::string& texture_path) {
        assert(texture_ref_count_.find(texture_path) != texture_ref_count_.end());
        texture_ref_count_[texture_path]--;
        if(texture_ref_count_[texture_path] == 0) {
            texture_ref_count_.erase(texture_path);
        }
    }

    std::set<std::string> active_textures() const {
        return container::keys(texture_ref_count_);
    }

private:
    kglt::Scene& scene_;

    std::string name_;
    uint32_t active_layer_;
    std::vector<std::tr1::shared_ptr<Layer> > layers_;

    uint32_t horizontal_tile_count_;
    uint32_t vertical_tile_count_;

    sigc::signal<void> signal_layers_changed_;

    std::map<std::string, uint32_t> texture_ref_count_;

};

}

#endif // LEVEL_H
