#include <glibmm/i18n.h>
#include <cassert>
#include <fstream>

#include "main_window.h"
#include "level.h"
#include "layer.h"
#include "kazbase/fdo/base_directory.h"
#include "kazbase/json/json.h"
#include "kazbase/os/core.h"
#include "kazbase/os/path.h"
#include "kazbase/file_utils.h"
#include "kazbase/string.h"

namespace pn {

static std::string CONFIG_DIR = os::path::join(fdo::xdg::get_config_home(), "platformation");
static std::string CONFIG_PATH = os::path::join(CONFIG_DIR, "platformation.json");

void MainWindow::_create_layer_list_model() {
    layer_list_model_ = Gtk::TreeStore::create(layer_list_columns_);

    Gtk::TreeView* view = ui<Gtk::TreeView>("layer_list");

    view->set_model(layer_list_model_);
    view->append_column("Name", layer_list_columns_.column_name_);
    view->append_column("Active?", layer_list_columns_.checked_);

    Glib::RefPtr<Gtk::TreeSelection> selection = view->get_selection();
    selection->set_mode (Gtk::SELECTION_SINGLE);
    selection->signal_changed().connect(sigc::mem_fun(this, &MainWindow::layer_selection_changed_cb));

    //Force a rebuild of the list
    level_layers_changed_cb();
}

void MainWindow::_create_tile_location_list_model() {
    tile_location_list_model_ = Gtk::TreeStore::create(tile_location_list_columns_);
    Gtk::TreeView* view = ui<Gtk::TreeView>("tile_location_list");
    view->set_model(tile_location_list_model_);
    view->append_column(_("Directory"), tile_location_list_columns_.folder);

    Glib::RefPtr<Gtk::TreeSelection> selection = view->get_selection();
    selection->set_mode(Gtk::SELECTION_SINGLE);
}

void MainWindow::layer_selection_changed_cb() {
    Gtk::TreeView* view = ui<Gtk::TreeView>("layer_list");

    Glib::RefPtr<Gtk::TreeSelection> selection = view->get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();

    if(iter) {
        Gtk::TreeModel::Row row = *iter;
        uint32_t active_layer = row[layer_list_columns_.column_id_];
        level_->set_active_layer(active_layer);
    }

    /*
        Go through the rows of the tree view, set the active one to have a checkmark, and the inactive
        ones to have no image
    */
    for(Gtk::TreeModel::iterator it = view->get_model()->children().begin(); it != view->get_model()->children().end(); ++it) {
        Gtk::TreeModel::Row row = *it;
        if(row[layer_list_columns_.column_id_] != (*iter)[layer_list_columns_.column_id_]) {
            row[layer_list_columns_.checked_] = Glib::RefPtr<Gdk::Pixbuf>();
        } else {
            row[layer_list_columns_.checked_] = view->render_icon_pixbuf(Gtk::Stock::YES, Gtk::ICON_SIZE_MENU);
        }
    }
}

void MainWindow::level_layers_changed_cb() {
    layer_list_model_->clear();

    if(!level_) {
        return;
    }

    Gtk::TreeView* view = ui<Gtk::TreeView>("layer_list");

    for(uint32_t i = 0; i < level_->layer_count(); ++i) {
        Layer& layer = level_->layer_at(i);

        Gtk::TreeModel::Row row = *(layer_list_model_->append());
        row[layer_list_columns_.column_id_] = i;
        row[layer_list_columns_.column_name_] = layer.name();

        //If this is the active layer, display a tick next to it
        if(i == level_->active_layer()) {
            row[layer_list_columns_.checked_] = view->render_icon_pixbuf(Gtk::Stock::YES, Gtk::ICON_SIZE_MENU);
        } else {
            row[layer_list_columns_.checked_] = Glib::RefPtr<Gdk::Pixbuf>();
        }
    }    
}

void MainWindow::save_tile_locations() {
    json::JSON j;
    json::Node& node = j.insert_array("locations");
    for(std::string location: tile_chooser_->directories()) {
        node.append_value(location);
    }

    std::ofstream fileout(CONFIG_PATH);
    fileout << json::dumps(j);
}

void MainWindow::load_tile_locations() {
    std::string contents = file_utils::read_contents(CONFIG_PATH);

    if(str::strip(contents).empty()) {
        L_INFO("Not loading locations from config as the config is empty");
        return;
    }

    json::JSON j = json::loads(contents);

    ui<Gtk::ProgressBar>("progress_bar")->show();
    if(j.has_key("locations")) {
        for(uint32_t i = 0; i < j["locations"].length(); ++i) {
            json::Node& n = j["locations"][i];
            tile_chooser_->add_directory(n.get());
        }
    }
    ui<Gtk::ProgressBar>("progress_bar")->hide();
}

void MainWindow::_generate_blank_config() {
    if(!os::path::exists(CONFIG_DIR)) {
        os::make_dirs(CONFIG_DIR);
    }

    if(!os::path::exists(CONFIG_PATH)) {
        os::touch(CONFIG_PATH);
    }
}

bool MainWindow::key_press_event_cb(GdkEventKey* key) {
    L_DEBUG("Key press event received");
    if(key->keyval == GDK_KEY_a) {
        L_DEBUG("Changing to previous tile selection");
        tile_chooser_->previous();
    } else if (key->keyval == GDK_KEY_d) {
        L_DEBUG("Changing to next tile selection");
        tile_chooser_->next();
    }
    return true;
}

MainWindow::MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder):
    Gtk::Window(cobject),
    builder_(builder),
    active_instance_(nullptr) {

    add_events(Gdk::EXPOSURE_MASK);
    add_events(Gdk::KEY_PRESS_MASK);
    builder_->get_widget_derived("canvas", canvas_);

    canvas_->signal_init().connect(sigc::mem_fun(this, &MainWindow::post_canvas_realize));

    _create_layer_list_model();
    _create_tile_location_list_model();
    _generate_blank_config();


    //Make the level name change when the text entry changes    
    ui<Gtk::Entry>("level_name_box")->signal_changed().connect(
        sigc::mem_fun(this, &MainWindow::level_name_box_changed_cb)
    );

    //Set up the signals for adding and removing layers
    ui<Gtk::Button>("add_layer_button")->signal_clicked().connect(
        sigc::mem_fun(this, &MainWindow::add_layer_button_clicked_cb)
    );
    ui<Gtk::Button>("remove_layer_button")->signal_clicked().connect(
        sigc::mem_fun(this, &MainWindow::remove_layer_button_clicked_cb)
    );

    //Set up the signals for adding and removing tile folders
    ui<Gtk::Button>("add_tile_location_button")->signal_clicked().connect(
        sigc::mem_fun(this, &MainWindow::add_tile_location_button_clicked_cb)
    );

    ui<Gtk::Button>("remove_tile_location_button")->signal_clicked().connect(
        sigc::mem_fun(this, &MainWindow::remove_tile_location_button_clicked_cb)
    );

    ui<Gtk::ProgressBar>("progress_bar")->hide();
    ui<Gtk::Scrollbar>("main_vertical_scrollbar")->signal_value_changed().connect(
        sigc::mem_fun(this, &MainWindow::scrollbar_value_changed)
    );
    ui<Gtk::Scrollbar>("main_horizontal_scrollbar")->signal_value_changed().connect(
        sigc::mem_fun(this, &MainWindow::scrollbar_value_changed)
    );

    signal_key_press_event().connect(
        sigc::mem_fun(this, &MainWindow::key_press_event_cb)
    );

    canvas_->signal_key_press_event().connect(
        sigc::mem_fun(this, &MainWindow::key_press_event_cb)
    );

    canvas_->signal_mesh_selected().connect(
        sigc::mem_fun(this, &MainWindow::mesh_selected_callback)
    );

    canvas_->signal_scroll_event().connect(
        sigc::mem_fun(this, &MainWindow::canvas_scroll_event)
    );

    maximize();    
}

}
