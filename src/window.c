#include "window.h"
#include "apps_manager.h"
#include "app_card.h"
#include "installer.h"
#include <gtk/gtk.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#include <X11/Xlib.h>
#endif

struct _NekoStoreWindow {
    GtkApplicationWindow parent_instance;
    GtkWidget           *stack;

    GtkWidget *welcome_page;
    GtkWidget *step_pages[6];
    GtkWidget *finished_page;
    GtkWidget *finished_label;
    GtkWidget *progress_bar;
    GtkWidget *status_label;

    GList       *apps_to_install;
    GList       *current_installing;
    guint        pulse_id;
    const char  *priv_prefix;  /* "", "sudo ", or "pkexec " */
};

G_DEFINE_TYPE (NekoStoreWindow, neko_store_window, GTK_TYPE_APPLICATION_WINDOW)

/* ── Helpers ─────────────────────────────────────────── */

static void
go_to_page (GtkButton *btn, gpointer user_data)
{
    NekoStoreWindow *self      = NEKO_STORE_WINDOW (user_data);
    const char      *target_id = g_object_get_data (G_OBJECT (btn), "target_page");
    if (target_id)
        gtk_stack_set_visible_child_name (GTK_STACK (self->stack), target_id);
}

/* ── Group toggle (Select All checkbox) ──────────────── */

static void
on_group_toggle_toggled (GtkCheckButton *btn, gpointer user_data)
{
    GtkWidget *flowbox = GTK_WIDGET (user_data);
    gboolean   active  = gtk_check_button_get_active (btn);

    for (GtkWidget *child = gtk_widget_get_first_child (flowbox);
         child != NULL;
         child = gtk_widget_get_next_sibling (child))
    {
        GtkWidget *card = gtk_flow_box_child_get_child (GTK_FLOW_BOX_CHILD (child));
        if (NEKO_IS_APP_CARD (card))
            neko_app_card_set_selected (NEKO_APP_CARD (card), active);
    }
}

/* ── App-group page factory ──────────────────────────── */

