#include <gtk/gtk.h>
#include "simplesystray-dbus-client.h"

typedef struct {
  gchar *id;
  GtkWidget *systray_item_widget;
} SysTrayItemView;

GtkWidget *systray_box;

static void new_sys_tray_item_view(SysTrayItem *item) {
  SysTrayItemView *view = g_new(SysTrayItemView, 1);
  view->id = g_strdup(item->id);
  if (item->icon_name != NULL || item->icon_name[0] != '\0') {
    view->systray_item_widget = gtk_button_new_from_icon_name(item->icon_name, GTK_ICON_SIZE_MENU);
  } else {
    GdkPixbuf *pixbuf =
      gdk_pixbuf_new_from_data(
        item->icon_pixmap->data,
        GDK_COLORSPACE_RGB,
        TRUE,
        8,
        item->icon_pixmap->width,
        item->icon_pixmap->height,
        item->icon_pixmap->width * 4,
        NULL,
        NULL
    );
    GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);
    view->systray_item_widget = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(view->systray_item_widget), image);
  }

  gtk_box_pack_start(GTK_BOX(systray_box), view->systray_item_widget, FALSE, FALSE, 0);
  gtk_widget_show_all(systray_box);
}

static void on_systray_item_added(GObject *emitter, SysTrayItem *item, gpointer user_data) {
  g_print("receivedsignal\n");
  g_print("Item added: %s\n", item->service);
  g_print("Item id: %s\n", item->id);
  new_sys_tray_item_view(item);
}

static void activate(GtkApplication *app, gpointer user_data) {
  GtkWidget *window;

  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Simple Tray");
  gtk_window_set_default_size(GTK_WINDOW(window), 200, 50);
  gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DIALOG);
  systray_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add(GTK_CONTAINER(window), systray_box);
  gtk_widget_show_all(window);
  init_dbus_client();
  printf("signal emitter: %p\n", signal_emitter);
  g_signal_connect(signal_emitter, "systray-item-added", G_CALLBACK(on_systray_item_added), NULL);
}


int main(int argc, char *argv[]) {
  GtkApplication *app;
  gint status;

  app = gtk_application_new("com.souheab.simpletray", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return EXIT_SUCCESS;
}
