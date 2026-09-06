#include "window.h"
#include "apps_manager.h"
#include "app_card.h"
#include "installer.h"
#include "mirror_manager.h"
#include <gtk/gtk.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#include <X11/Xlib.h>
#endif

// Layout scale. Every margin in this file is one of these or a multiple of 4,
// so the pages stay on a single rhythm. See data/style.css for the rest.
#define PAGE_PAD 24
#define GRID_GAP 12

typedef enum {
    LANG_EN = 0,
    LANG_ES,
    LANG_JA,
    LANG_IT
} Language;

struct _NekoStoreWindow {
    GtkApplicationWindow parent_instance;
    GtkWidget *stack;

    // Welcome Page
    GtkWidget *welcome_page;
    GtkWidget *welcome_title;
    GtkWidget *welcome_subtitle;
    GtkWidget *start_btn;
    GtkWidget *about_btn;

    // Apps Pages
    GtkWidget *gaming_page;
    GtkWidget *gaming_flowbox;

    GtkWidget *drawing_image_page;
    GtkWidget *drawing_image_flowbox;

    GtkWidget *audio_video_page;
    GtkWidget *audio_video_flowbox;

    GtkWidget *text_documents_page;
    GtkWidget *text_documents_flowbox;

    GtkWidget *social_page;
    GtkWidget *social_flowbox;

    // Drivers Page
    GtkWidget *drivers_page;
    GtkWidget *drivers_flowbox;

    // Security Page
    GtkWidget *security_page;
    GtkWidget *security_flowbox;

    // Finished Page
    GtkWidget *finished_page;
    GtkWidget *finished_label;
    GtkWidget *progress_bar;
    GtkWidget *status_label;

    // Mirror Page
    GtkWidget *mirror_page;
    GtkWidget *mirror_status_label;
    MirrorInfo *selected_mirror;

    // Installer State
    GList *apps_to_install;
    GList *current_installing;
    guint pulse_id;
    gboolean installing;
    int language;
    GtkWidget *lang_btn;
};

G_DEFINE_TYPE (NekoStoreWindow, neko_store_window, GTK_TYPE_APPLICATION_WINDOW)

static void term_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *msg = g_strdup_vprintf(fmt, args);
    va_end(args);
    g_print("%s\n", msg);
    g_free(msg);
    fflush(stdout);
}

// Live theme reload; see watch_user_theme() near the bottom of this file.
static GtkCssProvider *theme_provider = NULL;
static GFileMonitor *theme_monitor = NULL;

static void go_to_gaming_page(GtkButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    gtk_stack_set_visible_child(GTK_STACK(self->stack), self->gaming_page);
}

static void go_to_drawing_image_page(GtkButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    gtk_stack_set_visible_child(GTK_STACK(self->stack), self->drawing_image_page);
}

static void go_to_audio_video_page(GtkButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    gtk_stack_set_visible_child(GTK_STACK(self->stack), self->audio_video_page);
}

static void go_to_text_documents_page(GtkButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    gtk_stack_set_visible_child(GTK_STACK(self->stack), self->text_documents_page);
}

static void go_to_social_page(GtkButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    gtk_stack_set_visible_child(GTK_STACK(self->stack), self->social_page);
}

static void go_to_drivers_page(GtkButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    gtk_stack_set_visible_child(GTK_STACK(self->stack), self->drivers_page);
}

static void go_to_security_page(GtkButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    gtk_stack_set_visible_child(GTK_STACK(self->stack), self->security_page);
}

static void go_to_mirror_page(GtkButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    gtk_stack_set_visible_child(GTK_STACK(self->stack), self->mirror_page);
}

static void on_group_toggle_toggled(GtkCheckButton *btn, gpointer user_data) {
    GtkWidget *flowbox = GTK_WIDGET(user_data);
    gboolean active = gtk_check_button_get_active(btn);

    GtkWidget *child = gtk_widget_get_first_child(flowbox);
    while (child != NULL) {
        GtkWidget *card = gtk_flow_box_child_get_child(GTK_FLOW_BOX_CHILD(child));
        if (NEKO_IS_APP_CARD(card)) {
            neko_app_card_set_selected(NEKO_APP_CARD(card), active);
        }
        child = gtk_widget_get_next_sibling(child);
    }
}