static GtkWidget *
create_app_group_page (NekoStoreWindow *self,
                       const char      *title,
                       AppGroup         group_filter,
                       const char      *back_id,
                       const char      *next_id,
                       const char      *next_btn_label,
                       GCallback        next_cb)
{
    GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 16);

    /* Header */
    GtkWidget *header = gtk_label_new (title);
    gtk_widget_add_css_class (header, "page-header");
    gtk_widget_set_halign (header, GTK_ALIGN_START);
    gtk_widget_set_margin_top (header, 16);
    gtk_widget_set_margin_start (header, 16);
    gtk_box_append (GTK_BOX (vbox), header);

    /* Scrollable content */
    GtkWidget *scrolled = gtk_scrolled_window_new ();
    gtk_widget_set_vexpand (scrolled, TRUE);
    gtk_widget_set_margin_start (scrolled, 16);
    gtk_widget_set_margin_end (scrolled, 16);

    GtkWidget *content_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_bottom (content_box, 24);

    /* "Select All" row */
    GtkWidget *hbox       = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *toggle_all = gtk_check_button_new_with_label ("Select All");
    gtk_widget_set_halign (toggle_all, GTK_ALIGN_END);
    gtk_widget_set_hexpand (toggle_all, TRUE);
    gtk_box_append (GTK_BOX (hbox), toggle_all);
    gtk_box_append (GTK_BOX (content_box), hbox);

    /* App grid */
    GtkWidget *flowbox = gtk_flow_box_new ();
    gtk_widget_set_valign (flowbox, GTK_ALIGN_START);
    gtk_widget_set_halign (flowbox, GTK_ALIGN_FILL);
    gtk_flow_box_set_max_children_per_line (GTK_FLOW_BOX (flowbox), 6);
    gtk_flow_box_set_selection_mode (GTK_FLOW_BOX (flowbox), GTK_SELECTION_NONE);
    gtk_flow_box_set_row_spacing (GTK_FLOW_BOX (flowbox), 16);
    gtk_flow_box_set_column_spacing (GTK_FLOW_BOX (flowbox), 16);

    g_signal_connect (toggle_all, "toggled",
                      G_CALLBACK (on_group_toggle_toggled), flowbox);

    gtk_box_append (GTK_BOX (content_box), flowbox);

    GList *apps = get_all_apps ();
    for (GList *l = apps; l != NULL; l = l->next) {
        AppInfo *info = (AppInfo *) l->data;
        if (info->group == group_filter)
            gtk_flow_box_insert (GTK_FLOW_BOX (flowbox),
                                 neko_app_card_new (info), -1);
    }
    g_list_free (apps);

    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled), content_box);
    gtk_box_append (GTK_BOX (vbox), scrolled);

    /* Footer with Back / Next buttons */
    GtkWidget *footer = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_widget_set_margin_bottom (footer, 16);
    gtk_widget_set_margin_end (footer, 16);
    gtk_widget_set_margin_start (footer, 16);

    GtkWidget *spacer = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand (spacer, TRUE);

    if (back_id != NULL) {
        GtkWidget *back_btn = gtk_button_new_with_label ("Back");
        gtk_widget_add_css_class (back_btn, "suggested-action");
        gtk_widget_set_halign (back_btn, GTK_ALIGN_START);
        g_object_set_data (G_OBJECT (back_btn), "target_page", (gpointer) back_id);
        g_signal_connect (back_btn, "clicked", G_CALLBACK (go_to_page), self);
        gtk_box_append (GTK_BOX (footer), back_btn);
    }

    gtk_box_append (GTK_BOX (footer), spacer);

    GtkWidget *next_btn = gtk_button_new_with_label (next_btn_label);
    gtk_widget_add_css_class (next_btn, "suggested-action");
    gtk_widget_add_css_class (next_btn, "install-selected-btn");
    gtk_widget_set_halign (next_btn, GTK_ALIGN_END);
    if (next_id)
        g_object_set_data (G_OBJECT (next_btn), "target_page", (gpointer) next_id);
    g_signal_connect (next_btn, "clicked", next_cb, self);
    gtk_box_append (GTK_BOX (footer), next_btn);

    gtk_box_append (GTK_BOX (vbox), footer);

    return vbox;
}

/* ── Privilege escalation ────────────────────────────── */

static const char *
detect_priv_escalator (void)
{
    if (getuid () == 0)
        return "";
    if (access ("/usr/bin/sudo", X_OK) == 0 ||
        access ("/bin/sudo",     X_OK) == 0)
        return "sudo ";
    if (access ("/usr/bin/pkexec", X_OK) == 0 ||
        access ("/bin/pkexec",     X_OK) == 0)
        return "pkexec ";
    return "sudo ";  /* last resort — will fail loudly if absent */
}

/*
 * Replace every occurrence of "pkexec " in a command template
 * with the detected privilege prefix.
 */
static char *
apply_priv (const char *command, const char *priv)
{
    if (g_strcmp0 (priv, "pkexec ") == 0)
        return g_strdup (command);   /* already correct, no-op */

    gchar **parts  = g_strsplit (command, "pkexec ", -1);
    gchar  *result = g_strjoinv (priv, parts);
    g_strfreev (parts);
    return result;
}

/* ── Installation engine ─────────────────────────────── */

static void install_next_app (NekoStoreWindow *self);

static gboolean
pulse_progress_bar (gpointer user_data)
{
    NekoStoreWindow *self = NEKO_STORE_WINDOW (user_data);
    gtk_progress_bar_pulse (GTK_PROGRESS_BAR (self->progress_bar));
    return G_SOURCE_CONTINUE;
}

static void
stop_pulse (NekoStoreWindow *self)
{
    if (self->pulse_id > 0) {
        g_source_remove (self->pulse_id);
        self->pulse_id = 0;
    }
}

