#include <glibmm/i18n.h>
#include <cassert>
#include "main_window.h"
#include "level.h"
#include "layer.h"
#include "kazbase/fdo/base_directory.h"

namespace pn {

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

MainWindow::MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder):
    Gtk::Window(cobject),
    builder_(builder) {

    add_events(Gdk::EXPOSURE_MASK);
    builder_->get_widget_derived("canvas", canvas_);

    tile_chooser_.reset(new TileChooser(canvas_->scene()));
    tile_chooser_->signal_locations_changed().connect(
        sigc::mem_fun(this, &MainWindow::tile_location_changed_cb)
    );

    //Must happen after the canvas as been created
    level_.reset(new Level(canvas_->scene()));

    //Watch for layer changes on the level
    level_->signal_layers_changed().connect(
        sigc::mem_fun(this, &MainWindow::level_layers_changed_cb)
    );

    _create_layer_list_model();
    _create_tile_location_list_model();

    //Make the level name change when the text entry changes
    ui<Gtk::Entry>("level_name_box")->set_text(level_->name());
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

    maximize();
}

}
