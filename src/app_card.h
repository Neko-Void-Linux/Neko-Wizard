#ifndef APP_CARD_H
#define APP_CARD_H

#include <gtk/gtk.h>
#include "apps_manager.h"

G_BEGIN_DECLS

GtkWidget *neko_app_card_new(AppInfo *info);
void neko_app_card_set_selected(GtkWidget *card, gboolean selected);
gboolean neko_app_card_get_selected(GtkWidget *card);

G_END_DECLS

#endif
