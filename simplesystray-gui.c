#include <gtk/gtk.h>
#include "simplesystray-dbus-client.h"

static void on_systray_item_added(GObject *emitter, SysTrayItem *item, gpointer user_data) {
  g_print("Item added: %s\n", item->service);
  g_print("receivedsignal\n");
}

static void activate(GtkApplication *app, gpointer user_data) {
  GtkWidget *window;
  GtkWidget *box;

  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Simple Tray");
  gtk_window_set_default_size(GTK_WINDOW(window), 200, 50);
  gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DIALOG);
  box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add(GTK_CONTAINER(window), box);
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