static GtkWidget* create_app_group_page(NekoStoreWindow *self, const char *title, AppGroup group_filter, GtkWidget **flowbox_out, GCallback back_cb, GCallback next_cb, const char *next_btn_label, gboolean is_final_step) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Header
    GtkWidget *header = gtk_label_new(title);
    gtk_widget_add_css_class(header, "page-header");
    gtk_widget_set_halign(header, GTK_ALIGN_START);
    gtk_widget_set_margin_top(header, PAGE_PAD);
    gtk_widget_set_margin_start(header, PAGE_PAD);
    gtk_widget_set_margin_end(header, PAGE_PAD);
    gtk_box_append(GTK_BOX(vbox), header);

    // Toolbar: lives outside the scroll area so "Select All" stays reachable.
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_add_css_class(toolbar, "toolbar");
    gtk_widget_set_margin_top(toolbar, 16);
    gtk_widget_set_margin_start(toolbar, PAGE_PAD);
    gtk_widget_set_margin_end(toolbar, PAGE_PAD);

    GtkWidget *toggle_all = gtk_check_button_new_with_label("Select All");
    gtk_widget_set_halign(toggle_all, GTK_ALIGN_END);
    gtk_widget_set_hexpand(toggle_all, TRUE);
    gtk_box_append(GTK_BOX(toolbar), toggle_all);
    gtk_box_append(GTK_BOX(vbox), toolbar);

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_widget_set_margin_start(scrolled, PAGE_PAD);
    gtk_widget_set_margin_end(scrolled, PAGE_PAD);
    // Never scroll sideways: the grid reflows to whatever width it is given.
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget *flowbox = gtk_flow_box_new();
    gtk_widget_set_valign(flowbox, GTK_ALIGN_START);
    gtk_widget_set_halign(flowbox, GTK_ALIGN_FILL);
    gtk_widget_set_margin_top(flowbox, 16);
    gtk_widget_set_margin_bottom(flowbox, PAGE_PAD);
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(flowbox), 2);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flowbox), 8);
    gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(flowbox), TRUE);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flowbox), GTK_SELECTION_NONE);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flowbox), GRID_GAP);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flowbox), GRID_GAP);

    g_signal_connect(toggle_all, "toggled", G_CALLBACK(on_group_toggle_toggled), flowbox);

    *flowbox_out = flowbox;

    // Load apps
    GList *apps = get_all_apps();
    for (GList *l = apps; l != NULL; l = l->next) {
        AppInfo *info = (AppInfo *)l->data;
        if (info->group == group_filter) {
            GtkWidget *card = neko_app_card_new(info);
            gtk_flow_box_insert(GTK_FLOW_BOX(flowbox), card, -1);
        }
    }
    g_list_free(apps);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), flowbox);
    gtk_box_append(GTK_BOX(vbox), scrolled);

    // Footer buttons box
    GtkWidget *footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_add_css_class(footer, "footer");
    gtk_widget_set_margin_top(footer, 16);
    gtk_widget_set_margin_bottom(footer, PAGE_PAD);
    gtk_widget_set_margin_end(footer, PAGE_PAD);
    gtk_widget_set_margin_start(footer, PAGE_PAD);

    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);

    if (back_cb != NULL) {
        GtkWidget *back_btn = gtk_button_new_with_label("Back");
        gtk_widget_add_css_class(back_btn, "suggested-action");
        gtk_widget_set_halign(back_btn, GTK_ALIGN_START);
        g_signal_connect(back_btn, "clicked", back_cb, self);
        gtk_box_append(GTK_BOX(footer), back_btn);
    }

    gtk_box_append(GTK_BOX(footer), spacer);

    GtkWidget *next_btn = gtk_button_new_with_label(next_btn_label);
    gtk_widget_add_css_class(next_btn, "suggested-action");
    gtk_widget_add_css_class(next_btn, "install-selected-btn"); // using same styling
    gtk_widget_set_halign(next_btn, GTK_ALIGN_END);
    g_signal_connect(next_btn, "clicked", next_cb, self);
    gtk_box_append(GTK_BOX(footer), next_btn);

    gtk_box_append(GTK_BOX(vbox), footer);

    return vbox;
}


static void install_next_app(NekoStoreWindow *self);

static gboolean pulse_progress_bar(gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(self->progress_bar));
    return G_SOURCE_CONTINUE;
}

static void install_finished_cb(gboolean success, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);

    if (self->current_installing) {
        AppInfo *info = (AppInfo *)self->current_installing->data;
        info->install_success = success;
    }

    self->current_installing = self->current_installing->next;
    install_next_app(self);
}

static void install_progress_cb(const char *status_message, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    if (status_message && g_utf8_validate(status_message, -1, NULL)) {
        g_print("%s\n", status_message);
        fflush(stdout);
        char *trunc = g_strndup(status_message, 60);
        gtk_label_set_text(GTK_LABEL(self->status_label), trunc);
        g_free(trunc);
    }
}

static void install_next_app(NekoStoreWindow *self) {
    if (self->current_installing != NULL) {
        AppInfo *info = (AppInfo *)self->current_installing->data;
        char *status = g_strdup_printf("Installing %s...", info->name);
        gtk_label_set_text(GTK_LABEL(self->status_label), status);
        g_free(status);
        term_log("Installing %s...", info->name);

        install_app_async(info->install_command, install_progress_cb, install_finished_cb, self);
    } else {
        // Finished everything
        self->installing = FALSE;
        if (self->pulse_id > 0) {
            g_source_remove(self->pulse_id);
            self->pulse_id = 0;
        }
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(self->progress_bar), 1.0);

        GList *failed = NULL;
        for (GList *l = self->apps_to_install; l != NULL; l = l->next) {
            AppInfo *info = (AppInfo *)l->data;
            if (!info->install_success) {
                failed = g_list_append(failed, info);
            }
        }
        if (failed) {
            g_print("\n");
            term_log("Some apps reported a failure. Check the log above for details:");
            for (GList *l = failed; l != NULL; l = l->next) {
                AppInfo *info = (AppInfo *)l->data;
                g_print("  - %s\n", info->name);
                fflush(stdout);
            }
            g_list_free(failed);
        } else {
            term_log("All selected apps installed successfully.");
        }
        term_log("Neko Void is ready! you can close this window");
        gtk_label_set_text(GTK_LABEL(self->finished_label), "Neko Void is ready! you can close this window");
        gtk_label_set_text(GTK_LABEL(self->status_label), "All installations finished.");
    }
}

