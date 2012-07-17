#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <gtkmm.h>

#include "canvas.h"
#include "level.h"
#include "tile_chooser.h"

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


class MainWindow : public Gtk::Window {
public:
    MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);


    //Signals
    void level_name_box_changed_cb() {
        level_->set_name(ui<Gtk::Entry>("level_name_box")->get_text());
    }

    void level_layers_changed_cb();
    void layer_selection_changed_cb();

    void add_layer_button_clicked_cb() {
        level_->add_layer();
    }

    void remove_layer_button_clicked_cb() {
        Gtk::TreeView* view = ui<Gtk::TreeView>("layer_list");

        Glib::RefPtr<Gtk::TreeSelection> selection = view->get_selection();
        Gtk::TreeModel::iterator iter = selection->get_selected();

        if(iter) {
            //Only remove the active layer if something is selected
            level_->remove_layer(level_->active_layer());
        }
    }

    void add_tile_location_button_clicked_cb() {
        Gtk::FileChooserDialog fd("Please choose a folder", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);

        fd.set_transient_for(*this);
        fd.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
        fd.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);

        int result = fd.run();

        if(result == Gtk::RESPONSE_OK) {
            fd.hide();
            ui<Gtk::ProgressBar>("progress_bar")->show();
            tile_chooser_->add_directory(fd.get_filename());
            ui<Gtk::ProgressBar>("progress_bar")->hide();
        }
    }

    void tile_location_changed_cb() {
        Gtk::TreeView* view = ui<Gtk::TreeView>("tile_location_list");

        tile_location_list_model_->clear();

        int32_t i = 0;
        for(std::string directory: tile_chooser_->directories()) {
            Gtk::TreeModel::Row row = *(tile_location_list_model_->append());
            row[tile_location_list_columns_.column_id] = i++;
            row[tile_location_list_columns_.folder] = directory;
        }

        save_tile_locations();
    }

    void tile_loaded_cb(float percentage_done) {
        ui<Gtk::ProgressBar>("progress_bar")->set_fraction(percentage_done / 100.0);

        //Run a few events to keep things reponsive without slowing down too much (while events_pending() seems to just grind to a halt)
        int counter = 3;
        while(counter--) {
            Gtk::Main::iteration(false);
        }
    }

    void save_tile_locations();
    void load_tile_locations();

private:
    const Glib::RefPtr<Gtk::Builder>& builder_;
    Canvas* canvas_;
    TileChooser::ptr tile_chooser_;

    Level::ptr level_;

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
};

}

#endif // MAIN_WINDOW_H