static void
install_progress_cb (const char *message, gpointer user_data)
{
    NekoStoreWindow *self = NEKO_STORE_WINDOW (user_data);
    if (message && g_utf8_validate (message, -1, NULL)) {
        char *trunc = g_strndup (message, 60);
        gtk_label_set_text (GTK_LABEL (self->status_label), trunc);
        g_free (trunc);
    }
}

static void
install_finished_cb (gboolean success, gpointer user_data)
{
    NekoStoreWindow *self = NEKO_STORE_WINDOW (user_data);

    if (self->current_installing == NULL)
        return;

    AppInfo *info        = (AppInfo *) self->current_installing->data;
    info->install_success = success;

    self->current_installing = self->current_installing->next;
    install_next_app (self);
}

static void
install_next_app (NekoStoreWindow *self)
{
    if (self->current_installing != NULL) {
        AppInfo *info    = (AppInfo *) self->current_installing->data;
        char    *status  = g_strdup_printf ("Installing %s...", info->name);
        gtk_label_set_text (GTK_LABEL (self->status_label), status);
        g_free (status);

        char *cmd = apply_priv (info->install_command, self->priv_prefix);
        install_app_async (cmd, install_progress_cb, install_finished_cb, self);
        g_free (cmd);
    } else {
        stop_pulse (self);
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (self->progress_bar), 1.0);
        gtk_label_set_text (GTK_LABEL (self->finished_label),
                            "Neko Void is ready! You can close this window.");
        gtk_label_set_text (GTK_LABEL (self->status_label),
                            "All installations finished.");
    }
}

static void
system_update_finished_cb (gboolean success, gpointer user_data)
{
    (void) success;
    NekoStoreWindow *self = NEKO_STORE_WINDOW (user_data);
    gtk_label_set_text (GTK_LABEL (self->finished_label), "Installing Apps...");
    install_next_app (self);
}

static void
bootstrap_finished_cb (gboolean success, gpointer user_data)
{
    (void) success;
    NekoStoreWindow *self = NEKO_STORE_WINDOW (user_data);

    /*
     * polkit and flatpak are now installed (or were already present).
     * Re-detect the privilege escalator now that pkexec may be available.
     */
    self->priv_prefix = detect_priv_escalator ();

    char *syu_cmd = g_strdup_printf ("%sxbps-install -y -Syu", self->priv_prefix);
    gtk_label_set_text (GTK_LABEL (self->finished_label), "Updating System...");
    gtk_label_set_text (GTK_LABEL (self->status_label), syu_cmd);
    install_app_async (syu_cmd, install_progress_cb, system_update_finished_cb, self);
    g_free (syu_cmd);
}

static void on_install_selected_clicked(GtkButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);

    // Gather ALL selected apps from all categories
    g_list_free(self->apps_to_install);
    self->apps_to_install = NULL;

    GList *apps = get_all_apps();
    for (GList *l = apps; l != NULL; l = l->next) {
        AppInfo *info = (AppInfo *)l->data;
        if (info->selected) {
            self->apps_to_install = g_list_append(self->apps_to_install, info);
        }
    }
    g_list_free(apps);

    gtk_stack_set_visible_child(GTK_STACK(self->stack), self->finished_page);

    gtk_label_set_text(GTK_LABEL(self->finished_label), "Updating System...");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(self->progress_bar), 0.0);
    self->pulse_id = g_timeout_add(100, pulse_progress_bar, self);

    self->current_installing = self->apps_to_install;
    char *status = g_strdup("Running pkexec xbps-install -Syu...");
    gtk_label_set_text(GTK_LABEL(self->status_label), status);
    g_free(status);
    install_app_async("pkexec xbps-install -y -Syu", install_progress_cb, system_update_finished_cb, self);
}
/* ── Theme / language switches ───────────────────────── */