static void system_update_finished_cb(gboolean success, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    if (!success) {
        gtk_label_set_text(GTK_LABEL(self->status_label), "System update failed, continuing with apps...");
        term_log("System update failed, continuing with apps...");
    } else {
        gtk_label_set_text(GTK_LABEL(self->status_label), "System update OK, installing apps...");
        term_log("System update OK, installing apps...");
    }
    gtk_label_set_text(GTK_LABEL(self->finished_label), "Installing Apps...");
    install_next_app(self);
}

static void on_install_selected_clicked(GtkButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);

    // An install run is already in progress (the user can navigate back while
    // apps are installing): starting a second chain would make both runs share
    // current_installing and advance the same list, skipping apps.
    if (self->installing) {
        return;
    }
    self->installing = TRUE;

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
    g_print("\n");
    term_log("Starting Neko Void setup with %d selected app(s)...", g_list_length(self->apps_to_install));
    term_log("%s", status);
    g_free(status);
    install_app_async("pkexec xbps-install -y -Syu", install_progress_cb, system_update_finished_cb, self);
}

static void on_about_clicked(GtkButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    (void)btn;

    const char *title_text;
    const char *license_text;
    const char *desc;
    const char *close_text;
    switch (self->language) {
        case LANG_ES:
            title_text = "Acerca de Neko Void";
            license_text = "Bajo la licencia WTFPL";
            desc = "Un asistente sencillo e instalador de controladores, "
                   "diseñado específicamente para Void Linux.\n"
                   "Seleccione las aplicaciones que desee y Neko Void "
                   "preparará su sistema.";
            close_text = "Cerrar";
            break;
        case LANG_JA:
            title_text = "Neko Void について";
            license_text = "WTFPL ライセンスの下で公開";
            desc = "Void Linux 専用に設計された、シンプルなウィザード兼"
                   "ドライバーインストーラーです。\n"
                   "インストールしたいアプリを選ぶだけで、"
                   "Neko Void がシステムをセットアップします。";
            close_text = "閉じる";
            break;
        case LANG_IT:
            title_text = "Informazioni su Neko Void";
            license_text = "Concesso in licenza WTFPL";
            desc = "Una semplice procedura guidata e installer di driver, "
                   "progettato appositamente per Void Linux.\n"
                   "Seleziona le app che vuoi e Neko Void "
                   "prepara il tuo sistema.";
            close_text = "Chiudi";
            break;
        default:
            title_text = "About Neko Void";
            license_text = "Licensed under the WTFPL";
            desc = "A straightforward wizard and driver installer "
                   "designed specifically for Void Linux.\n"
                   "Select the apps you want and Neko Void "
                   "sets up your system.";
            close_text = "Close";
            break;
    }

    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(self));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_title(GTK_WINDOW(dialog), title_text);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, -1);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(box, 28);
    gtk_widget_set_margin_bottom(box, 24);
    gtk_widget_set_margin_start(box, 28);
    gtk_widget_set_margin_end(box, 28);
    gtk_window_set_child(GTK_WINDOW(dialog), box);

    char *logo_path = get_resource_path("resources/logo.png");
    GtkWidget *icon = gtk_image_new_from_file(logo_path);
    g_free(logo_path);
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 96);
    gtk_widget_set_halign(icon, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), icon);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Neko Void</b>");
    gtk_widget_add_css_class(title, "page-header");
    gtk_label_set_justify(GTK_LABEL(title), GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(box), title);

    GtkWidget *license = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(license), license_text);
    gtk_widget_add_css_class(license, "dim-label");
    gtk_label_set_justify(GTK_LABEL(license), GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(box), license);

    GtkWidget *description = gtk_label_new(desc);
    gtk_label_set_wrap(GTK_LABEL(description), TRUE);
    gtk_label_set_justify(GTK_LABEL(description), GTK_JUSTIFY_CENTER);
    gtk_widget_add_css_class(description, "dim-label");
    gtk_widget_set_margin_top(description, 8);
    gtk_box_append(GTK_BOX(box), description);

    GtkWidget *close_btn = gtk_button_new_with_label(close_text);
    gtk_widget_add_css_class(close_btn, "suggested-action");
    gtk_widget_set_halign(close_btn, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(close_btn, 12);
    g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
    gtk_box_append(GTK_BOX(box), close_btn);

    gtk_window_present(GTK_WINDOW(dialog));
}

static void update_language(NekoStoreWindow *self, Language lang) {
    self->language = lang;

    const char *title;
    const char *subtitle;
    const char *start;
    const char *about;
    const char *lang_name;
    switch (lang) {
        case LANG_ES:
            title = "Bienvenido a Neko Void";
            subtitle = "Preparemos su sistema con sus aplicaciones favoritas.";
            start = "Iniciar Configuración";
            about = "Acerca de";
            lang_name = "Español";
            break;
        case LANG_JA:
            title = "Neko Void へようこそ";
            subtitle = "お気に入りのアプリでシステムを準備しましょう。";
            start = "セットアップを開始";
            about = "このアプリについて";
            lang_name = "日本語";
            break;
        case LANG_IT:
            title = "Benvenuto a Neko Void";
            subtitle = "Prepariamo il tuo sistema con le tue app preferite.";
            start = "Avvia la configurazione";
            about = "Informazioni";
            lang_name = "Italiano";
            break;
        default:
            title = "Welcome to Neko Void";
            subtitle = "Let's get your system ready with your favorite apps.";
            start = "Start Setup";
            about = "About";
            lang_name = "English";
            break;
    }

    if (self->welcome_title) gtk_label_set_text(GTK_LABEL(self->welcome_title), title);
    if (self->welcome_subtitle) gtk_label_set_text(GTK_LABEL(self->welcome_subtitle), subtitle);
    if (self->start_btn) gtk_button_set_label(GTK_BUTTON(self->start_btn), start);
    if (self->about_btn) gtk_button_set_label(GTK_BUTTON(self->about_btn), about);
    if (self->lang_btn) gtk_menu_button_set_label(GTK_MENU_BUTTON(self->lang_btn), lang_name);
}

