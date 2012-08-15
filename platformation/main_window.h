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


class MainWindow : public Gtk::Window {
public:
    MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);


    //Signals
    void level_name_box_changed_cb() {
        level_->set_name(ui<Gtk::Entry>("level_name_box")->get_text());
    }

    void level_layers_changed_cb();
    void layer_selection_changed_cb();

    void scrollbar_value_changed() {
        double x_pos = ui<Gtk::Scrollbar>("main_horizontal_scrollbar")->get_value();
        double y_pos = ui<Gtk::Scrollbar>("main_vertical_scrollbar")->get_value();
        canvas_->scene().active_camera().move_to(x_pos, -y_pos, 0.0);
    }

    bool canvas_scroll_event(GdkEventScroll* scroll_event) {
        GdkModifierType modifiers = gtk_accelerator_get_default_mod_mask();
        if((scroll_event->state & modifiers) == GDK_CONTROL_MASK) {
            //Ignore if CTRL is pressed
            return false;
        }

        if(scroll_event->direction == GDK_SCROLL_UP) {
            double value = ui<Gtk::Scrollbar>("main_vertical_scrollbar")->get_value();
            double step = ui<Gtk::Scrollbar>("main_vertical_scrollbar")->get_adjustment()->get_step_increment();
            ui<Gtk::Scrollbar>("main_vertical_scrollbar")->set_value(value - step);
            return true;
        }
        if (scroll_event->direction == GDK_SCROLL_DOWN) {
            double value = ui<Gtk::Scrollbar>("main_vertical_scrollbar")->get_value();
            double step = ui<Gtk::Scrollbar>("main_vertical_scrollbar")->get_adjustment()->get_step_increment();
            ui<Gtk::Scrollbar>("main_vertical_scrollbar")->set_value(value + step);
            return true;
        }
        return false;
    }

    void recalculate_scrollbars(kglt::Pass& pass) {
        if(!canvas_->scene().active_camera().frustum().initialized()) return;

        double frustum_height = canvas_->scene().active_camera().frustum().near_height();
        double frustum_width = canvas_->scene().active_camera().frustum().near_width();
        double level_height = (double) level_->vertical_tile_count();
        double level_width = (double) level_->horizontal_tile_count();

        Glib::RefPtr<Gtk::Adjustment> vadj = ui<Gtk::Scrollbar>("main_vertical_scrollbar")->get_adjustment();
        if(vadj->get_page_size() != frustum_height) {
            vadj->set_lower(-level_height / 2.0);
            vadj->set_upper(level_height / 2.0 + frustum_height);
            vadj->set_page_size(frustum_height);
            vadj->set_step_increment(0.1);
        }

        Glib::RefPtr<Gtk::Adjustment> hadj = ui<Gtk::Scrollbar>("main_horizontal_scrollbar")->get_adjustment();
        double hpage_size = hadj->get_page_size();
        if(hpage_size != frustum_width) {
            hadj->set_lower(-level_width / 2.0);
            hadj->set_upper(level_width / 2.0 + frustum_width);
            hadj->set_page_size(frustum_width);
            hadj->set_step_increment(0.1);
        }
    }

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
            canvas_->tile_chooser().add_directory(fd.get_filename());
            ui<Gtk::ProgressBar>("progress_bar")->hide();
        }
    }

    void remove_tile_location_button_clicked_cb() {
        Gtk::TreeView* view = ui<Gtk::TreeView>("tile_location_list");
        Gtk::TreeModel::iterator iter = view->get_selection()->get_selected();
        if(iter) {
            std::string path = Glib::ustring((*iter)[tile_location_list_columns_.folder]);
            L_DEBUG("Removing path: " + path);
            canvas_->tile_chooser().remove_directory(path);
        }
    }

    void tile_location_changed_cb() {
        tile_location_list_model_->clear();

        int32_t i = 0;
        for(std::string directory: canvas_->tile_chooser().directories()) {
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

    bool key_press_event_cb(GdkEventKey* key);

    void post_canvas_realize() {
        canvas_->tile_chooser().signal_locations_changed().connect(
            sigc::mem_fun(this, &MainWindow::tile_location_changed_cb)
        );
        canvas_->tile_chooser().signal_tile_loaded().connect(
            sigc::mem_fun(this, &MainWindow::tile_loaded_cb)
        );

        //Must happen after the canvas as been created
        level_.reset(new Level(canvas_->scene()));

        //Watch for layer changes on the level
        level_->signal_layers_changed().connect(
            sigc::mem_fun(this, &MainWindow::level_layers_changed_cb)
        );

        ui<Gtk::Entry>("level_name_box")->set_text(level_->name());

        canvas_->scene().signal_render_pass_started().connect(sigc::mem_fun(this, &MainWindow::recalculate_scrollbars));
        Glib::signal_idle().connect_once(sigc::mem_fun(this, &MainWindow::load_tile_locations));

        //Force a rebuild of the layer list
        level_layers_changed_cb();
    }

    void save_button_clicked_callback() {
        Gtk::FileChooserDialog fd("Please choose a file", Gtk::FILE_CHOOSER_ACTION_SAVE);
        fd.set_transient_for(*this);
        fd.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
        fd.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);

        int result = fd.run();

        if(result == Gtk::RESPONSE_OK) {
            fd.hide();

            PFNSerializer serializer(*level_);
            serializer.save_to(fd.get_filename());
        }
    }


private:
    const Glib::RefPtr<Gtk::Builder>& builder_;
    Canvas* canvas_;    

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
