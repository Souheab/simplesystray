#include "dbus-status-notifier-watcher.h"
#include <gtk/gtk.h>

SnWatcher *watcher_proxy = NULL;
GList *items_list = NULL;

typedef struct {
  GtkWidget *tray_icon;
  gchar *const service;
} SysTrayItem;

static void on_item_registered(SnWatcher *watcher, char *service, gpointer data) {
  g_print("Item registered %s", service);
}

static void on_item_unregistered(SnWatcher *watcher, char *service, gpointer data) {
  g_print("Item unregistered %s", service);
}

static gboolean setup_dbus_proxy() {
  GError *error = NULL;
  printf("Setting up dbus to org.kde.StatusNotifierWatcher\n");
  watcher_proxy = sn_watcher_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                    G_DBUS_PROXY_FLAGS_NONE,
                                    "org.kde.StatusNotifierWatcher",
                                    "/StatusNotifierWatcher",
                                    NULL,
                                    &error
                                    );
  printf("Proxy set up\n");
  if (error != NULL) {
    g_printerr("Error setting up proxy: %s\n", error->message);
    g_error_free(error);
  }

  printf("Registering host\n");
  error = NULL;
  char name[100];
  g_snprintf(name, 100, "org.freedesktop.StatusNotifierHost-%d", getpid());
  sn_watcher_call_register_host_sync(watcher_proxy, name, NULL, &error);
  if (error != NULL) {
    g_printerr("Error registering host: %s\n", error->message);
    g_print("Perhaps the StatusNotifierWatcher service is not running\nExiting Program\n");
    g_error_free(error);
    return FALSE;
  }

  g_print("Host registered\n");

  g_print("Connecting to item-registered signal\n");
  // TODO: use item-registered and item-unregistered instead as they pass service name as argument
  g_signal_connect(watcher_proxy, "item-registered", G_CALLBACK(on_item_registered), NULL);
  return TRUE;
}

static void activate(GtkApplication *app, gpointer user_data) {
  GtkWidget *window;

  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Simple Tray");
  gtk_window_set_default_size(GTK_WINDOW(window), 200, 50);
  gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DIALOG);
  gtk_widget_show_all(window);
  gboolean result = setup_dbus_proxy();
  if (!result) {
    g_application_quit(G_APPLICATION(app));
  }
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