static gboolean
on_theme_switch_state_set (GtkSwitch *widget, gboolean state, gpointer user_data)
{
    GtkSettings     *settings = gtk_settings_get_default ();
    NekoStoreWindow *self     = NEKO_STORE_WINDOW (user_data);

    g_object_set (settings, "gtk-application-prefer-dark-theme", state, NULL);

    if (state)
        gtk_widget_remove_css_class (GTK_WIDGET (self), "light-mode");
    else
        gtk_widget_add_css_class (GTK_WIDGET (self), "light-mode");

    gtk_switch_set_state (widget, state);
    return TRUE;
}

static gboolean
on_language_switch_state_set (GtkSwitch *widget, gboolean state, gpointer user_data)
{
    NekoStoreWindow *self = NEKO_STORE_WINDOW (user_data);

    GtkWidget *box          = self->welcome_page;
    GtkWidget *main_content = gtk_widget_get_last_child (box);

    GtkWidget *icon     = gtk_widget_get_first_child (main_content);
    GtkWidget *title    = gtk_widget_get_next_sibling (icon);
    GtkWidget *subtitle = gtk_widget_get_next_sibling (title);
    GtkWidget *btn_w    = gtk_widget_get_last_child (main_content);

    if (state) {
        gtk_label_set_text (GTK_LABEL (title),    "Bienvenido a Neko Void");
        gtk_label_set_text (GTK_LABEL (subtitle), "Preparemos su sistema con sus aplicaciones favoritas.");
        gtk_button_set_label (GTK_BUTTON (btn_w), "Iniciar Configuración");
    } else {
        gtk_label_set_text (GTK_LABEL (title),    "Welcome to Neko Void");
        gtk_label_set_text (GTK_LABEL (subtitle), "Let's get your system ready with your favorite apps.");
        gtk_button_set_label (GTK_BUTTON (btn_w), "Start Setup");
    }

    gtk_switch_set_state (widget, state);
    return TRUE;
}

/* ── Page builders ───────────────────────────────────── */