static void on_language_option_toggled(GtkCheckButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    if (!gtk_check_button_get_active(btn)) return;

    update_language(self, GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "lang")));

    GtkWidget *popover = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "popover"));
    if (popover) gtk_popover_popdown(GTK_POPOVER(popover));
}

static void build_welcome_page(NekoStoreWindow *self) {
    // Top bar: About on the left, language picker on the right.
    GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_widget_set_valign(top_bar, GTK_ALIGN_START);
    gtk_widget_set_hexpand(top_bar, TRUE);

    self->about_btn = gtk_button_new_with_label("About");
    gtk_widget_add_css_class(self->about_btn, "about-btn");
    gtk_widget_set_valign(self->about_btn, GTK_ALIGN_CENTER);
    g_signal_connect(self->about_btn, "clicked", G_CALLBACK(on_about_clicked), self);
    gtk_box_append(GTK_BOX(top_bar), self->about_btn);

    GtkWidget *lang_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(lang_box, GTK_ALIGN_END);
    gtk_widget_set_hexpand(lang_box, TRUE);
    gtk_widget_set_valign(lang_box, GTK_ALIGN_CENTER);

    GtkWidget *lang_popover = gtk_popover_new();
    GtkWidget *lang_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_popover_set_child(GTK_POPOVER(lang_popover), lang_list);

    GtkWidget *lang_en = gtk_check_button_new_with_label("English");
    GtkWidget *lang_es = gtk_check_button_new_with_label("Español");
    GtkWidget *lang_ja = gtk_check_button_new_with_label("日本語");
    GtkWidget *lang_it = gtk_check_button_new_with_label("Italiano");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(lang_es), GTK_CHECK_BUTTON(lang_en));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(lang_ja), GTK_CHECK_BUTTON(lang_en));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(lang_it), GTK_CHECK_BUTTON(lang_en));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(lang_en), TRUE);
    g_object_set_data(G_OBJECT(lang_en), "lang", GINT_TO_POINTER(LANG_EN));
    g_object_set_data(G_OBJECT(lang_es), "lang", GINT_TO_POINTER(LANG_ES));
    g_object_set_data(G_OBJECT(lang_ja), "lang", GINT_TO_POINTER(LANG_JA));
    g_object_set_data(G_OBJECT(lang_it), "lang", GINT_TO_POINTER(LANG_IT));
    g_object_set_data(G_OBJECT(lang_en), "popover", lang_popover);
    g_object_set_data(G_OBJECT(lang_es), "popover", lang_popover);
    g_object_set_data(G_OBJECT(lang_ja), "popover", lang_popover);
    g_object_set_data(G_OBJECT(lang_it), "popover", lang_popover);
    g_signal_connect(lang_en, "toggled", G_CALLBACK(on_language_option_toggled), self);
    g_signal_connect(lang_es, "toggled", G_CALLBACK(on_language_option_toggled), self);
    g_signal_connect(lang_ja, "toggled", G_CALLBACK(on_language_option_toggled), self);
    g_signal_connect(lang_it, "toggled", G_CALLBACK(on_language_option_toggled), self);
    gtk_box_append(GTK_BOX(lang_list), lang_en);
    gtk_box_append(GTK_BOX(lang_list), lang_es);
    gtk_box_append(GTK_BOX(lang_list), lang_ja);
    gtk_box_append(GTK_BOX(lang_list), lang_it);

    self->lang_btn = gtk_menu_button_new();
    gtk_menu_button_set_label(GTK_MENU_BUTTON(self->lang_btn), "English");
    gtk_widget_add_css_class(self->lang_btn, "lang-btn");
    gtk_widget_set_valign(self->lang_btn, GTK_ALIGN_CENTER);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(self->lang_btn), lang_popover);
    gtk_box_append(GTK_BOX(lang_box), self->lang_btn);

    gtk_box_append(GTK_BOX(top_bar), lang_box);

    // Vertical rhythm comes from the CSS margins, hence spacing 0.
    GtkWidget *main_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(main_content, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(main_content, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(main_content, TRUE);

    GtkWidget *icon;
    char *logo_path = get_resource_path("resources/logo.png");
    icon = gtk_image_new_from_file(logo_path);
    g_free(logo_path);
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 128);
    gtk_widget_add_css_class(icon, "welcome-icon");

    self->welcome_title = gtk_label_new("Welcome to Neko Void");
    gtk_widget_add_css_class(self->welcome_title, "welcome-title");

    self->welcome_subtitle = gtk_label_new("Let's get your system ready with your favorite apps.");
    gtk_widget_add_css_class(self->welcome_subtitle, "welcome-subtitle");

    self->start_btn = gtk_button_new_with_label("Start Setup");
    gtk_widget_add_css_class(self->start_btn, "suggested-action");
    gtk_widget_add_css_class(self->start_btn, "start-btn");
    gtk_widget_set_halign(self->start_btn, GTK_ALIGN_CENTER);
    g_signal_connect(self->start_btn, "clicked", G_CALLBACK(go_to_mirror_page), self);

    gtk_box_append(GTK_BOX(main_content), icon);
    gtk_box_append(GTK_BOX(main_content), self->welcome_title);
    gtk_box_append(GTK_BOX(main_content), self->welcome_subtitle);
    gtk_box_append(GTK_BOX(main_content), self->start_btn);

    // Assemble the whole structure
    // We want the top bar at the strict top, and main content centered.
    GtkWidget *welcome_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_top(top_bar, PAGE_PAD);
    gtk_widget_set_margin_start(top_bar, PAGE_PAD);
    gtk_widget_set_margin_end(top_bar, PAGE_PAD);

    gtk_box_append(GTK_BOX(welcome_container), top_bar);
    gtk_box_append(GTK_BOX(welcome_container), main_content);

    // Re-assign self->welcome_page to the container, since it expects a widget to be added to the stack
    self->welcome_page = welcome_container;
}

