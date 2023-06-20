#include "dashboard_gui.h"

GObject* dashboard_gui_init(struct Module *m)
{

    GtkBuilder *builder = gtk_builder_new_from_resource(DASHBOARD_UI_PATH);
    GObject *w_vbox = gtk_builder_get_object(builder, "dashboard_main_box");
    //GObject *w_vbox = G_OBJECT(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    struct Module *tmp = m->head;

    //GtkWidget *l = gtk_label_new("beverboi");

    //gtk_box_append(GTK_BOX(w_vbox), GTK_WIDGET(l));

    while (tmp != NULL) {
        if (tmp != m) {
            printf("append: %s, %p -> %p\n", tmp->name, tmp->widget, w_vbox);
            gtk_box_append(GTK_BOX(w_vbox), GTK_WIDGET(tmp->widget));
        }
        tmp = tmp->next;
    }

    return w_vbox;
}
