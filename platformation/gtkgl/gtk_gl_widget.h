#ifndef GTK_GL_WIDGET_H
#define GTK_GL_WIDGET_H

#include <gtkmm.h>
#include <tr1/memory>

#include <GL/glx.h>
#include <gdk/gdkx.h>

class GtkGLWidget : public Gtk::DrawingArea {
public:
    typedef std::tr1::shared_ptr<GtkGLWidget> ptr;
    
    GtkGLWidget(BaseObjectType* cobject);
    virtual ~GtkGLWidget() {
        if(idle_connection_.connected()) {
            idle_connection_.disconnect();
        }
    }
        
    void on_area_realize();
    bool on_area_draw(const Cairo::RefPtr<Cairo::Context>& cr);
    
    //bool on_area_expose(GdkEvent *event);
    bool on_area_configure(GdkEventConfigure* event);
    bool on_area_idle();
    
    sigc::signal<void>& signal_init() { return signal_init_; }
protected:
    bool make_current();
    
    virtual void do_render() {}
    virtual void do_init() {}
    virtual void do_resize(int width, int height) {}    
    virtual void swap_gl_buffers();

    sigc::signal<void> signal_init_;
private:
    GLXContext context_;
    sigc::connection idle_connection_;


    
    
};

#endif
