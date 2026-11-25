#include <gtk/gtk.h>
#include <locale.h>
#include "window.h"

static void
on_activate (GtkApplication *app,
             gpointer        user_data)
{
  (void)user_data;
  neko_store_window_new (app);
}

int
main (int argc, char **argv)
{
  setlocale (LC_ALL, "");
  g_setenv ("GSK_RENDERER", "gl", FALSE);
  // Isolate the UI from the desktop GTK theme so external themes never inject
  // colors, gradients, or check/radio bitmap assets into Neko Wizard.
  g_setenv ("GTK_THEME", "Default:dark", TRUE);
  // Avoid forwarding this launch to an already-running app instance that uses
  // the generic ID from an older Neko Wizard build.
  g_autoptr(GtkApplication) app = gtk_application_new ("com.nekowizard.neko-wizard", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
  return g_application_run (G_APPLICATION (app), argc, argv);
}