static void
build_welcome_page (NekoStoreWindow *self)
{
    /* Top bar: theme switch + language switch */
    GtkWidget *top_bar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_widget_set_valign (top_bar, GTK_ALIGN_START);
    gtk_widget_set_hexpand (top_bar, TRUE);
    gtk_widget_set_margin_top (top_bar, 16);
    gtk_widget_set_margin_start (top_bar, 16);
    gtk_widget_set_margin_end (top_bar, 16);

    GtkWidget *theme_box    = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *theme_icon   = gtk_image_new_from_icon_name ("weather-clear-night-symbolic");
    GtkWidget *theme_switch = gtk_switch_new ();
    gtk_switch_set_active (GTK_SWITCH (theme_switch), TRUE);
    g_signal_connect (theme_switch, "state-set",
                      G_CALLBACK (on_theme_switch_state_set), self);
    gtk_box_append (GTK_BOX (theme_box), theme_icon);
    gtk_box_append (GTK_BOX (theme_box), theme_switch);

    GtkWidget *spacer_top = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand (spacer_top, TRUE);

    GtkWidget *lang_box      = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *lang_label_en = gtk_label_new ("EN");
    GtkWidget *lang_switch   = gtk_switch_new ();
    GtkWidget *lang_label_es = gtk_label_new ("ES");
    g_signal_connect (lang_switch, "state-set",
                      G_CALLBACK (on_language_switch_state_set), self);
    gtk_box_append (GTK_BOX (lang_box), lang_label_en);
    gtk_box_append (GTK_BOX (lang_box), lang_switch);
    gtk_box_append (GTK_BOX (lang_box), lang_label_es);

    gtk_box_append (GTK_BOX (top_bar), theme_box);
    gtk_box_append (GTK_BOX (top_bar), spacer_top);
    gtk_box_append (GTK_BOX (top_bar), lang_box);

    /* Centred main content */
    GtkWidget *main_content = gtk_box_new (GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_halign (main_content, GTK_ALIGN_CENTER);
    gtk_widget_set_valign (main_content, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand (main_content, TRUE);

    char      *logo_path = get_resource_path ("resources/logo.png");
    GtkWidget *icon      = gtk_image_new_from_file (logo_path);
    g_free (logo_path);
    gtk_image_set_pixel_size (GTK_IMAGE (icon), 128);
    gtk_widget_add_css_class (icon, "welcome-icon");

    GtkWidget *title = gtk_label_new ("Welcome to Neko Void");
    gtk_widget_add_css_class (title, "welcome-title");

    GtkWidget *subtitle = gtk_label_new ("Let's get your system ready with your favorite apps.");
    gtk_widget_add_css_class (subtitle, "welcome-subtitle");

    GtkWidget *btn = gtk_button_new_with_label ("Start Setup");
    gtk_widget_add_css_class (btn, "suggested-action");
    g_object_set_data (G_OBJECT (btn), "target_page", "step_0");
    g_signal_connect (btn, "clicked", G_CALLBACK (go_to_page), self);

    GtkWidget *gap = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request (gap, -1, 32);

    gtk_box_append (GTK_BOX (main_content), icon);
    gtk_box_append (GTK_BOX (main_content), title);
    gtk_box_append (GTK_BOX (main_content), subtitle);
    gtk_box_append (GTK_BOX (main_content), gap);
    gtk_box_append (GTK_BOX (main_content), btn);

    /* Assemble welcome page container */
    GtkWidget *welcome_container = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append (GTK_BOX (welcome_container), top_bar);
    gtk_box_append (GTK_BOX (welcome_container), main_content);

    self->welcome_page = welcome_container;
}

static void
build_finished_page (NekoStoreWindow *self)
{
    self->finished_page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_valign (self->finished_page, GTK_ALIGN_CENTER);
    gtk_widget_set_halign (self->finished_page, GTK_ALIGN_CENTER);

    self->finished_label = gtk_label_new ("Installing Apps...");
    gtk_widget_add_css_class (self->finished_label, "welcome-title");

    self->progress_bar = gtk_progress_bar_new ();
    gtk_widget_set_size_request (self->progress_bar, 400, -1);

    self->status_label = gtk_label_new ("Preparing...");
    gtk_widget_add_css_class (self->status_label, "dim-label");

    GtkWidget *gap = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request (gap, -1, 32);

    gtk_box_append (GTK_BOX (self->finished_page), self->finished_label);
    gtk_box_append (GTK_BOX (self->finished_page), gap);
    gtk_box_append (GTK_BOX (self->finished_page), self->progress_bar);
    gtk_box_append (GTK_BOX (self->finished_page), self->status_label);
}

/* ── Window init / class ─────────────────────────────── */

typedef struct {
    const char *id;
    const char *title;
    AppGroup    group;
} PageDef;

static const PageDef defs[] = {
    {"step_0", "Step 1: Gaming Apps",               GROUP_GAMING},
    {"step_1", "Step 2: Drawing and Image Editing",  GROUP_DRAWING_IMAGE},
    {"step_2", "Step 3: Audio & Video Editing",      GROUP_AUDIO_VIDEO},
    {"step_3", "Step 4: Text Editing and Documents", GROUP_TEXT_DOCUMENTS},
    {"step_4", "Step 5: Social Apps and Internet",   GROUP_SOCIAL},
    {"step_5", "Step 6: Drivers",                    GROUP_DRIVERS},
};

static void
neko_store_window_init (NekoStoreWindow *self)
{
    self->stack = gtk_stack_new ();
    gtk_stack_set_transition_type (GTK_STACK (self->stack),
                                   GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);

    build_welcome_page (self);
    build_finished_page (self);

    gtk_stack_add_named (GTK_STACK (self->stack), self->welcome_page, "welcome");

    for (guint i = 0; i < G_N_ELEMENTS (defs); i++) {
        const char *back_id    = (i == 0) ? "welcome" : defs[i - 1].id;
        const char *next_id    = (i < G_N_ELEMENTS (defs) - 1) ? defs[i + 1].id : NULL;
        const char *btn_label  = (next_id != NULL) ? "Next" : "Install";
        GCallback   next_cb    = (next_id != NULL)
                                     ? G_CALLBACK (go_to_page)
                                     : G_CALLBACK (on_install_selected_clicked);

        self->step_pages[i] = create_app_group_page (self,
                                                      defs[i].title,
                                                      defs[i].group,
                                                      back_id, next_id,
                                                      btn_label, next_cb);
        gtk_stack_add_named (GTK_STACK (self->stack),
                             self->step_pages[i], defs[i].id);
    }

    gtk_stack_add_named (GTK_STACK (self->stack), self->finished_page, "finished");
    gtk_stack_set_visible_child_name (GTK_STACK (self->stack), "welcome");
    gtk_window_set_child (GTK_WINDOW (self), self->stack);
}

static void
neko_store_window_dispose (GObject *object)
{
    NekoStoreWindow *self = (NekoStoreWindow *) object;
    stop_pulse (self);
    g_clear_pointer (&self->apps_to_install, g_list_free);
    G_OBJECT_CLASS (neko_store_window_parent_class)->dispose (object);
}

#ifdef GDK_WINDOWING_X11
static void
on_window_map (GtkWidget *widget, gpointer data)
{
    (void) data;
    GdkSurface *surface = gtk_native_get_surface (GTK_NATIVE (widget));
    if (!surface || !GDK_IS_X11_SURFACE (surface))
        return;

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    Display *xdisplay = gdk_x11_display_get_xdisplay (gdk_surface_get_display (surface));
    Window   xid      = gdk_x11_surface_get_xid (surface);
    G_GNUC_END_IGNORE_DEPRECATIONS

    int w = gtk_widget_get_width (widget);
    int h = gtk_widget_get_height (widget);
    if (w <= 0) w = 1000;
    if (h <= 0) h = 700;

    int screen = DefaultScreen (xdisplay);
    int sw     = DisplayWidth (xdisplay, screen);
    int sh     = DisplayHeight (xdisplay, screen);

    if (sw > w && sh > h)
        XMoveWindow (xdisplay, xid, (sw - w) / 2, (sh - h) / 2);
}
#endif

static void
neko_store_window_class_init (NekoStoreWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = neko_store_window_dispose;
}

NekoStoreWindow *
neko_store_window_new (GtkApplication *app)
{
    NekoStoreWindow *window = g_object_new (NEKO_STORE_TYPE_WINDOW,
                                            "application", app, NULL);

    gtk_window_set_title (GTK_WINDOW (window), "Neko Void Setup");
    gtk_window_set_default_size (GTK_WINDOW (window), 1000, 700);

    GtkWidget *header = gtk_header_bar_new ();
    gtk_window_set_titlebar (GTK_WINDOW (window), header);

    struct passwd *pw = getpwuid (getuid ());
    if (pw) {
        char      *greeting = g_strdup_printf ("Neko Void — %s", pw->pw_name);
        GtkWidget *label    = gtk_label_new (greeting);
        g_free (greeting);
        gtk_widget_add_css_class (label, "title-label");
        gtk_header_bar_set_title_widget (GTK_HEADER_BAR (header), label);
    }

    char      *css_path  = get_resource_path ("data/style.css");
    GFile     *css_file  = g_file_new_for_path (css_path);
    GtkCssProvider *prov = gtk_css_provider_new ();
    gtk_css_provider_load_from_file (prov, css_file);
    gtk_style_context_add_provider_for_display (gdk_display_get_default (),
                                                GTK_STYLE_PROVIDER (prov),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref (prov);
    g_object_unref (css_file);
    g_free (css_path);

#ifdef GDK_WINDOWING_X11
    g_signal_connect (window, "map", G_CALLBACK (on_window_map), NULL);
#endif

    gtk_window_present (GTK_WINDOW (window));
    return window;
}