static void build_finished_page(NekoStoreWindow *self) {
    self->finished_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_valign(self->finished_page, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(self->finished_page, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start(self->finished_page, PAGE_PAD);
    gtk_widget_set_margin_end(self->finished_page, PAGE_PAD);

    self->finished_label = gtk_label_new("Installing Apps...");
    gtk_widget_add_css_class(self->finished_label, "page-header");
    gtk_label_set_wrap(GTK_LABEL(self->finished_label), TRUE);
    gtk_label_set_justify(GTK_LABEL(self->finished_label), GTK_JUSTIFY_CENTER);
    gtk_label_set_max_width_chars(GTK_LABEL(self->finished_label), 40);

    self->progress_bar = gtk_progress_bar_new();
    gtk_widget_add_css_class(self->progress_bar, "install-progress");
    gtk_widget_set_hexpand(self->progress_bar, TRUE);
    gtk_widget_set_size_request(self->progress_bar, 320, -1);
    gtk_widget_set_margin_top(self->progress_bar, 32);

    self->status_label = gtk_label_new("Preparing...");
    gtk_widget_add_css_class(self->status_label, "dim-label");
    gtk_widget_set_margin_top(self->status_label, 12);
    gtk_label_set_ellipsize(GTK_LABEL(self->status_label), PANGO_ELLIPSIZE_END);
    gtk_label_set_max_width_chars(GTK_LABEL(self->status_label), 48);

    gtk_box_append(GTK_BOX(self->finished_page), self->finished_label);
    gtk_box_append(GTK_BOX(self->finished_page), self->progress_bar);
    gtk_box_append(GTK_BOX(self->finished_page), self->status_label);
}

static void on_mirror_selected(GtkCheckButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    if (gtk_check_button_get_active(btn)) {
        self->selected_mirror = g_object_get_data(G_OBJECT(btn), "mirror");
    }
}

static void test_progress_cb(const char *line, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    if (line && g_utf8_validate(line, -1, NULL)) {
        if (g_str_has_prefix(line, "OK:")) {
            char **parts = g_strsplit(line, ":", 3);
            if (parts && parts[0] && parts[1] && parts[2]) {
                double connect_time = g_ascii_strtod(parts[2], NULL);
                char *status = g_strdup_printf("✓ Reachable (HTTP %s, %.0fms)", parts[1], connect_time * 1000);
                gtk_label_set_text(GTK_LABEL(self->mirror_status_label), status);
                g_free(status);
            }
            g_strfreev(parts);
        } else if (g_str_equal(line, "FAIL")) {
            gtk_label_set_text(GTK_LABEL(self->mirror_status_label), "✗ Unreachable");
        } else {
            char *trunc = g_strndup(line, 80);
            gtk_label_set_text(GTK_LABEL(self->mirror_status_label), trunc);
            g_free(trunc);
        }
    }
}

static void test_finished_cb(gboolean success, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    const char *text = gtk_label_get_text(GTK_LABEL(self->mirror_status_label));
    if (!text || strlen(text) == 0 || g_str_equal(text, "Testing connectivity...")) {
        gtk_label_set_text(GTK_LABEL(self->mirror_status_label), "✗ Connection failed");
    }
}

static void on_test_mirror_clicked(GtkButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    MirrorInfo *mirror = g_object_get_data(G_OBJECT(btn), "mirror");
    if (!mirror) return;

    gtk_label_set_text(GTK_LABEL(self->mirror_status_label), "Testing connectivity...");

    const char *base = mirror->url;
    int len = strlen(base);
    char *url;
    if (len > 0 && base[len-1] == '/') {
        url = g_strdup(base);
    } else {
        url = g_strdup_printf("%s/", base);
    }

    char *command = g_strdup_printf(
        "curl -s --connect-timeout 5 --max-time 10 -o /dev/null -w \"OK:%%{http_code}:%%{time_connect}\" \"%s\" 2>/dev/null || echo \"FAIL\"",
        url
    );

    install_app_async(command, test_progress_cb, test_finished_cb, self);
    g_free(command);
    g_free(url);
}

static void on_mirror_set_finished_cb(gboolean success, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    if (self->selected_mirror && success) {
        gtk_label_set_text(GTK_LABEL(self->mirror_status_label), "Mirror set successfully!");
    } else {
        gtk_label_set_text(GTK_LABEL(self->mirror_status_label), "Mirror configured. Continuing...");
    }
    gtk_stack_set_visible_child(GTK_STACK(self->stack), self->gaming_page);
}

static void on_mirror_next_clicked(GtkButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);

    if (!self->selected_mirror) {
        gtk_label_set_text(GTK_LABEL(self->mirror_status_label), "Please select a mirror first.");
        return;
    }

    char *command = g_strdup_printf("pkexec xmirror --set %s", self->selected_mirror->url);
    gtk_label_set_text(GTK_LABEL(self->mirror_status_label), "Setting system mirror...");
    install_app_async(command, test_progress_cb, on_mirror_set_finished_cb, self);
    g_free(command);
}

static void build_mirror_page(NekoStoreWindow *self);

static void go_to_welcome_page(GtkButton *btn, gpointer user_data);

static void build_mirror_page(NekoStoreWindow *self) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget *header = gtk_label_new("Choose Repository");
    gtk_widget_add_css_class(header, "page-header");
    gtk_widget_set_halign(header, GTK_ALIGN_START);
    gtk_widget_set_margin_top(header, PAGE_PAD);
    gtk_widget_set_margin_start(header, PAGE_PAD);
    gtk_widget_set_margin_end(header, PAGE_PAD);
    gtk_box_append(GTK_BOX(vbox), header);

    GtkWidget *desc = gtk_label_new("Select a Void Linux mirror to use for installations. Test connectivity to find the fastest mirror.");
    gtk_widget_add_css_class(desc, "page-desc");
    gtk_widget_set_margin_top(desc, 4);
    gtk_widget_set_margin_start(desc, PAGE_PAD);
    gtk_widget_set_margin_end(desc, PAGE_PAD);
    gtk_label_set_wrap(GTK_LABEL(desc), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(desc), 60);
    gtk_label_set_xalign(GTK_LABEL(desc), 0.0);
    gtk_box_append(GTK_BOX(vbox), desc);

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_widget_set_margin_top(scrolled, 16);
    gtk_widget_set_margin_start(scrolled, PAGE_PAD);
    gtk_widget_set_margin_end(scrolled, PAGE_PAD);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget *list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    GtkCheckButton *first_radio = NULL;
    int current_tier = 0;
    gboolean first_mirror = TRUE;

    GList *mirrors = get_all_mirrors();
    for (GList *l = mirrors; l != NULL; l = l->next) {
        MirrorInfo *mirror = (MirrorInfo *)l->data;

        if (mirror->tier != current_tier) {
            current_tier = mirror->tier;
            GtkWidget *tier_header = gtk_label_new(NULL);
            char *markup;
            if (current_tier == 1) {
                markup = g_strdup("<b>Tier 1 Mirrors</b>  <small>(maintained by Void Linux Team)</small>");
            } else {
                markup = g_strdup("<b>Tier 2 Mirrors</b>  <small>(community mirrors, sync from Tier 1)</small>");
            }
            gtk_label_set_markup(GTK_LABEL(tier_header), markup);
            g_free(markup);
            gtk_label_set_xalign(GTK_LABEL(tier_header), 0.0);
            gtk_widget_add_css_class(tier_header, "mirror-section-header");
            gtk_widget_set_margin_top(tier_header, 20);
            gtk_widget_set_margin_bottom(tier_header, 4);
            gtk_box_append(GTK_BOX(list_box), tier_header);
        }

        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_add_css_class(row, "mirror-row");

        GtkWidget *radio = gtk_check_button_new();
        if (first_radio != NULL) {
            gtk_check_button_set_group(GTK_CHECK_BUTTON(radio), first_radio);
        } else {
            first_radio = GTK_CHECK_BUTTON(radio);
        }
        if (first_mirror) {
            gtk_check_button_set_active(GTK_CHECK_BUTTON(radio), TRUE);
            self->selected_mirror = mirror;
            first_mirror = FALSE;
        }
        g_object_set_data(G_OBJECT(radio), "mirror", mirror);
        g_signal_connect(radio, "toggled", G_CALLBACK(on_mirror_selected), self);
        gtk_box_append(GTK_BOX(row), radio);

        GtkWidget *info_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_widget_set_hexpand(info_box, TRUE);

        // No [T1]/[T2] prefix: the tier is already the section header above.
        GtkWidget *name_label = gtk_label_new(mirror->name);
        gtk_widget_add_css_class(name_label, "mirror-name");
        gtk_label_set_xalign(GTK_LABEL(name_label), 0.0);
        gtk_label_set_ellipsize(GTK_LABEL(name_label), PANGO_ELLIPSIZE_END);
        gtk_box_append(GTK_BOX(info_box), name_label);

        char *detail = g_strdup_printf("%s  •  %s, %s", mirror->url, mirror->region, mirror->location);
        GtkWidget *detail_label = gtk_label_new(detail);
        g_free(detail);
        gtk_widget_add_css_class(detail_label, "mirror-detail");
        gtk_label_set_xalign(GTK_LABEL(detail_label), 0.0);
        // Long mirror URLs would otherwise pin the window to their full width.
        gtk_label_set_ellipsize(GTK_LABEL(detail_label), PANGO_ELLIPSIZE_END);
        gtk_box_append(GTK_BOX(info_box), detail_label);

        gtk_box_append(GTK_BOX(row), info_box);

        GtkWidget *test_btn = gtk_button_new_with_label("Test");
        gtk_widget_add_css_class(test_btn, "mirror-test-btn");
        g_object_set_data(G_OBJECT(test_btn), "mirror", mirror);
        g_signal_connect(test_btn, "clicked", G_CALLBACK(on_test_mirror_clicked), self);
        gtk_box_append(GTK_BOX(row), test_btn);

        gtk_box_append(GTK_BOX(list_box), row);
    }
    g_list_free(mirrors);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list_box);
    gtk_box_append(GTK_BOX(vbox), scrolled);

    self->mirror_status_label = gtk_label_new("Select a mirror and click Test to check connectivity");
    gtk_widget_add_css_class(self->mirror_status_label, "dim-label");
    gtk_widget_set_hexpand(self->mirror_status_label, TRUE);
    gtk_widget_set_margin_top(self->mirror_status_label, 16);
    gtk_widget_set_margin_start(self->mirror_status_label, PAGE_PAD);
    gtk_widget_set_margin_end(self->mirror_status_label, PAGE_PAD);
    gtk_label_set_xalign(GTK_LABEL(self->mirror_status_label), 0.0);
    gtk_label_set_ellipsize(GTK_LABEL(self->mirror_status_label), PANGO_ELLIPSIZE_END);
    gtk_box_append(GTK_BOX(vbox), self->mirror_status_label);

    GtkWidget *footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_add_css_class(footer, "footer");
    gtk_widget_set_margin_top(footer, 16);
    gtk_widget_set_margin_bottom(footer, PAGE_PAD);
    gtk_widget_set_margin_end(footer, PAGE_PAD);
    gtk_widget_set_margin_start(footer, PAGE_PAD);

    GtkWidget *back_btn = gtk_button_new_with_label("Back");
    gtk_widget_add_css_class(back_btn, "suggested-action");
    g_signal_connect(back_btn, "clicked", G_CALLBACK(go_to_welcome_page), self);
    gtk_box_append(GTK_BOX(footer), back_btn);

    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(footer), spacer);

    GtkWidget *next_btn = gtk_button_new_with_label("Next");
    gtk_widget_add_css_class(next_btn, "suggested-action");
    gtk_widget_add_css_class(next_btn, "install-selected-btn");
    g_signal_connect(next_btn, "clicked", G_CALLBACK(on_mirror_next_clicked), self);
    gtk_box_append(GTK_BOX(footer), next_btn);

    gtk_box_append(GTK_BOX(vbox), footer);

    self->mirror_page = vbox;
}

