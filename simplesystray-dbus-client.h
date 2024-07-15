#ifndef __SIMPLESYSTRAY_DBUS_CLIENT_H__
#define __SIMPLESYSTRAY_DBUS_CLIENT_H__
#include <gio/gio.h>
typedef struct {
  gint width;
  gint height;
  guint8 *data;
} IconPixmap;

typedef struct {
  gchar *id;
  gchar *service;
  gchar *title;
  gchar *icon_name;
  gboolean item_is_menu;
  gchar *menu_object_path;
  IconPixmap *icon_pixmap;
} SysTrayItem;
void init_dbus_client();
extern GObject *signal_emitter;
#endif
