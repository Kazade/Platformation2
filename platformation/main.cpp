#include <gtkmm.h>

#include "kazbase/logging/logging.h"
#include "kazbase/fdo/base_directory.h"
#include "main_window.h"

int main(int argc, char* argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler()));
    logging::get_logger("/")->set_level(logging::LOG_LEVEL_DEBUG);

    Glib::RefPtr<Gtk::Application> app =
            Gtk::Application::create(argc, argv, "uk.co.kazade.platformation");


    std::string ui_file = fdo::xdg::find_data_file("data/ui/main_window.ui");
    Glib::RefPtr<Gtk::Builder> ui = Gtk::Builder::create();
    ui->add_from_file(ui_file);

    pn::MainWindow* window = nullptr;
    ui->get_widget_derived("main_window", window);
    return app->run(*window);
}
