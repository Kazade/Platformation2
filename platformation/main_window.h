#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <gtkmm.h>

#include "canvas.h"
#include "level.h"

namespace pn {

struct LayerListColumns : public Gtk::TreeModel::ColumnRecord {
    LayerListColumns() { add(column_id_); add(column_name_); add(checked_); }
    Gtk::TreeModelColumn<int> column_id_;
    Gtk::TreeModelColumn<Glib::ustring> column_name_;
    Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > checked_;
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

private:
    const Glib::RefPtr<Gtk::Builder>& builder_;
    Canvas* canvas_;

    Level::ptr level_;

    LayerListColumns layer_list_columns_;
    Glib::RefPtr<Gtk::TreeStore> layer_list_model_;

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
};

}

#endif // MAIN_WINDOW_H
