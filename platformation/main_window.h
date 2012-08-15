#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <gtkmm.h>

#include "kazbase/logging/logging.h"
#include "canvas.h"
#include "level.h"
#include "tile_chooser.h"
#include "layer.h"
#include "user_data_types.h"
#include "serializers/pfn_serializer.h"

namespace pn {

struct LayerListColumns : public Gtk::TreeModel::ColumnRecord {
    LayerListColumns() { add(column_id_); add(column_name_); add(checked_); }
    Gtk::TreeModelColumn<int> column_id_;
    Gtk::TreeModelColumn<Glib::ustring> column_name_;
    Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > checked_;
};

struct TileLocationListColumns : public Gtk::TreeModel::ColumnRecord {
    TileLocationListColumns() { add(column_id); add(folder); }
    Gtk::TreeModelColumn<int> column_id;
    Gtk::TreeModelColumn<Glib::ustring> folder;
};

enum EditMode {
    EDIT_MODE_TILE,
    EDIT_MODE_GEOMETRY
};

class MainWindow : public Gtk::Window {
public:
    MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);


    //Signals
    void level_name_box_changed_cb() {
        canvas_->level().set_name(ui<Gtk::Entry>("level_name_box")->get_text());
    }

    void level_layers_changed_cb();
    void layer_selection_changed_cb();

    void scrollbar_value_changed();

    bool canvas_scroll_event(GdkEventScroll* scroll_event);

    void recalculate_scrollbars(kglt::Pass& pass);
    void add_layer_button_clicked_cb();
    void remove_layer_button_clicked_cb();

    void add_tile_location_button_clicked_cb();
    void remove_tile_location_button_clicked_cb();
    void tile_location_changed_cb();
    void tile_loaded_cb(float percentage_done);
    void save_tile_locations();
    void load_tile_locations();
    bool key_press_event_cb(GdkEventKey* key);
    void post_canvas_init();
    void save_button_clicked_callback();

    void set_mode(EditMode mode);

private:
    const Glib::RefPtr<Gtk::Builder>& builder_;
    Canvas* canvas_;    

    LayerListColumns layer_list_columns_;
    Glib::RefPtr<Gtk::TreeStore> layer_list_model_;

    TileLocationListColumns tile_location_list_columns_;
    Glib::RefPtr<Gtk::TreeStore> tile_location_list_model_;

    template<typename T>
    T* ui(const std::string& name) {
        std::map<std::string, Gtk::Widget*>::iterator it = widget_cache_.find(name);
        if(it != widget_cache_.end()) {
            return dynamic_cast<T*>(it->second);
        }

        T* result;
        builder_->get_widget(name, result);
        widget_cache_[name] = result;
        return result;
    }

    std::map<std::string, Gtk::Widget*> widget_cache_;

    void _create_layer_list_model();
    void _create_tile_location_list_model();
    void _generate_blank_config();

    EditMode current_mode_;
};

}

#endif // MAIN_WINDOW_H