static void go_to_welcome_page(GtkButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    gtk_stack_set_visible_child(GTK_STACK(self->stack), self->welcome_page);
}

static void neko_store_window_init (NekoStoreWindow *self) {
    self->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(self->stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);

    build_welcome_page(self);
    build_mirror_page(self);

    self->gaming_page = create_app_group_page(self, "Step 1: Gaming Apps", GROUP_GAMING, &self->gaming_flowbox, G_CALLBACK(go_to_mirror_page), G_CALLBACK(go_to_drawing_image_page), "Next", FALSE);
    self->drawing_image_page = create_app_group_page(self, "Step 2: Drawing and Image Editing", GROUP_DRAWING_IMAGE, &self->drawing_image_flowbox, G_CALLBACK(go_to_gaming_page), G_CALLBACK(go_to_audio_video_page), "Next", FALSE);
    self->audio_video_page = create_app_group_page(self, "Step 3: Audio & Video Editing", GROUP_AUDIO_VIDEO, &self->audio_video_flowbox, G_CALLBACK(go_to_drawing_image_page), G_CALLBACK(go_to_text_documents_page), "Next", FALSE);
    self->text_documents_page = create_app_group_page(self, "Step 4: Text Editing and Documents", GROUP_TEXT_DOCUMENTS, &self->text_documents_flowbox, G_CALLBACK(go_to_audio_video_page), G_CALLBACK(go_to_social_page), "Next", FALSE);
    self->social_page = create_app_group_page(self, "Step 5: Social Apps and Internet", GROUP_SOCIAL, &self->social_flowbox, G_CALLBACK(go_to_text_documents_page), G_CALLBACK(go_to_drivers_page), "Next", FALSE);
    self->drivers_page = create_app_group_page(self, "Step 6: Drivers", GROUP_DRIVERS, &self->drivers_flowbox, G_CALLBACK(go_to_social_page), G_CALLBACK(go_to_security_page), "Next", FALSE);
    self->security_page = create_app_group_page(self, "Step 7: Security", GROUP_SECURITY, &self->security_flowbox, G_CALLBACK(go_to_social_page), G_CALLBACK(on_install_selected_clicked), "Install", TRUE);
    build_finished_page(self);

    gtk_stack_add_named(GTK_STACK(self->stack), self->welcome_page, "welcome");
    gtk_stack_add_named(GTK_STACK(self->stack), self->mirror_page, "mirror");
    gtk_stack_add_named(GTK_STACK(self->stack), self->gaming_page, "gaming");
    gtk_stack_add_named(GTK_STACK(self->stack), self->drawing_image_page, "drawing");
    gtk_stack_add_named(GTK_STACK(self->stack), self->audio_video_page, "audio_video");
    gtk_stack_add_named(GTK_STACK(self->stack), self->text_documents_page, "text_documents");
    gtk_stack_add_named(GTK_STACK(self->stack), self->social_page, "social");
    gtk_stack_add_named(GTK_STACK(self->stack), self->drivers_page, "drivers");
    gtk_stack_add_named(GTK_STACK(self->stack), self->security_page, "security");
    gtk_stack_add_named(GTK_STACK(self->stack), self->finished_page, "finished");

    gtk_window_set_child(GTK_WINDOW(self), self->stack);
}

