#ifndef __SIMPLESYSTRAY_DBUS_CLIENT_H__
#define __SIMPLESYSTRAY_DBUS_CLIENT_H__
#include <gio/gio.h>
typedef struct {
  gint width;
  gint height;
  const guint8 *data;
} IconPixmap;

typedef struct {
  const gchar *id;
  const gchar *service;
  const gchar *title;
  const gchar *icon_name;
  gboolean item_is_menu;
  const gchar *menu_object_path;
  const IconPixmap *icon_pixmap;
} SysTrayItem;
void init_dbus_client();
extern GObject *signal_emitter;
#endif
