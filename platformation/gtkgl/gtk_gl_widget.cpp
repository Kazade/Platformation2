#include "gtk_gl_widget.h"
#include "kazbase/logging/logging.h"
#include <gdkmm.h>
#include <gdk/gdkx.h>

int attributes[] = {
    GLX_RGBA, 
    GLX_RED_SIZE, 1, 
    GLX_GREEN_SIZE, 1,
    GLX_BLUE_SIZE, 1, 
    GLX_DOUBLEBUFFER, True, 
    GLX_DEPTH_SIZE, 24, 
    None 
};

GtkGLWidget::GtkGLWidget(BaseObjectType* cobject):
    Gtk::DrawingArea(cobject) {
    
    set_double_buffered(false);
    
    Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default();
    Display* xdisplay = GDK_DISPLAY_XDISPLAY(display->gobj());
    int xscreen = DefaultScreen(xdisplay);
    GdkScreen* screen = gdk_screen_get_default();
    XVisualInfo* xvisual = glXChooseVisual(xdisplay, xscreen, attributes);

    GdkVisual* visual = gdk_x11_screen_lookup_visual(screen, xvisual->visualid);

    gtk_widget_set_visual(GTK_WIDGET(gobj()), visual);

    context_ = glXCreateContext (xdisplay, xvisual, NULL, TRUE);
    set_size_request(320, -1);

    add_events(
        Gdk::EXPOSURE_MASK |
        Gdk::BUTTON_PRESS_MASK | 
        Gdk::BUTTON_RELEASE_MASK |
        Gdk::POINTER_MOTION_MASK | 
        Gdk::POINTER_MOTION_HINT_MASK |
        Gdk::KEY_PRESS_MASK | 
        Gdk::KEY_RELEASE_MASK
    );
    
    signal_realize().connect(sigc::mem_fun(this, &GtkGLWidget::on_area_realize));
    signal_draw().connect(sigc::mem_fun(this, &GtkGLWidget::on_area_draw));
    //signal_event().connect(sigc::mem_fun(this, &GtkGLWidget::on_area_expose));
    signal_configure_event().connect(sigc::mem_fun(this, &GtkGLWidget::on_area_configure));
    idle_connection_ = Glib::signal_idle().connect(sigc::mem_fun(this, &GtkGLWidget::on_area_idle));

    queue_draw();
}

bool GtkGLWidget::make_current() {
    Glib::RefPtr<Gdk::Window> window = get_window();
    Display* xdisplay = GDK_WINDOW_XDISPLAY(window->gobj());
    int id = GDK_WINDOW_XID(window->gobj());
    return glXMakeCurrent(xdisplay, id, context_) == TRUE;
}

bool GtkGLWidget::on_area_idle() {
    //queue_draw();
    if(make_current()) {
        do_render();
    }
    return true;
}

void GtkGLWidget::on_area_realize() {
    if(make_current()) {
        L_DEBUG("Initializing a GL widget");
        signal_init_();
        do_init();
    }
}


bool GtkGLWidget::on_area_draw(const ::Cairo::RefPtr< ::Cairo::Context>& cr) {
    //if(event->count > 0) return true;

    if(make_current()) {
        do_render();
    }

    return true;
}

/*
bool GtkGLWidget::on_area_expose(GdkEvent *event) {
    std::cout << event->type << std::endl;

    if(event->type != GDK_EXPOSE) return false;
//    if(event->count > 0) return true;
    
    if(make_current()) {
        do_render();
    }
    
    return true;
}*/

bool GtkGLWidget::on_area_configure(GdkEventConfigure* event) {
    Gtk::Allocation allocation = get_allocation();
    if(make_current()) {
        //glViewport (0, 0, allocation.get_width(), allocation.get_height());
        do_resize(allocation.get_width(), allocation.get_height());
    }
    return true;
}

void GtkGLWidget::swap_gl_buffers() {
    if(!make_current()) return;
    
    Glib::RefPtr<Gdk::Window> window = get_window();
    Display* xdisplay = GDK_WINDOW_XDISPLAY(window->gobj());
    int id = GDK_WINDOW_XID(window->gobj());
    glXSwapBuffers(xdisplay, id);
}