static void neko_store_window_dispose(GObject *object) {
    NekoStoreWindow *self = (NekoStoreWindow *)object;
    if (self->pulse_id > 0) {
        g_source_remove(self->pulse_id);
        self->pulse_id = 0;
    }
    if (self->apps_to_install) {
        g_list_free(self->apps_to_install);
        self->apps_to_install = NULL;
    }
    g_clear_object(&theme_monitor);
    g_clear_object(&theme_provider);
    G_OBJECT_CLASS(neko_store_window_parent_class)->dispose(object);
}

static void on_window_map(GtkWidget *widget, gpointer data) {
#ifdef GDK_WINDOWING_X11
    GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(widget));
    if (surface && GDK_IS_X11_SURFACE(surface)) {
        Display *xdisplay = gdk_x11_display_get_xdisplay(gdk_surface_get_display(surface));
        Window xid = gdk_x11_surface_get_xid(surface);

        int w = gtk_widget_get_width(widget);
        int h = gtk_widget_get_height(widget);
        if (w <= 0) w = 1000;
        if (h <= 0) h = 700;

        int screen = DefaultScreen(xdisplay);
        int sw = DisplayWidth(xdisplay, screen);
        int sh = DisplayHeight(xdisplay, screen);

        if (sw > w && sh > h) {
            XMoveWindow(xdisplay, xid, (sw - w) / 2, (sh - h) / 2);
        }
    }
