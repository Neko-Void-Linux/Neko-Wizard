#include <gtk/gtk.h>
#include "window.h"

static void
on_activate (GtkApplication *app,
             gpointer        user_data)
{
  (void)user_data; // Required by the GtkApplication activate signal.
  neko_store_window_new (app);
}

int
main (int argc, char **argv)
{
  g_setenv ("GSK_RENDERER", "gl", FALSE);
  // Avoid forwarding this launch to an already-running app instance that uses
  // the generic ID from an older Neko Wizard build.
  g_autoptr(GtkApplication) app = gtk_application_new ("com.nekowizard.neko-wizard", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
  return g_application_run (G_APPLICATION (app), argc, argv);
}
