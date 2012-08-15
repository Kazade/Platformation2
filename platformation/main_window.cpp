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

MainWindow::MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder):
    Gtk::Window(cobject),
    builder_(builder) {

    add_events(Gdk::EXPOSURE_MASK);
    add_events(Gdk::KEY_PRESS_MASK);
    builder_->get_widget_derived("canvas", canvas_);

    canvas_->signal_post_init().connect(sigc::mem_fun(this, &MainWindow::post_canvas_init));

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

    canvas_->signal_scroll_event().connect(
        sigc::mem_fun(this, &MainWindow::canvas_scroll_event)
    );

    ui<Gtk::ToolButton>("save_toolbutton")->signal_clicked().connect(
        sigc::mem_fun(this, &MainWindow::save_button_clicked_callback)
    );

    maximize();
}

void MainWindow::_create_layer_list_model() {
    layer_list_model_ = Gtk::TreeStore::create(layer_list_columns_);

    Gtk::TreeView* view = ui<Gtk::TreeView>("layer_list");

    view->set_model(layer_list_model_);
    view->append_column("Name", layer_list_columns_.column_name_);
    view->append_column("Active?", layer_list_columns_.checked_);

    Glib::RefPtr<Gtk::TreeSelection> selection = view->get_selection();
    selection->set_mode (Gtk::SELECTION_SINGLE);
    selection->signal_changed().connect(sigc::mem_fun(this, &MainWindow::layer_selection_changed_cb));
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
    for(std::string location: canvas_->tile_chooser().directories()) {
        node.append_value().set(location);
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
            canvas_->tile_chooser().add_directory(n.get());
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
        canvas_->tile_chooser().previous();
    } else if (key->keyval == GDK_KEY_d) {
        L_DEBUG("Changing to next tile selection");
        canvas_->tile_chooser().next();
    }
    return true;
}

void MainWindow::save_button_clicked_callback() {
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

void MainWindow::post_canvas_init() {
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

void MainWindow::tile_loaded_cb(float percentage_done) {
    ui<Gtk::ProgressBar>("progress_bar")->set_fraction(percentage_done / 100.0);

    //Run a few events to keep things reponsive without slowing down too much (while events_pending() seems to just grind to a halt)
    int counter = 3;
    while(counter--) {
        Gtk::Main::iteration(false);
    }
}

void MainWindow::tile_location_changed_cb() {
    tile_location_list_model_->clear();

    int32_t i = 0;
    for(std::string directory: canvas_->tile_chooser().directories()) {
        Gtk::TreeModel::Row row = *(tile_location_list_model_->append());
        row[tile_location_list_columns_.column_id] = i++;
        row[tile_location_list_columns_.folder] = directory;
    }

    save_tile_locations();
}

void MainWindow::remove_tile_location_button_clicked_cb() {
    Gtk::TreeView* view = ui<Gtk::TreeView>("tile_location_list");
    Gtk::TreeModel::iterator iter = view->get_selection()->get_selected();
    if(iter) {
        std::string path = Glib::ustring((*iter)[tile_location_list_columns_.folder]);
        L_DEBUG("Removing path: " + path);
        canvas_->tile_chooser().remove_directory(path);
    }
}

void MainWindow::add_tile_location_button_clicked_cb() {
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

void MainWindow::remove_layer_button_clicked_cb() {
    Gtk::TreeView* view = ui<Gtk::TreeView>("layer_list");

    Glib::RefPtr<Gtk::TreeSelection> selection = view->get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();

    if(iter) {
        //Only remove the active layer if something is selected
        level_->remove_layer(level_->active_layer());
    }
}

void MainWindow::add_layer_button_clicked_cb() {
    level_->add_layer();
}

void MainWindow::recalculate_scrollbars(kglt::Pass& pass) {
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

bool MainWindow::canvas_scroll_event(GdkEventScroll* scroll_event) {
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

void MainWindow::scrollbar_value_changed() {
    double x_pos = ui<Gtk::Scrollbar>("main_horizontal_scrollbar")->get_value();
    double y_pos = ui<Gtk::Scrollbar>("main_vertical_scrollbar")->get_value();
    canvas_->scene().active_camera().move_to(x_pos, -y_pos, 0.0);
}

}
