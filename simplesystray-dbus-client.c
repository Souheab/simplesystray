#include "gio/gio.h"
#include "glib.h"
#include "interfaces/dbus-status-notifier-watcher.h"
#include "interfaces/dbus-status-notifier-item.h"
#include "simplesystray-dbus-client.h"
#include <gtk/gtk.h>
#include <time.h>

SnWatcher *watcher_proxy = NULL;
GList *items_list = NULL;

static guint sys_tray_item_added_signal_id = 0;
GObject *signal_emitter = NULL;

GVariant *get_all_properties(GDBusProxy *proxy, gchar *interface) {
  GError *error = NULL;
  GVariant *result = g_dbus_proxy_call_sync(proxy,
                                            "org.freedesktop.DBus.Properties.GetAll",
                                            g_variant_new("(s)", interface),
                                            G_DBUS_CALL_FLAGS_NONE,
                                            -1,
                                            NULL,
                                            &error
                                            );
  if (error != NULL) {
    g_printerr("Error getting all properties: %s\n", error->message);
    g_error_free(error);
    return NULL;
  }
  return result;
}

SnItem *get_sn_item_proxy(gchar *const service) {
  GError *error = NULL;

  SnItem *item_proxy = sn_item_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, service, "/StatusNotifierItem", NULL, &error);

  if (error != NULL) {
    g_printerr("Error getting proxy for service %s: %s\n", service, error->message);
    g_error_free(error);
    return NULL;
  }
  g_print("Proxy for service %s set up\n", service);
  return item_proxy;
}

gint compare_item(gconstpointer a, gconstpointer b) {
  return g_strcmp0(((SysTrayItem *)a)->service, ((SysTrayItem *)b)->service);
}

static SysTrayItem *new_systray_item_from_proxy(SnItem *item_proxy, gchar *const service) {
  const gchar *id = sn_item_get_id(item_proxy);
  const gchar *title = sn_item_get_title(item_proxy);
  const gchar *icon_name = sn_item_get_icon_name(item_proxy);
  gboolean item_is_menu = sn_item_get_item_is_menu(item_proxy);
  const gchar *menu_object_path = sn_item_get_menu(item_proxy);
  GVariant *pixmap = sn_item_get_icon_pixmap(item_proxy);
  pixmap = g_variant_get_child_value(pixmap, 0);
  gint width, height;
  GVariant *temp;
  temp = g_variant_get_child_value(pixmap, 0) ;
  g_variant_get(temp, "i", &width);
  temp = g_variant_get_child_value(pixmap, 1) ;
  g_variant_get(temp, "i", &height);
  const guint8 *data;
  gsize length;
  temp = g_variant_get_child_value(pixmap, 2);
  data = g_variant_get_fixed_array(temp, &length, sizeof(guint8));

  SysTrayItem *item = g_new(SysTrayItem, 1);
  item->id = id;
  item->service = service;
  item->title = title;
  item->icon_name = icon_name;
  item->item_is_menu = item_is_menu;
  item->menu_object_path = menu_object_path;
  IconPixmap *icon_pixmap = g_new(IconPixmap, 1);
  icon_pixmap->width = width;
  icon_pixmap->height = height;
  icon_pixmap->data = data;
  item->icon_pixmap = icon_pixmap;

  return item;
}

static void add_item(gchar *const service) {
  g_print("adding item\n");
  GtkWidget *tray_icon = gtk_button_new();
  SnItem *item_proxy = get_sn_item_proxy(service);
  if (item_proxy == NULL) {
    g_print("Failed to get proxy for service %s\n", service);
    return;
  }
  SysTrayItem *item = new_systray_item_from_proxy(item_proxy, service);
  items_list = g_list_append(items_list, item);
  g_print("Item of service %s added\n", service);
  g_signal_emit(signal_emitter, sys_tray_item_added_signal_id, 0, item);
}

static void remove_item(gchar *const service) {
  GList *item = g_list_find_custom(items_list, service, compare_item);
  if (item != NULL) {
    items_list = g_list_delete_link(items_list, item);
    g_print("Item of service %s removed\n", service);
  }
}

static void on_item_registered(SnWatcher *watcher, gchar *const service, gpointer data) {
  g_print("Item registered %s", service);
  add_item(service);
}

static void on_item_unregistered(SnWatcher *watcher, gchar *const service, gpointer data) {
  g_print("Item unregistered %s", service);
  remove_item(service);
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

  //TODO: connect to item-unregistered signal
  g_signal_connect(watcher_proxy, "item-registered", G_CALLBACK(on_item_registered), NULL);
  return TRUE;
}

void init_dbus_client() {
  signal_emitter = g_object_new(G_TYPE_OBJECT, NULL);
  sys_tray_item_added_signal_id = g_signal_new("systray-item-added",
                                                 G_TYPE_OBJECT,
                                                 G_SIGNAL_RUN_LAST,
                                                 0,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 G_TYPE_NONE,
                                               1,
                                               G_TYPE_POINTER
                                               );
  g_print("early sig emit %p\n", signal_emitter);
  if (!setup_dbus_proxy()) {
    g_printerr("Failed to set up dbus proxy\n");
    return;
  }
}
