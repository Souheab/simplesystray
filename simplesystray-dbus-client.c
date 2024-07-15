#include "dbus-status-notifier-watcher.h"
#include "simplesystray-dbus-client.h"
#include <gtk/gtk.h>

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

GDBusProxy *get_sn_item_proxy(gchar *const service) {
  GError *error = NULL;

  // TODO: check if org.freedesktop.StatusNotifierItem exists
  // by the way, even if interface doesn't exist a proxy is still returned
  // so use a robust way to check if the interface exists
  GDBusProxy *item_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                    G_DBUS_PROXY_FLAGS_NONE,
                                    NULL,
                                    service,
                                    "/StatusNotifierItem",
                                    "org.kde.StatusNotifierItem",
                                    NULL,
                                    &error
                                    );

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

SysTrayItem *new_sys_tray_item(gchar *const service, GVariant *properties) {
  GVariantIter *iter;
  gchar *key;
  GVariant *value;
  SysTrayItem *item = g_new(SysTrayItem, 1);
  item->id = NULL;
  item->service = g_strdup(service);
  item->title = NULL;
  item->icon_name = NULL;
  item->item_is_menu = FALSE;
  item->icon_pixmap = NULL;
  item->menu_object_path = NULL;

  g_variant_get(properties, "(a{sv})", &iter);
  g_print("Iterating over properties\n");
  while (g_variant_iter_loop(iter, "{sv}", &key, &value)) {
    /* GVariant  */

    g_print("Processing property\n");
    g_print("Key: %s\n", key);
    g_print("Value type: %s\n", g_variant_get_type_string(value));

    if (g_strcmp0(key, "Id") == 0) {
      item->id = g_variant_dup_string(value, NULL);
    } else if (g_strcmp0(key, "Title") == 0) {
      item->title = g_variant_dup_string(value, NULL);
    } else if (g_strcmp0(key, "IconName") == 0) {
      item->icon_name = g_variant_dup_string(value, NULL);
    } else if (g_strcmp0(key, "IconPixmap") == 0) {
      g_print("IconPixmap found\n");
      GVariant *pixmap;
      pixmap = g_variant_get_child_value(value, 0);
      g_print("pixmap type: %s\n", g_variant_get_type_string(pixmap));
      gint width, height;
      GVariant *temp;
      temp = g_variant_get_child_value(pixmap, 0) ;
      g_variant_get(temp, "i", &width);
      temp = g_variant_get_child_value(pixmap, 1) ;
      g_variant_get(temp, "i", &height);
      temp = g_variant_get_child_value(pixmap, 2);

      const guint8 *data;
      gsize length;
      data = g_variant_get_fixed_array(temp, &length, sizeof(guint8));
      g_print("length: %lu\n", length);
      item->icon_pixmap = g_new(IconPixmap, 1);
      item->icon_pixmap->width = width;
      item->icon_pixmap->height = height;
      item->icon_pixmap->data = g_memdup2(data, length);

      g_print("IconPixmap: width: %d, height: %d, data_lenght: %lu\n", width, height, length);
    } else if (g_strcmp0(key, "ItemIsMenu") == 0) {
      item->item_is_menu = g_variant_get_boolean(value);
    } else if (g_strcmp0(key, "Menu") == 0) {
      gchar* menu_object_path = g_variant_dup_string(value, NULL);
      printf("Menu object path: %s\n", menu_object_path);
    }
    /* if (g_variant_is_of_type(value, G_VARIANT_TYPE_STRING)) { */
    /*   g_print("Value: %s\n", g_variant_get_string(value, NULL)); */
    /* } else if (g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN)) { */
    /*   g_print("Value: %s\n", g_variant_get_boolean(value) ? "true" : "false"); */
    /* } else if (g_variant_is_of_type(value, G_VARIANT_TYPE_INT32)) { */
    /*   g_print("Value: %d\n", g_variant_get_int32(value)); */
    /* } else { */
    /*   gchar *value_str = g_variant_print(value, TRUE); */
    /*   /\* g_print("Value: %s\n", value_str); *\/ */
    /*   g_free(value_str); */
    /* } */
  }
  g_variant_iter_free(iter);
  g_variant_unref(properties);
  g_print("Done processing\n");
  return item;
}

static void add_item(gchar *const service) {
  g_print("adding item\n");
  GtkWidget *tray_icon = gtk_button_new();
  SysTrayItem *item;

  GDBusProxy *item_proxy = get_sn_item_proxy(service);
  if (item_proxy == NULL) {
    g_print("Failed to get proxy for service %s\n", service);
    return;
  }

  GVariant *properties = get_all_properties(item_proxy, "org.kde.StatusNotifierItem");
  g_object_unref(item_proxy);

  if (properties != NULL) {
    g_print("Processing properties for service %s:\n", service);
    item = new_sys_tray_item(service, properties);
    if (item != NULL) {
      items_list = g_list_append(items_list, item);
      g_print("Item of service %s added\n", service);
      g_signal_emit(signal_emitter, sys_tray_item_added_signal_id, 0, item);
    } else {
      g_print("Failed to create item for service %s\n", service);
    }
  } else {
    g_print("Failed to get properties for service %s\n", service);
  }
}

static void remove_item(gchar *const service) {
  GList *item = g_list_find_custom(items_list, service, compare_item);
  if (item != NULL) {
    g_free(((SysTrayItem *)item->data)->service);
    g_free(item->data);
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