#endif
}

/* --- Live theme reload -----------------------------------------------------
 * GTK reads ~/.config/gtk-4.0/gtk.css exactly once, at startup, and never
 * watches it afterwards. Noctalia rewrites the colours it pulls in whenever the
 * system theme changes, so without this the app keeps whatever palette it
 * launched with until it is restarted.
 *
 * Re-loading the same file into our own provider one step above GTK's own USER
 * provider makes the fresh values win. Nothing else about precedence changes:
 * the user's CSS already outranked ours at 800.
 * -------------------------------------------------------------------------- */
static char *user_gtk_css_path(void) {
    return g_build_filename(g_get_user_config_dir(), "gtk-4.0", "gtk.css", NULL);
}

static void reload_user_theme_css(void) {
    if (theme_provider == NULL) return;

    char *path = user_gtk_css_path();
    if (g_file_test(path, G_FILE_TEST_EXISTS)) {
        GFile *file = g_file_new_for_path(path);
        gtk_css_provider_load_from_file(theme_provider, file);
        g_object_unref(file);
    }
    g_free(path);
}

static void on_theme_dir_changed(GFileMonitor *monitor, GFile *file, GFile *other_file,
                                 GFileMonitorEvent event, gpointer user_data) {
    if (event == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT ||
        event == G_FILE_MONITOR_EVENT_CREATED ||
        event == G_FILE_MONITOR_EVENT_RENAMED) {
        reload_user_theme_css();
    }
}

static void watch_user_theme(void) {
    char *path = user_gtk_css_path();
    gboolean exists = g_file_test(path, G_FILE_TEST_EXISTS);
    g_free(path);
    // No user CSS at all: the fallback palette in style.css stands on its own.
    if (!exists) return;

    theme_provider = gtk_css_provider_new();
    reload_user_theme_css();
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                               GTK_STYLE_PROVIDER(theme_provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_USER + 1);

    // Watch the directory, not the file: gtk.css only @imports the generated
    // palette, so its own mtime never moves when the theme changes.
    char *dir_path = g_build_filename(g_get_user_config_dir(), "gtk-4.0", NULL);
    GFile *dir = g_file_new_for_path(dir_path);
    g_free(dir_path);

    theme_monitor = g_file_monitor_directory(dir, G_FILE_MONITOR_NONE, NULL, NULL);
    g_object_unref(dir);

    if (theme_monitor != NULL) {
        g_signal_connect(theme_monitor, "changed", G_CALLBACK(on_theme_dir_changed), NULL);
    }
}

static void force_square_corners(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, "* { border-radius: 0; }");
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                               GTK_STYLE_PROVIDER(provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_USER + 2);
    g_object_unref(provider);
}

static void neko_store_window_class_init (NekoStoreWindowClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = neko_store_window_dispose;
}

NekoStoreWindow *neko_store_window_new (GtkApplication *app) {
    NekoStoreWindow *window = g_object_new (NEKO_STORE_TYPE_WINDOW, "application", app, NULL);

    gtk_window_set_title (GTK_WINDOW (window), "Neko Void Setup");
    gtk_window_set_default_size (GTK_WINDOW (window), 1000, 700);

    GtkCssProvider *provider = gtk_css_provider_new();
    char *css_path = get_resource_path("data/style.css");
    GFile *css_file = g_file_new_for_path(css_path);
    gtk_css_provider_load_from_file(provider, css_file);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    g_object_unref(css_file);
    g_free(css_path);

    watch_user_theme();
    force_square_corners();

    g_signal_connect(window, "map", G_CALLBACK(on_window_map), NULL);

    gtk_window_maximize(GTK_WINDOW(window));
    gtk_window_present (GTK_WINDOW (window));
    return window;
}
