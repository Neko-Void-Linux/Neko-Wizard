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

// Layout scale. Every margin in this file is one of these or a multiple of 4,
// so the pages stay on a single rhythm. See data/style.css for the rest.
#define PAGE_PAD 24
#define GRID_GAP 8

typedef enum {
    LANG_EN = 0,
    LANG_ES,
    LANG_JA,
    LANG_IT
} Language;

typedef struct {
    const char *en;
    const char *es;
    const char *ja;
    const char *it;
} UiTranslation;

/* Only UI copy lives here. Package names, mirror names and installer output
   remain untouched, as they identify external objects or command output. */
static const UiTranslation ui_translations[] = {
    {"HOME", "INICIO", "ホーム", "INIZIO"},
    {"TOOLS", "HERRAMIENTAS", "ツール", "STRUMENTI"},
    {"Overview", "Resumen", "概要", "Panoramica"},
    {"First Steps", "Primeros pasos", "最初の手順", "Primi passi"},
    {"Software Manager", "Gestor de software", "ソフトウェア管理", "Gestore software"},
    {"Driver Manager", "Gestor de controladores", "ドライバー管理", "Gestore driver"},
    {"Software Sources", "Fuentes de software", "ソフトウェアソース", "Sorgenti software"},
    {"Update Manager", "Gestor de actualizaciones", "更新管理", "Gestore aggiornamenti"},
    {"Firewall", "Cortafuegos", "ファイアウォール", "Firewall"},
    {"CATEGORIES", "CATEGORÍAS", "カテゴリ", "CATEGORIE"},
    {"RECOMMENDED", "RECOMENDADO", "おすすめ", "CONSIGLIATI"},
    {"EXPLORE MORE", "EXPLORAR MÁS", "さらに探す", "ESPLORA ALTRO"},
    {"Gaming", "Juegos", "ゲーム", "Giochi"},
    {"Games", "Juegos", "ゲーム", "Giochi"},
    {"Media", "Multimedia", "メディア", "Media"},
    {"Graphics", "Gráficos", "画像", "Grafica"},
    {"Internet", "Internet", "インターネット", "Internet"},
    {"Documents", "Documentos", "文書", "Documenti"},
    {"Audio and video", "Audio y vídeo", "音声と動画", "Audio e video"},
    {"Drawing and images", "Dibujo e imágenes", "描画と画像", "Disegno e immagini"},
    {"Internet and communication", "Internet y comunicación", "インターネットと通信", "Internet e comunicazione"},
    {"Documents and writing", "Documentos y escritura", "文書と執筆", "Documenti e scrittura"},
    {"Browse a category, select applications, then install them.", "Explora una categoría, selecciona aplicaciones y luego instálalas.", "カテゴリを選び、アプリを選択してインストールします。", "Sfoglia una categoria, seleziona le applicazioni e installale."},
    {"Start with the essentials. Each task can be opened whenever you need it.", "Empieza por lo esencial. Puedes abrir cada tarea cuando la necesites.", "基本から始めましょう。各タスクはいつでも開けます。", "Inizia dall'essenziale. Puoi aprire ogni attività quando vuoi."},
    {"Install available system updates.", "Instala las actualizaciones disponibles del sistema.", "利用可能なシステム更新をインストールします。", "Installa gli aggiornamenti di sistema disponibili."},
    {"Test and change the system repository mirror.", "Prueba y cambia el servidor réplica del sistema.", "システムのリポジトリミラーを確認・変更します。", "Verifica e cambia il mirror dei repository di sistema."},
    {"Set up graphics, Bluetooth, and printer support.", "Configura gráficos, Bluetooth e impresoras.", "グラフィック、Bluetooth、印刷のサポートを設定します。", "Configura grafica, Bluetooth e stampanti."},
    {"Browse and install applications by category.", "Explora e instala aplicaciones por categoría.", "カテゴリ別にアプリを探してインストールします。", "Sfoglia e installa applicazioni per categoria."},
    {"Install GUFW and enable UFW.", "Instala GUFW y activa UFW.", "GUFW をインストールして UFW を有効にします。", "Installa GUFW e abilita UFW."},
    {"Choose the device support you need. NVIDIA versions are separate alternatives.", "Elige los controladores necesarios. Las versiones de NVIDIA son alternativas independientes.", "必要なデバイスサポートを選択してください。NVIDIA の各バージョンは別の選択肢です。", "Scegli il supporto necessario. Le versioni NVIDIA sono alternative separate."},
    {"The firewall option installs GUFW and enables UFW.", "La opción de cortafuegos instala GUFW y activa UFW.", "ファイアウォールの項目は GUFW をインストールし、UFW を有効にします。", "L'opzione firewall installa GUFW e abilita UFW."},
    {"Choose the items you want, then install them when you are ready.", "Elige los elementos que deseas e instálalos cuando estés listo.", "必要な項目を選び、準備ができたらインストールします。", "Scegli gli elementi desiderati e installali quando sei pronto."},
    {"Select all, including hidden results", "Seleccionar todo, incluso lo oculto", "非表示の結果もすべて選択", "Seleziona tutto, anche i risultati nascosti"},
    {"Search apps…", "Buscar aplicaciones…", "アプリを検索…", "Cerca applicazioni…"},
    {"Search apps", "Buscar aplicaciones", "アプリを検索", "Cerca applicazioni"},
    {"Search drivers…", "Buscar controladores…", "ドライバーを検索…", "Cerca driver…"},
    {"Search drivers", "Buscar controladores", "ドライバーを検索", "Cerca driver"},
    {"Search tools…", "Buscar herramientas…", "ツールを検索…", "Cerca strumenti…"},
    {"Search tools", "Buscar herramientas", "ツールを検索", "Cerca strumenti"},
    {"Select every app in this category, including results hidden by search.", "Selecciona todas las aplicaciones de esta categoría, incluso las ocultas por la búsqueda.", "検索で非表示の項目を含め、このカテゴリの全アプリを選択します。", "Seleziona tutte le applicazioni della categoria, anche quelle nascoste dalla ricerca."},
    {"Filters visible items. Existing selections are kept.", "Filtra los elementos visibles. Se conservan las selecciones existentes.", "表示する項目を絞り込みます。既存の選択は保持されます。", "Filtra gli elementi visibili. Le selezioni esistenti restano attive."},
    {"No apps match this search.", "No hay aplicaciones que coincidan.", "一致するアプリはありません。", "Nessuna applicazione corrisponde alla ricerca."},
    {"No drivers match this search.", "No hay controladores que coincidan.", "一致するドライバーはありません。", "Nessun driver corrisponde alla ricerca."},
    {"No tools match this search.", "No hay herramientas que coincidan.", "一致するツールはありません。", "Nessuno strumento corrisponde alla ricerca."},
    {"Install selected", "Instalar seleccionados", "選択項目をインストール", "Installa selezionati"},
    {"Install selected drivers", "Instalar controladores seleccionados", "選択したドライバーをインストール", "Installa driver selezionati"},
    {"Install selected tools", "Instalar herramientas seleccionadas", "選択したツールをインストール", "Installa strumenti selezionati"},
    {"Install and enable firewall", "Instalar y activar cortafuegos", "ファイアウォールをインストールして有効化", "Installa e abilita firewall"},
    {"Back to tools", "Volver a herramientas", "ツールに戻る", "Torna agli strumenti"},
    {"Back to categories", "Volver a categorías", "カテゴリに戻る", "Torna alle categorie"},
    {"Back", "Volver", "戻る", "Indietro"},
    {"Refresh package data and install available updates from your configured Void Linux repositories.", "Actualiza los datos de paquetes e instala las actualizaciones de los repositorios configurados de Void Linux.", "設定済みの Void Linux リポジトリからパッケージ情報を更新し、利用可能な更新をインストールします。", "Aggiorna i dati dei pacchetti e installa gli aggiornamenti dai repository Void Linux configurati."},
    {"Install available updates", "Instalar actualizaciones disponibles", "利用可能な更新をインストール", "Installa aggiornamenti disponibili"},
    {"Select a system mirror and optionally test its connectivity before applying it.", "Elige un servidor réplica y, si quieres, prueba la conexión antes de aplicarlo.", "システムミラーを選び、必要なら適用前に接続を確認します。", "Scegli un mirror di sistema e, se vuoi, verifica la connessione prima di applicarlo."},
    {"Tier 1 Mirrors (maintained by Void Linux Team)", "Réplica de nivel 1 (mantenidas por Void Linux)", "Tier 1 ミラー（Void Linux チームが管理）", "Mirror Tier 1 (gestiti dal team Void Linux)"},
    {"Tier 2 Mirrors (community mirrors, sync from Tier 1)", "Réplica de nivel 2 (comunidad; sincronizadas desde nivel 1)", "Tier 2 ミラー（コミュニティ管理、Tier 1 と同期）", "Mirror Tier 2 (comunità; sincronizzati da Tier 1)"},
    {"Test", "Probar", "テスト", "Verifica"},
    {"Apply mirror", "Aplicar réplica", "ミラーを適用", "Applica mirror"},
    {"Select a mirror and click Test to check connectivity", "Elige una réplica y pulsa Probar para comprobar la conexión", "ミラーを選び、テストで接続を確認してください", "Scegli un mirror e premi Verifica per controllare la connessione"},
    {"%d selected", "%d seleccionados", "%d 件選択", "%d selezionati"},
    {"%d apps", "%d aplicaciones", "%d アプリ", "%d applicazioni"},
    {"%d app", "%d aplicación", "%d アプリ", "%d applicazione"},
    {"%d apps · %d selected", "%d aplicaciones · %d seleccionadas", "%d アプリ · %d 件選択", "%d applicazioni · %d selezionate"},
    {"%d app · %d selected", "%d aplicación · %d seleccionada", "%d アプリ · %d 件選択", "%d applicazione · %d selezionata"},
    {"Installing %s (%d of %d)…", "Instalando %s (%d de %d)…", "%s をインストール中（%d/%d）…", "Installazione di %s (%d di %d)…"},
    {"Could not install: ", "No se pudo instalar: ", "インストール失敗: ", "Installazione non riuscita: "},
    {"Installation finished with errors", "La instalación terminó con errores", "インストールはエラーで終了しました", "Installazione terminata con errori"},
    {"Installation complete", "Instalación completada", "インストール完了", "Installazione completata"},
    {"All selected items installed successfully.", "Todos los elementos seleccionados se instalaron correctamente.", "選択した項目はすべてインストールされました。", "Tutti gli elementi selezionati sono stati installati."},
    {"System update complete", "Actualización del sistema completada", "システム更新完了", "Aggiornamento del sistema completato"},
    {"System update failed", "Falló la actualización del sistema", "システム更新に失敗しました", "Aggiornamento del sistema non riuscito"},
    {"The update command finished successfully.", "El comando de actualización terminó correctamente.", "更新コマンドが正常に終了しました。", "Il comando di aggiornamento è terminato correttamente."},
    {"Check the output above for details, then try again.", "Revisa la salida anterior y vuelve a intentarlo.", "上の出力を確認して再試行してください。", "Controlla l'output qui sopra e riprova."},
    {"Updating system…", "Actualizando el sistema…", "システムを更新中…", "Aggiornamento del sistema…"},
    {"Running xbps-install -Syu", "Ejecutando xbps-install -Syu", "xbps-install -Syu を実行中", "Esecuzione di xbps-install -Syu"},
    {"Nothing selected", "No hay elementos seleccionados", "項目が選択されていません", "Nessun elemento selezionato"},
    {"Choose at least one item in this category to install.", "Elige al menos un elemento de esta categoría para instalar.", "このカテゴリから少なくとも 1 件選択してください。", "Scegli almeno un elemento di questa categoria da installare."},
    {"Installing selected items…", "Instalando los elementos seleccionados…", "選択項目をインストール中…", "Installazione degli elementi selezionati…"},
    {"Preparing selected packages…", "Preparando los paquetes seleccionados…", "選択したパッケージを準備中…", "Preparazione dei pacchetti selezionati…"},
    {"✓ Reachable (HTTP %s, %.0fms)", "✓ Disponible (HTTP %s, %.0f ms)", "✓ 接続可能 (HTTP %s, %.0f ms)", "✓ Raggiungibile (HTTP %s, %.0f ms)"},
    {"✗ Unreachable", "✗ No disponible", "✗ 接続不可", "✗ Non raggiungibile"},
    {"✗ Connection failed", "✗ Falló la conexión", "✗ 接続失敗", "✗ Connessione non riuscita"},
    {"Testing connectivity...", "Probando la conexión…", "接続を確認中…", "Verifica della connessione…"},
    {"System mirror applied successfully.", "La réplica del sistema se aplicó correctamente.", "システムミラーを適用しました。", "Mirror di sistema applicato correttamente."},
    {"Could not apply the mirror. Check permissions and try again.", "No se pudo aplicar la réplica. Comprueba los permisos e inténtalo de nuevo.", "ミラーを適用できませんでした。権限を確認して再試行してください。", "Impossibile applicare il mirror. Controlla i permessi e riprova."},
    {"Please select a mirror first.", "Primero elige una réplica.", "先にミラーを選択してください。", "Seleziona prima un mirror."},
    {"Setting system mirror...", "Aplicando la réplica del sistema…", "システムミラーを設定中…", "Impostazione del mirror di sistema…"},
};

static const char *ui_tr(Language lang, const char *english) {
    for (gsize i = 0; i < G_N_ELEMENTS(ui_translations); ++i) {
        if (g_strcmp0(ui_translations[i].en, english) != 0) continue;
        switch (lang) {
        case LANG_ES: return ui_translations[i].es;
        case LANG_JA: return ui_translations[i].ja;
        case LANG_IT: return ui_translations[i].it;
        default: return english;
        }
    }
    return english;
}

typedef enum { UI_LABEL, UI_BUTTON, UI_CHECK, UI_PLACEHOLDER, UI_ACCESSIBLE, UI_TOOLTIP } UiTextKind;
typedef struct {
    GtkWidget *widget;
    const char *english;
    UiTextKind kind;
} UiTextBinding;

typedef enum {
    TOOL_ICON_GAMEPAD,
    TOOL_ICON_MEDIA,
    TOOL_ICON_PALETTE,
    TOOL_ICON_GLOBE,
    TOOL_ICON_DOCUMENT,
    TOOL_ICON_UPDATE,
    TOOL_ICON_REPOSITORY,
    TOOL_ICON_HARDWARE,
    TOOL_ICON_SOFTWARE,
    TOOL_ICON_SHIELD
} ToolIcon;

struct _NekoStoreWindow {
    GtkApplicationWindow parent_instance;
    GtkWidget *stack;
    GtkWidget *sidebar;
    GtkWidget *first_steps_page;
    GtkWidget *update_page;
    GPtrArray *navigation_buttons;

    // Welcome Page
    GtkWidget *welcome_page;
    GtkWidget *apps_home_page;
    GtkWidget *category_stack;
    GtkWidget *firewall_category_button;
    GtkWidget *welcome_logo;
    GtkWidget *welcome_title;
    GtkWidget *welcome_subtitle;
    GtkWidget *welcome_body;
    GtkWidget *welcome_start_btn;
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
    GtkWidget *finished_back_btn;
    char *finished_return_page_name;

    // Mirror Page
    GtkWidget *mirror_page;
    GtkWidget *mirror_status_label;
    MirrorInfo *selected_mirror;

    // Installer State
    GList *apps_to_install;
    GList *current_installing;
    gboolean installing;
    int language;
    GtkWidget *lang_btn;
    GPtrArray *selection_count_labels;
    GPtrArray *category_count_labels;
    GPtrArray *ui_text_bindings;
};

G_DEFINE_TYPE (NekoStoreWindow, neko_store_window, GTK_TYPE_APPLICATION_WINDOW)

static void apply_ui_binding(UiTextBinding *binding, Language lang) {
    const char *text = ui_tr(lang, binding->english);
    switch (binding->kind) {
    case UI_LABEL: gtk_label_set_text(GTK_LABEL(binding->widget), text); break;
    case UI_BUTTON: gtk_button_set_label(GTK_BUTTON(binding->widget), text); break;
    case UI_CHECK: gtk_check_button_set_label(GTK_CHECK_BUTTON(binding->widget), text); break;
    case UI_PLACEHOLDER: gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(binding->widget), text); break;
    case UI_ACCESSIBLE:
        gtk_accessible_update_property(GTK_ACCESSIBLE(binding->widget),
                                       GTK_ACCESSIBLE_PROPERTY_LABEL, text, -1);
        break;
    case UI_TOOLTIP: gtk_widget_set_tooltip_text(binding->widget, text); break;
    }
}

static void bind_ui_text(NekoStoreWindow *self, GtkWidget *widget,
                         const char *english, UiTextKind kind) {
    UiTextBinding *binding = g_new0(UiTextBinding, 1);
    binding->widget = widget;
    binding->english = english;
    binding->kind = kind;
    g_ptr_array_add(self->ui_text_bindings, binding);
    apply_ui_binding(binding, (Language)self->language);
}

static void set_ui_status(NekoStoreWindow *self, GtkWidget *label,
                          const char *english) {
    g_object_set_data(G_OBJECT(label), "ui-status-source", (gpointer)english);
    gtk_label_set_text(GTK_LABEL(label), ui_tr((Language)self->language, english));
}

static void use_tabular_numbers(GtkWidget *label) {
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_font_features_new("tnum=1"));
    gtk_label_set_attributes(GTK_LABEL(label), attrs);
    pango_attr_list_unref(attrs);
}

static void on_group_toggle_toggled(GtkCheckButton *btn, gpointer user_data);
static void on_update_system_clicked(GtkButton *btn, gpointer user_data);
static void on_install_selected_clicked(GtkButton *btn, gpointer user_data);
static void on_hub_action_clicked(GtkButton *btn, gpointer user_data);
static void sync_sidebar_selection(NekoStoreWindow *self, const char *page_name);

static void term_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *msg = g_strdup_vprintf(fmt, args);
    va_end(args);
    g_print("%s\n", msg);
    g_free(msg);
    fflush(stdout);
}

static void update_selection_count_labels(NekoStoreWindow *self) {
    for (guint i = 0; self->selection_count_labels && i < self->selection_count_labels->len; ++i) {
        GtkWidget *label = g_ptr_array_index(self->selection_count_labels, i);
        int group_value = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(label), "count-group"));
        AppGroup group = (AppGroup)(group_value - 1);
        int selected_count = 0;
        GList *apps = get_all_apps();
        for (GList *l = apps; l != NULL; l = l->next) {
            AppInfo *info = l->data;
            if (info->selected && info->group == group)
                ++selected_count;
        }
        g_list_free(apps);
        char *summary = g_strdup_printf(ui_tr((Language)self->language, "%d selected"), selected_count);
        gtk_label_set_text(GTK_LABEL(label), summary);
        g_free(summary);
        GtkWidget *install_button = g_object_get_data(G_OBJECT(label), "install-button");
        if (install_button)
            gtk_widget_set_sensitive(install_button, selected_count > 0);
    }

    for (guint i = 0; self->category_count_labels && i < self->category_count_labels->len; ++i) {
        GtkWidget *label = g_ptr_array_index(self->category_count_labels, i);
        int group_value = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(label), "count-group"));
        AppGroup group = (AppGroup)(group_value - 1);
        int total = 0;
        int selected = 0;
        GList *apps = get_all_apps();
        for (GList *l = apps; l != NULL; l = l->next) {
            AppInfo *info = l->data;
            if (info->group != group) continue;
            total++;
            if (info->selected) selected++;
        }
        g_list_free(apps);
        const char *format = selected > 0
            ? (total == 1 ? "%d app · %d selected" : "%d apps · %d selected")
            : (total == 1 ? "%d app" : "%d apps");
        char *summary = selected > 0
            ? g_strdup_printf(ui_tr((Language)self->language, format), total, selected)
            : g_strdup_printf(ui_tr((Language)self->language, format), total);
        gtk_label_set_text(GTK_LABEL(label), summary);
        g_free(summary);
    }
}

static void on_app_card_toggled(GtkToggleButton *button, gpointer user_data) {
    GtkWidget *select_all = g_object_get_data(G_OBJECT(button), "select-all");
    if (select_all) {
        GtkWidget *flowbox_child = gtk_widget_get_parent(GTK_WIDGET(button));
        GtkWidget *flowbox = flowbox_child ? gtk_widget_get_parent(flowbox_child) : NULL;
        gboolean all_selected = flowbox != NULL;
        for (GtkWidget *child = flowbox ? gtk_widget_get_first_child(flowbox) : NULL;
             child != NULL; child = gtk_widget_get_next_sibling(child)) {
            GtkWidget *card = gtk_flow_box_child_get_child(GTK_FLOW_BOX_CHILD(child));
            if (GTK_IS_TOGGLE_BUTTON(card) && !neko_app_card_get_selected(card))
                all_selected = FALSE;
        }
        g_signal_handlers_block_by_func(select_all, G_CALLBACK(on_group_toggle_toggled), flowbox);
        gtk_check_button_set_active(GTK_CHECK_BUTTON(select_all), all_selected);
        g_signal_handlers_unblock_by_func(select_all, G_CALLBACK(on_group_toggle_toggled), flowbox);
    }
    update_selection_count_labels(NEKO_STORE_WINDOW(user_data));
}

// Live theme reload; see watch_user_theme() near the bottom of this file.
static GtkCssProvider *theme_provider = NULL;
static GFileMonitor *theme_monitor = NULL;

static const char *sidebar_target_for_page(const char *page_name) {
    if (!page_name) return NULL;
    if (g_str_equal(page_name, "gaming") || g_str_equal(page_name, "drawing") ||
        g_str_equal(page_name, "audio_video") || g_str_equal(page_name, "text_documents") ||
        g_str_equal(page_name, "social") || g_str_equal(page_name, "security"))
        return "apps_home";
    return page_name;
}

static void sync_sidebar_selection(NekoStoreWindow *self, const char *page_name) {
    const char *target = sidebar_target_for_page(page_name);
    if (!target || !self->navigation_buttons) return;

    for (guint i = 0; i < self->navigation_buttons->len; ++i) {
        GtkWidget *button = g_ptr_array_index(self->navigation_buttons, i);
        const char *button_page = g_object_get_data(G_OBJECT(button), "page-name");
        g_signal_handlers_block_by_func(button, G_CALLBACK(on_hub_action_clicked), self);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), g_strcmp0(button_page, target) == 0);
        g_signal_handlers_unblock_by_func(button, G_CALLBACK(on_hub_action_clicked), self);
    }
}

static void on_hub_action_clicked(GtkButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    const char *page_name = g_object_get_data(G_OBJECT(btn), "page-name");
    if (g_strcmp0(page_name, "security") == 0 && self->category_stack) {
        gtk_stack_set_visible_child_name(GTK_STACK(self->category_stack), "security");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->firewall_category_button), TRUE);
        gtk_stack_set_visible_child_name(GTK_STACK(self->stack), "apps_home");
        sync_sidebar_selection(self, "apps_home");
        return;
    }
    GtkWidget *page = page_name ? gtk_stack_get_child_by_name(GTK_STACK(self->stack), page_name) : NULL;
    if (page) {
        gtk_stack_set_visible_child(GTK_STACK(self->stack), page);
        sync_sidebar_selection(self, page_name);
    }
}

static const char *pixel_icon_name(ToolIcon icon_type) {
    switch (icon_type) {
    case TOOL_ICON_GAMEPAD:    return "neko-gamepad-symbolic";
    case TOOL_ICON_MEDIA:      return "neko-music-symbolic";
    case TOOL_ICON_PALETTE:    return "neko-brush-symbolic";
    case TOOL_ICON_GLOBE:      return "neko-globe-symbolic";
    case TOOL_ICON_DOCUMENT:   return "neko-file-symbolic";
    case TOOL_ICON_UPDATE:     return "neko-refresh-symbolic";
    case TOOL_ICON_REPOSITORY: return "neko-server-symbolic";
    case TOOL_ICON_HARDWARE:   return "neko-cpu-symbolic";
    case TOOL_ICON_SOFTWARE:   return "neko-store-symbolic";
    case TOOL_ICON_SHIELD:     return "neko-shield-symbolic";
    default:                   return NULL;
    }
}

static GtkWidget *create_tool_button(ToolIcon icon_type, const char *title,
                                    const char *description, const char *page_name,
                                    NekoStoreWindow *self) {
    GtkWidget *button = gtk_button_new();
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *icon = gtk_image_new_from_icon_name(pixel_icon_name(icon_type));
    gtk_widget_set_size_request(icon, 24, 24);
    gtk_widget_add_css_class(icon, "tool-icon");
    gtk_widget_set_halign(icon, GTK_ALIGN_CENTER);
    // Keep the text block's top edge consistent across cards, even when
    // localized titles or descriptions wrap to different numbers of lines.
    gtk_widget_set_valign(icon, GTK_ALIGN_START);
    gtk_widget_set_can_target(icon, FALSE);
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 22);

    GtkWidget *text = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    GtkWidget *heading = gtk_label_new(title);
    GtkWidget *detail = gtk_label_new(description);

    gtk_widget_add_css_class(button, "tool-launcher");
    gtk_widget_add_css_class(heading, "tool-title");
    gtk_widget_add_css_class(detail, "tool-description");
    bind_ui_text(self, heading, title, UI_LABEL);
    bind_ui_text(self, detail, description, UI_LABEL);
    gtk_label_set_xalign(GTK_LABEL(heading), 0.0);
    gtk_label_set_wrap(GTK_LABEL(heading), TRUE);
    gtk_label_set_xalign(GTK_LABEL(detail), 0.0);
    gtk_label_set_wrap(GTK_LABEL(detail), TRUE);
    gtk_widget_set_hexpand(text, TRUE);
    gtk_widget_set_valign(icon, GTK_ALIGN_START);
    gtk_widget_set_valign(text, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(text), heading);
    gtk_box_append(GTK_BOX(text), detail);
    gtk_box_append(GTK_BOX(content), icon);
    gtk_box_append(GTK_BOX(content), text);
    gtk_button_set_child(GTK_BUTTON(button), content);
    gtk_widget_set_hexpand(button, TRUE);
    gtk_accessible_update_property(GTK_ACCESSIBLE(button),
                                   GTK_ACCESSIBLE_PROPERTY_LABEL, title,
                                   -1);
    bind_ui_text(self, button, title, UI_ACCESSIBLE);
    g_object_set_data_full(G_OBJECT(button), "page-name", g_strdup(page_name), g_free);
    g_signal_connect(button, "clicked", G_CALLBACK(on_hub_action_clicked), self);
    return button;
}

static GtkWidget *create_sidebar_button(NekoStoreWindow *self, const char *title, const char *page_name) {
    GtkWidget *button = gtk_toggle_button_new();
    GtkWidget *label = gtk_label_new(title);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_label_set_single_line_mode(GTK_LABEL(label), TRUE);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_button_set_child(GTK_BUTTON(button), label);
    gtk_widget_add_css_class(button, "sidebar-button");
    bind_ui_text(self, label, title, UI_LABEL);
    if (self->navigation_buttons->len > 0) {
        GtkWidget *first = g_ptr_array_index(self->navigation_buttons, 0);
        gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(button), GTK_TOGGLE_BUTTON(first));
    }
    g_object_set_data_full(G_OBJECT(button), "page-name", g_strdup(page_name), g_free);
    g_signal_connect(button, "clicked", G_CALLBACK(on_hub_action_clicked), self);
    g_ptr_array_add(self->navigation_buttons, button);
    return button;
}

static void on_group_toggle_toggled(GtkCheckButton *btn, gpointer user_data) {
    GtkWidget *flowbox = GTK_WIDGET(user_data);
    gboolean active = gtk_check_button_get_active(btn);

    GtkWidget *child = gtk_widget_get_first_child(flowbox);
    while (child != NULL) {
        GtkWidget *card = gtk_flow_box_child_get_child(GTK_FLOW_BOX_CHILD(child));
        if (GTK_IS_TOGGLE_BUTTON(card)) {
            neko_app_card_set_selected(card, active);
        }
        child = gtk_widget_get_next_sibling(child);
    }
}

static gboolean app_card_matches_search(GtkFlowBoxChild *child, gpointer user_data) {
    GtkWidget *card = gtk_flow_box_child_get_child(child);
    AppInfo *info = g_object_get_data(G_OBJECT(card), "app-info");
    const char *query = gtk_editable_get_text(GTK_EDITABLE(user_data));
    if (!info || !query || query[0] == '\0')
        return TRUE;

    g_autofree char *folded_query = g_utf8_casefold(query, -1);
    g_autofree char *folded_name = g_utf8_casefold(info->name, -1);
    return g_strstr_len(folded_name, -1, folded_query) != NULL;
}

static void on_app_search_changed(GtkSearchEntry *entry, gpointer user_data) {
    GtkFlowBox *flowbox = GTK_FLOW_BOX(user_data);
    gtk_flow_box_invalidate_filter(flowbox);

    guint matches = 0;
    for (GtkWidget *child = gtk_widget_get_first_child(GTK_WIDGET(flowbox));
         child != NULL; child = gtk_widget_get_next_sibling(child)) {
        if (app_card_matches_search(GTK_FLOW_BOX_CHILD(child), entry))
            ++matches;
    }

    GtkWidget *empty_state = g_object_get_data(G_OBJECT(flowbox), "empty-state");
    if (empty_state)
        gtk_widget_set_visible(empty_state, matches == 0);
}

static GtkWidget* create_app_group_page(NekoStoreWindow *self, int step_number, const char *title, AppGroup group_filter, GtkWidget **flowbox_out, GCallback next_cb, const char *next_btn_label, gboolean embedded) {
    (void)step_number;
    const int inner_pad = embedded ? 12 : PAGE_PAD;
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Category views are independent tools rather than mandatory wizard steps.
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_top(header_box, embedded ? 4 : PAGE_PAD);
    gtk_widget_set_margin_start(header_box, inner_pad);
    gtk_widget_set_margin_end(header_box, inner_pad);
    gtk_widget_set_hexpand(header_box, TRUE);

    GtkWidget *header = gtk_label_new(title);
    gtk_widget_add_css_class(header, embedded ? "category-page-title" : "page-header");
    bind_ui_text(self, header, title, UI_LABEL);
    gtk_widget_set_halign(header, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(header, TRUE);
    gtk_label_set_xalign(GTK_LABEL(header), 0.0);
    gtk_label_set_wrap(GTK_LABEL(header), TRUE);
    gtk_box_append(GTK_BOX(header_box), header);

    if (!embedded || group_filter == GROUP_SECURITY) {
        const char *help = group_filter == GROUP_DRIVERS
            ? "Choose the device support you need. NVIDIA versions are separate alternatives."
            : group_filter == GROUP_SECURITY
            ? "The firewall option installs GUFW and enables UFW."
            : "Choose the items you want, then install them when you are ready.";
        GtkWidget *description = gtk_label_new(help);
        gtk_widget_add_css_class(description, "page-desc");
        bind_ui_text(self, description, help, UI_LABEL);
        gtk_widget_set_halign(description, GTK_ALIGN_FILL);
        gtk_widget_set_hexpand(description, TRUE);
        gtk_label_set_xalign(GTK_LABEL(description), 0.0);
        gtk_label_set_wrap(GTK_LABEL(description), TRUE);
        gtk_box_append(GTK_BOX(header_box), description);
    }

    gtk_box_append(GTK_BOX(vbox), header_box);

    // Keep bulk selection explicit and report the count for this category.
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_add_css_class(toolbar, "toolbar");
    gtk_widget_set_margin_top(toolbar, 12);
    gtk_widget_set_margin_start(toolbar, inner_pad);
    gtk_widget_set_margin_end(toolbar, inner_pad);

    GtkWidget *toggle_all = gtk_check_button_new_with_label("Select all, including hidden results");
    bind_ui_text(self, toggle_all, "Select all, including hidden results", UI_CHECK);
    gtk_widget_set_tooltip_text(toggle_all,
        "Select every app in this category, including results hidden by search.");
    bind_ui_text(self, toggle_all,
                 "Select every app in this category, including results hidden by search.", UI_TOOLTIP);
    gtk_widget_set_visible(toggle_all, group_filter != GROUP_DRIVERS && group_filter != GROUP_SECURITY);
    GtkWidget *search_entry = gtk_search_entry_new();
    const char *search_placeholder = group_filter == GROUP_DRIVERS ? "Search drivers…"
                                   : group_filter == GROUP_SECURITY ? "Search tools…" : "Search apps…";
    const char *search_name = group_filter == GROUP_DRIVERS ? "Search drivers"
                              : group_filter == GROUP_SECURITY ? "Search tools" : "Search apps";
    gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(search_entry), search_placeholder);
    bind_ui_text(self, search_entry, search_placeholder, UI_PLACEHOLDER);
    gtk_widget_add_css_class(search_entry, "package-search");
    gtk_widget_set_size_request(search_entry, 140, -1);
    gtk_widget_set_hexpand(search_entry, TRUE);
    gtk_widget_set_tooltip_text(search_entry,
        "Filters visible items. Existing selections are kept.");
    bind_ui_text(self, search_entry,
                 "Filters visible items. Existing selections are kept.", UI_TOOLTIP);
    gtk_accessible_update_property(GTK_ACCESSIBLE(search_entry),
                                   GTK_ACCESSIBLE_PROPERTY_LABEL, search_name,
                                   -1);
    bind_ui_text(self, search_entry, search_name, UI_ACCESSIBLE);
    gtk_box_append(GTK_BOX(toolbar), search_entry);

    GtkWidget *selection_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(selection_row), toggle_all);

    GtkWidget *selection_count = gtk_label_new("0 selected");
    use_tabular_numbers(selection_count);
    gtk_widget_add_css_class(selection_count, "selection-count");
    gtk_widget_set_halign(selection_count, GTK_ALIGN_END);
    gtk_widget_set_hexpand(selection_count, TRUE);
    gtk_box_append(GTK_BOX(selection_row), selection_count);
    gtk_box_append(GTK_BOX(toolbar), selection_row);
    if (self->selection_count_labels)
        g_ptr_array_add(self->selection_count_labels, selection_count);
    g_object_set_data(G_OBJECT(selection_count), "count-group", GINT_TO_POINTER(group_filter + 1));
    gtk_box_append(GTK_BOX(vbox), toolbar);

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_widget_set_margin_start(scrolled, inner_pad);
    gtk_widget_set_margin_end(scrolled, inner_pad);
    // Never scroll sideways: the grid reflows to whatever width it is given.
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget *flowbox = gtk_flow_box_new();
    gtk_widget_set_valign(flowbox, GTK_ALIGN_START);
    gtk_widget_set_halign(flowbox, GTK_ALIGN_FILL);
    gtk_widget_set_margin_top(flowbox, 16);
    gtk_widget_set_margin_bottom(flowbox, inner_pad);
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(flowbox), 1);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flowbox), 1);
    gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(flowbox), TRUE);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flowbox), GTK_SELECTION_NONE);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flowbox), GRID_GAP);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flowbox), GRID_GAP);
    gtk_flow_box_set_filter_func(GTK_FLOW_BOX(flowbox), app_card_matches_search,
                                 search_entry, NULL);
    g_signal_connect(search_entry, "search-changed",
                     G_CALLBACK(on_app_search_changed), flowbox);

    const char *empty_text = group_filter == GROUP_DRIVERS ? "No drivers match this search."
                            : group_filter == GROUP_SECURITY ? "No tools match this search."
                            : "No apps match this search.";
    GtkWidget *empty_label = gtk_label_new(empty_text);
    bind_ui_text(self, empty_label, empty_text, UI_LABEL);
    gtk_widget_add_css_class(empty_label, "empty-state");
    gtk_widget_set_halign(empty_label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(empty_label, GTK_ALIGN_CENTER);
    gtk_widget_set_visible(empty_label, FALSE);
    g_object_set_data(G_OBJECT(flowbox), "empty-state", empty_label);

    g_signal_connect(toggle_all, "toggled", G_CALLBACK(on_group_toggle_toggled), flowbox);

    *flowbox_out = flowbox;

    // Load apps
    GList *apps = get_all_apps();
    for (GList *l = apps; l != NULL; l = l->next) {
        AppInfo *info = (AppInfo *)l->data;
        if (info->group == group_filter) {
            GtkWidget *card = neko_app_card_new(info);
            g_object_set_data(G_OBJECT(card), "select-all", toggle_all);
            g_signal_connect(card, "toggled", G_CALLBACK(on_app_card_toggled), self);
            gtk_flow_box_insert(GTK_FLOW_BOX(flowbox), card, -1);
        }
    }
    g_list_free(apps);
    update_selection_count_labels(self);

    GtkWidget *flow_overlay = gtk_overlay_new();
    gtk_overlay_set_child(GTK_OVERLAY(flow_overlay), flowbox);
    gtk_overlay_add_overlay(GTK_OVERLAY(flow_overlay), empty_label);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), flow_overlay);
    gtk_box_append(GTK_BOX(vbox), scrolled);

    // Footer buttons box
    GtkWidget *footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_add_css_class(footer, "footer");
    gtk_widget_set_margin_top(footer, 8);
    gtk_widget_set_margin_bottom(footer, inner_pad);
    gtk_widget_set_margin_end(footer, inner_pad);
    gtk_widget_set_margin_start(footer, inner_pad);

    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);

    gtk_box_append(GTK_BOX(footer), spacer);

    GtkWidget *next_btn = gtk_button_new_with_label(next_btn_label);
    bind_ui_text(self, next_btn, next_btn_label, UI_BUTTON);
    gtk_widget_add_css_class(next_btn, "suggested-action");
    gtk_widget_add_css_class(next_btn, "install-selected-btn"); // using same styling
    gtk_widget_set_halign(next_btn, GTK_ALIGN_END);
    g_object_set_data(G_OBJECT(next_btn), "install-group", GINT_TO_POINTER(group_filter + 1));
    g_signal_connect(next_btn, "clicked", next_cb, self);
    gtk_box_append(GTK_BOX(footer), next_btn);
    g_object_set_data(G_OBJECT(selection_count), "install-button", next_btn);
    update_selection_count_labels(self);

    gtk_box_append(GTK_BOX(vbox), footer);

    return vbox;
}


static void install_next_app(NekoStoreWindow *self);

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
        int total = g_list_length(self->apps_to_install);
        int index = g_list_position(self->apps_to_install, self->current_installing);
        if (index < 0) index = 0;
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(self->progress_bar),
                                      total > 0 ? (double)index / total : 0.0);
        char *status = g_strdup_printf(ui_tr((Language)self->language, "Installing %s (%d of %d)…"), info->name, index + 1, total);
        gtk_label_set_text(GTK_LABEL(self->status_label), status);
        g_free(status);
        term_log("Installing %s...", info->name);

        install_app_async(info->install_command, install_progress_cb, install_finished_cb, self);
    } else {
        // Finished everything
        self->installing = FALSE;
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(self->progress_bar), 1.0);

        GList *failed = NULL;
        for (GList *l = self->apps_to_install; l != NULL; l = l->next) {
            AppInfo *info = (AppInfo *)l->data;
            if (!info->install_success) {
                failed = g_list_append(failed, info);
            }
        }
        gboolean has_failures = failed != NULL;
        if (has_failures) {
            g_print("\n");
            term_log("Some selected items could not be installed:");
            const char *failure_prefix = ui_tr((Language)self->language, "Could not install: ");
            GString *failure_summary = g_string_new(failure_prefix);
            for (GList *l = failed; l != NULL; l = l->next) {
                AppInfo *info = (AppInfo *)l->data;
                g_print("  - %s\n", info->name);
                fflush(stdout);
                if (failure_summary->len > strlen(failure_prefix))
                    g_string_append(failure_summary, ", ");
                g_string_append(failure_summary, info->name);
            }
            gtk_label_set_text(GTK_LABEL(self->status_label), failure_summary->str);
            g_string_free(failure_summary, TRUE);
            g_list_free(failed);
        } else {
            term_log("All selected items installed successfully.");
        }
        if (has_failures) {
            gtk_label_set_text(GTK_LABEL(self->finished_label), ui_tr((Language)self->language, "Installation finished with errors"));
        } else {
            term_log("Neko Wizard is ready! You can close this window.");
            gtk_label_set_text(GTK_LABEL(self->finished_label), ui_tr((Language)self->language, "Installation complete"));
            gtk_label_set_text(GTK_LABEL(self->status_label), ui_tr((Language)self->language, "All selected items installed successfully."));
        }
        gtk_widget_set_visible(self->finished_back_btn, TRUE);
    }
}

static void system_update_finished_cb(gboolean success, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    self->installing = FALSE;
    gtk_label_set_text(GTK_LABEL(self->finished_label), ui_tr((Language)self->language, success ? "System update complete" : "System update failed"));
    gtk_label_set_text(GTK_LABEL(self->status_label), ui_tr((Language)self->language, success ? "The update command finished successfully." : "Check the output above for details, then try again."));
    gtk_widget_set_visible(self->finished_back_btn, TRUE);
}

static void on_update_system_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    if (self->installing)
        return;

    g_free(self->finished_return_page_name);
    const char *current_page = gtk_stack_get_visible_child_name(GTK_STACK(self->stack));
    self->finished_return_page_name = g_strdup(current_page ? current_page : "first_steps");
    self->installing = TRUE;
    gtk_widget_set_visible(self->finished_back_btn, FALSE);
    gtk_stack_set_visible_child(GTK_STACK(self->stack), self->finished_page);
    gtk_label_set_text(GTK_LABEL(self->finished_label), ui_tr((Language)self->language, "Updating system…"));
    gtk_label_set_text(GTK_LABEL(self->status_label), ui_tr((Language)self->language, "Running xbps-install -Syu"));
    gtk_widget_set_visible(self->progress_bar, FALSE);
    install_app_async("pkexec xbps-install -y -Syu", install_progress_cb, system_update_finished_cb, self);
}

static void on_install_selected_clicked(GtkButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);

    // An install run is already in progress (the user can navigate back while
    // apps are installing): starting a second chain would make both runs share
    // current_installing and advance the same list, skipping apps.
    if (self->installing) {
        return;
    }
    g_free(self->finished_return_page_name);
    const char *current_page = gtk_stack_get_visible_child_name(GTK_STACK(self->stack));
    self->finished_return_page_name = g_strdup(current_page ? current_page : "first_steps");
    self->installing = TRUE;

    // Install only selections in the current tool/category.
    int group_value = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "install-group"));
    AppGroup selected_group = (AppGroup)(group_value - 1);
    g_list_free(self->apps_to_install);
    self->apps_to_install = NULL;

    GList *apps = get_all_apps();
    for (GList *l = apps; l != NULL; l = l->next) {
        AppInfo *info = (AppInfo *)l->data;
        if (info->selected && info->group == selected_group) {
            self->apps_to_install = g_list_append(self->apps_to_install, info);
        }
    }
    g_list_free(apps);

    gtk_stack_set_visible_child(GTK_STACK(self->stack), self->finished_page);

    gtk_widget_set_visible(self->finished_back_btn, FALSE);
    if (self->apps_to_install == NULL) {
        self->installing = FALSE;
        gtk_widget_set_visible(self->progress_bar, FALSE);
        gtk_label_set_text(GTK_LABEL(self->finished_label), ui_tr((Language)self->language, "Nothing selected"));
        gtk_label_set_text(GTK_LABEL(self->status_label), ui_tr((Language)self->language, "Choose at least one item in this category to install."));
        gtk_widget_set_visible(self->finished_back_btn, TRUE);
        gtk_stack_set_visible_child(GTK_STACK(self->stack), self->finished_page);
        return;
    }
    gtk_label_set_text(GTK_LABEL(self->finished_label), ui_tr((Language)self->language, "Installing selected items…"));
    gtk_widget_set_visible(self->progress_bar, TRUE);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(self->progress_bar), 0.0);

    self->current_installing = self->apps_to_install;
    gtk_label_set_text(GTK_LABEL(self->status_label), ui_tr((Language)self->language, "Preparing selected packages…"));
    g_print("\n");
    term_log("Installing %d selected item(s)...", g_list_length(self->apps_to_install));
    gtk_stack_set_visible_child(GTK_STACK(self->stack), self->finished_page);
    install_next_app(self);
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
            title_text = "Acerca de Neko Wizard";
            license_text = "Bajo la licencia WTFPL";
            desc = "Herramientas sencillas para preparar Void Linux.\n"
                   "Instale aplicaciones, configure dispositivos, elija "
                   "un espejo o actualice el sistema por separado.";
            close_text = "Cerrar";
            break;
        case LANG_JA:
            title_text = "Neko Wizard について";
            license_text = "WTFPL ライセンスの下で公開";
            desc = "Void Linux を準備するためのシンプルなツール集です。\n"
                   "アプリ、デバイス、ミラー、システム更新を個別に管理できます。";
            close_text = "閉じる";
            break;
        case LANG_IT:
            title_text = "Informazioni su Neko Wizard";
            license_text = "Concesso in licenza WTFPL";
            desc = "Strumenti semplici per configurare Void Linux.\n"
                   "Gestisci separatamente applicazioni, dispositivi, mirror e aggiornamenti.";
            close_text = "Chiudi";
            break;
        default:
            title_text = "About Neko Wizard";
            license_text = "Licensed under the WTFPL";
            desc = "A set of simple tools for preparing Void Linux.\n"
                   "Manage apps, devices, mirrors, and system updates separately.";
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
    gtk_label_set_markup(GTK_LABEL(title), "<b>Neko Wizard</b>");
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
    const char *body;
    const char *start_label;
    const char *about;
    const char *lang_name;
    switch (lang) {
        case LANG_ES:
            title = "Bienvenido a Neko Wizard";
            subtitle = "Herramientas del sistema para Void Linux.";
            body = "Use Primeros pasos para las tareas recomendadas, o elija una herramienta desde la barra lateral.";
            start_label = "Abrir Primeros pasos";
            about = "Acerca de";
            lang_name = "Español";
            break;
        case LANG_JA:
            title = "Neko Wizard へようこそ";
            subtitle = "Void Linux のシステムツール。";
            body = "おすすめの初期設定は「はじめに」から、またはサイドバーからツールを選べます。";
            start_label = "はじめにを開く";
            about = "このアプリについて";
            lang_name = "日本語";
            break;
        case LANG_IT:
            title = "Benvenuto a Neko Wizard";
            subtitle = "Strumenti di sistema per Void Linux.";
            body = "Usa Primi passi per le attività consigliate o scegli uno strumento dalla barra laterale.";
            start_label = "Apri Primi passi";
            about = "Informazioni";
            lang_name = "Italiano";
            break;
        default:
            title = "Welcome to Neko Wizard";
            subtitle = "System tools for Void Linux.";
            body = "Use First Steps for recommended setup tasks, or choose a tool from the sidebar whenever you need it.";
            start_label = "Open First Steps";
            about = "About";
            lang_name = "English";
            break;
    }

    if (self->welcome_title) gtk_label_set_text(GTK_LABEL(self->welcome_title), title);
    if (self->welcome_subtitle) gtk_label_set_text(GTK_LABEL(self->welcome_subtitle), subtitle);
    if (self->welcome_body) gtk_label_set_text(GTK_LABEL(self->welcome_body), body);
    if (self->welcome_start_btn) gtk_button_set_label(GTK_BUTTON(self->welcome_start_btn), start_label);
    if (self->about_btn) gtk_button_set_label(GTK_BUTTON(self->about_btn), about);
    if (self->lang_btn) gtk_menu_button_set_label(GTK_MENU_BUTTON(self->lang_btn), lang_name);
    for (guint i = 0; self->ui_text_bindings && i < self->ui_text_bindings->len; ++i)
        apply_ui_binding(g_ptr_array_index(self->ui_text_bindings, i), lang);
    GtkWidget *dynamic_labels[] = { self->finished_label, self->status_label, self->mirror_status_label };
    for (guint i = 0; i < G_N_ELEMENTS(dynamic_labels); ++i) {
        if (!dynamic_labels[i]) continue;
        const char *source = g_object_get_data(G_OBJECT(dynamic_labels[i]), "ui-status-source");
        if (source) gtk_label_set_text(GTK_LABEL(dynamic_labels[i]), ui_tr(lang, source));
    }
    update_selection_count_labels(self);
}

static void on_language_option_toggled(GtkCheckButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    if (!gtk_check_button_get_active(btn)) return;

    update_language(self, GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "lang")));

    GtkWidget *popover = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "popover"));
    if (popover) gtk_popover_popdown(GTK_POPOVER(popover));
}

static void build_welcome_page(NekoStoreWindow *self) {
    // Top bar: keep the language picker aligned to the right.
    GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_widget_set_valign(top_bar, GTK_ALIGN_START);
    gtk_widget_set_hexpand(top_bar, TRUE);

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

    GtkWidget *main_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_halign(main_content, GTK_ALIGN_FILL);
    gtk_widget_set_valign(main_content, GTK_ALIGN_START);
    gtk_widget_set_hexpand(main_content, TRUE);
    gtk_widget_set_margin_start(main_content, PAGE_PAD);
    gtk_widget_set_margin_end(main_content, PAGE_PAD);
    gtk_widget_set_margin_top(main_content, 16);
    gtk_widget_set_margin_bottom(main_content, 24);

    char *logo_path = get_resource_path("resources/logo.png");
    self->welcome_logo = gtk_image_new_from_file(logo_path);
    g_free(logo_path);
    gtk_image_set_pixel_size(GTK_IMAGE(self->welcome_logo), 56);
    gtk_widget_add_css_class(self->welcome_logo, "welcome-icon");
    gtk_widget_set_halign(self->welcome_logo, GTK_ALIGN_START);

    self->welcome_title = gtk_label_new("Welcome to Neko Wizard");
    gtk_widget_add_css_class(self->welcome_title, "welcome-title");
    gtk_label_set_xalign(GTK_LABEL(self->welcome_title), 0.0);

    self->welcome_subtitle = gtk_label_new("System tools for Void Linux.");
    gtk_widget_add_css_class(self->welcome_subtitle, "welcome-subtitle");
    gtk_label_set_xalign(GTK_LABEL(self->welcome_subtitle), 0.0);

    self->welcome_body = gtk_label_new("Use First Steps for recommended setup tasks, or choose a tool from the sidebar whenever you need it.");
    gtk_widget_add_css_class(self->welcome_body, "welcome-description");
    gtk_label_set_xalign(GTK_LABEL(self->welcome_body), 0.0);
    gtk_label_set_wrap(GTK_LABEL(self->welcome_body), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(self->welcome_body), 68);

    self->welcome_start_btn = gtk_button_new_with_label("Open First Steps");
    gtk_widget_add_css_class(self->welcome_start_btn, "start-btn");
    gtk_widget_set_halign(self->welcome_start_btn, GTK_ALIGN_START);
    gtk_widget_set_margin_top(self->welcome_start_btn, 8);
    g_object_set_data_full(G_OBJECT(self->welcome_start_btn), "page-name",
                           g_strdup("first_steps"), g_free);
    g_signal_connect(self->welcome_start_btn, "clicked",
                     G_CALLBACK(on_hub_action_clicked), self);

    gtk_box_append(GTK_BOX(main_content), self->welcome_logo);
    gtk_box_append(GTK_BOX(main_content), self->welcome_title);
    gtk_box_append(GTK_BOX(main_content), self->welcome_subtitle);
    gtk_box_append(GTK_BOX(main_content), self->welcome_body);
    gtk_box_append(GTK_BOX(main_content), self->welcome_start_btn);

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

static void on_category_clicked(GtkButton *button, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    const char *page_name = g_object_get_data(G_OBJECT(button), "category-page");
    if (!page_name || !self->category_stack) return;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_stack_set_visible_child_name(GTK_STACK(self->category_stack), page_name);
}

static GtkWidget *create_category_button(NekoStoreWindow *self, ToolIcon icon_type,
                                         const char *title, const char *page_name,
                                         AppGroup group, GtkWidget *first_button) {
    GtkWidget *button = gtk_toggle_button_new();
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *icon = gtk_image_new_from_icon_name(pixel_icon_name(icon_type));
    GtkWidget *text = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *heading = gtk_label_new(title);
    GtkWidget *count = gtk_label_new("");
    use_tabular_numbers(count);

    gtk_widget_add_css_class(button, "category-button");
    gtk_widget_add_css_class(icon, "category-icon");
    gtk_widget_add_css_class(heading, "category-nav-title");
    gtk_widget_add_css_class(count, "category-count");
    bind_ui_text(self, heading, title, UI_LABEL);
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 22);
    gtk_widget_set_size_request(icon, 24, 24);
    gtk_widget_set_valign(icon, GTK_ALIGN_CENTER);
    gtk_label_set_xalign(GTK_LABEL(heading), 0);
    gtk_label_set_wrap(GTK_LABEL(heading), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(heading), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_max_width_chars(GTK_LABEL(heading), 18);
    gtk_label_set_xalign(GTK_LABEL(count), 0);
    gtk_box_append(GTK_BOX(text), heading);
    gtk_box_append(GTK_BOX(text), count);
    gtk_widget_set_hexpand(text, TRUE);
    gtk_box_append(GTK_BOX(content), icon);
    gtk_box_append(GTK_BOX(content), text);
    gtk_button_set_child(GTK_BUTTON(button), content);
    if (first_button)
        gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(button), GTK_TOGGLE_BUTTON(first_button));
    g_object_set_data_full(G_OBJECT(button), "category-page", g_strdup(page_name), g_free);
    g_object_set_data(G_OBJECT(count), "count-group", GINT_TO_POINTER(group + 1));
    g_ptr_array_add(self->category_count_labels, count);
    g_signal_connect(button, "clicked", G_CALLBACK(on_category_clicked), self);
    return button;
}

static void build_apps_home_page(NekoStoreWindow *self) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_top(page, PAGE_PAD);
    gtk_widget_set_margin_bottom(page, 8);
    gtk_widget_set_margin_start(page, PAGE_PAD);
    gtk_widget_set_margin_end(page, PAGE_PAD);

    GtkWidget *title = gtk_label_new("Software Manager");
    GtkWidget *description = gtk_label_new("Browse a category, select applications, then install them.");
    gtk_widget_add_css_class(title, "page-header");
    gtk_widget_add_css_class(description, "page-desc");
    bind_ui_text(self, title, "Software Manager", UI_LABEL);
    bind_ui_text(self, description, "Browse a category, select applications, then install them.", UI_LABEL);
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_widget_set_halign(description, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(description, TRUE);
    gtk_label_set_xalign(GTK_LABEL(description), 0);
    gtk_label_set_wrap(GTK_LABEL(description), TRUE);
    gtk_widget_set_margin_bottom(description, 16);
    gtk_box_append(GTK_BOX(page), title);
    gtk_box_append(GTK_BOX(page), description);

    GtkWidget *workspace = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_hexpand(workspace, TRUE);
    gtk_widget_set_vexpand(workspace, TRUE);

    GtkWidget *category_rail = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_add_css_class(category_rail, "category-rail");
    GtkWidget *rail_scrolled = gtk_scrolled_window_new();
    gtk_widget_set_size_request(rail_scrolled, 168, -1);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(rail_scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(rail_scrolled), category_rail);
    GtkWidget *section_title = gtk_label_new("CATEGORIES");
    gtk_widget_add_css_class(section_title, "sidebar-section-title");
    bind_ui_text(self, section_title, "CATEGORIES", UI_LABEL);
    gtk_widget_set_halign(section_title, GTK_ALIGN_START);
    gtk_widget_set_margin_start(section_title, 10);
    gtk_widget_set_margin_bottom(section_title, 6);
    gtk_box_append(GTK_BOX(category_rail), section_title);

    self->category_stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(self->category_stack), GTK_STACK_TRANSITION_TYPE_NONE);
    gtk_widget_set_hexpand(self->category_stack, TRUE);
    gtk_widget_set_vexpand(self->category_stack, TRUE);
    gtk_stack_add_named(GTK_STACK(self->category_stack), self->gaming_page, "gaming");
    gtk_stack_add_named(GTK_STACK(self->category_stack), self->audio_video_page, "audio_video");
    gtk_stack_add_named(GTK_STACK(self->category_stack), self->drawing_image_page, "drawing");
    gtk_stack_add_named(GTK_STACK(self->category_stack), self->social_page, "social");
    gtk_stack_add_named(GTK_STACK(self->category_stack), self->text_documents_page, "text_documents");
    gtk_stack_add_named(GTK_STACK(self->category_stack), self->security_page, "security");

    GtkWidget *first = create_category_button(self, TOOL_ICON_GAMEPAD, "Games", "gaming", GROUP_GAMING, NULL);
    gtk_box_append(GTK_BOX(category_rail), first);
    gtk_box_append(GTK_BOX(category_rail), create_category_button(self, TOOL_ICON_MEDIA, "Media", "audio_video", GROUP_AUDIO_VIDEO, first));
    gtk_box_append(GTK_BOX(category_rail), create_category_button(self, TOOL_ICON_PALETTE, "Graphics", "drawing", GROUP_DRAWING_IMAGE, first));
    gtk_box_append(GTK_BOX(category_rail), create_category_button(self, TOOL_ICON_GLOBE, "Internet", "social", GROUP_SOCIAL, first));
    gtk_box_append(GTK_BOX(category_rail), create_category_button(self, TOOL_ICON_DOCUMENT, "Documents", "text_documents", GROUP_TEXT_DOCUMENTS, first));
    self->firewall_category_button = create_category_button(self, TOOL_ICON_SHIELD, "Firewall", "security", GROUP_SECURITY, first);
    gtk_box_append(GTK_BOX(category_rail), self->firewall_category_button);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(first), TRUE);
    gtk_stack_set_visible_child_name(GTK_STACK(self->category_stack), "gaming");

    gtk_box_append(GTK_BOX(workspace), rail_scrolled);
    gtk_box_append(GTK_BOX(workspace), self->category_stack);
    gtk_box_append(GTK_BOX(page), workspace);
    self->apps_home_page = page;
    update_selection_count_labels(self);
}

static GtkWidget *build_first_steps_page(NekoStoreWindow *self) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(page, PAGE_PAD);
    gtk_widget_set_margin_bottom(page, PAGE_PAD);
    gtk_widget_set_margin_start(page, PAGE_PAD);
    gtk_widget_set_margin_end(page, PAGE_PAD);

    GtkWidget *title = gtk_label_new("First Steps");
    GtkWidget *description = gtk_label_new("Start with the essentials. Each task can be opened whenever you need it.");
    gtk_widget_add_css_class(title, "page-header");
    gtk_widget_add_css_class(description, "page-desc");
    bind_ui_text(self, title, "First Steps", UI_LABEL);
    bind_ui_text(self, description, "Start with the essentials. Each task can be opened whenever you need it.", UI_LABEL);
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_widget_set_halign(description, GTK_ALIGN_START);
    gtk_label_set_wrap(GTK_LABEL(description), TRUE);
    gtk_box_append(GTK_BOX(page), title);
    gtk_box_append(GTK_BOX(page), description);

    GtkWidget *recommended = gtk_label_new("RECOMMENDED");
    gtk_widget_add_css_class(recommended, "category-section-title");
    bind_ui_text(self, recommended, "RECOMMENDED", UI_LABEL);
    gtk_widget_set_halign(recommended, GTK_ALIGN_START);
    gtk_widget_set_margin_top(recommended, 8);
    gtk_box_append(GTK_BOX(page), recommended);

    GtkWidget *steps = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_append(GTK_BOX(steps), create_tool_button(TOOL_ICON_UPDATE, "Update Manager", "Install available system updates.", "update", self));
    gtk_box_append(GTK_BOX(steps), create_tool_button(TOOL_ICON_REPOSITORY, "Software Sources", "Test and change the system repository mirror.", "mirror", self));
    gtk_box_append(GTK_BOX(steps), create_tool_button(TOOL_ICON_HARDWARE, "Driver Manager", "Set up graphics, Bluetooth, and printer support.", "drivers", self));
    gtk_box_append(GTK_BOX(page), steps);

    GtkWidget *optional = gtk_label_new("EXPLORE MORE");
    gtk_widget_add_css_class(optional, "category-section-title");
    bind_ui_text(self, optional, "EXPLORE MORE", UI_LABEL);
    gtk_widget_set_halign(optional, GTK_ALIGN_START);
    gtk_widget_set_margin_top(optional, 12);
    gtk_box_append(GTK_BOX(page), optional);
    GtkWidget *more = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_append(GTK_BOX(more), create_tool_button(TOOL_ICON_SOFTWARE, "Software Manager", "Browse and install applications by category.", "apps_home", self));
    gtk_box_append(GTK_BOX(more), create_tool_button(TOOL_ICON_SHIELD, "Firewall", "Install GUFW and enable UFW.", "security", self));
    gtk_box_append(GTK_BOX(page), more);

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), page);
    self->first_steps_page = scrolled;
    return scrolled;
}

static GtkWidget *build_update_page(NekoStoreWindow *self) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(page, PAGE_PAD);
    gtk_widget_set_margin_start(page, PAGE_PAD);
    gtk_widget_set_margin_end(page, PAGE_PAD);

    GtkWidget *title = gtk_label_new("Update Manager");
    GtkWidget *description = gtk_label_new("Refresh package data and install available updates from your configured Void Linux repositories.");
    gtk_widget_add_css_class(title, "page-header");
    gtk_widget_add_css_class(description, "page-desc");
    bind_ui_text(self, title, "Update Manager", UI_LABEL);
    bind_ui_text(self, description, "Refresh package data and install available updates from your configured Void Linux repositories.", UI_LABEL);
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_widget_set_halign(description, GTK_ALIGN_START);
    gtk_label_set_wrap(GTK_LABEL(description), TRUE);
    gtk_widget_set_margin_bottom(description, 8);

    GtkWidget *update_btn = gtk_button_new_with_label("Install available updates");
    bind_ui_text(self, update_btn, "Install available updates", UI_BUTTON);
    gtk_widget_add_css_class(update_btn, "suggested-action");
    gtk_widget_add_css_class(update_btn, "update-system-btn");
    gtk_widget_set_halign(update_btn, GTK_ALIGN_START);
    g_signal_connect(update_btn, "clicked", G_CALLBACK(on_update_system_clicked), self);

    gtk_box_append(GTK_BOX(page), title);
    gtk_box_append(GTK_BOX(page), description);
    gtk_box_append(GTK_BOX(page), update_btn);
    self->update_page = page;
    return page;
}

static GtkWidget *build_sidebar(NekoStoreWindow *self) {
    self->navigation_buttons = g_ptr_array_new();
    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_add_css_class(sidebar, "app-sidebar");
    gtk_widget_set_size_request(sidebar, 250, -1);
    gtk_widget_set_hexpand(sidebar, FALSE);
    gtk_widget_set_vexpand(sidebar, TRUE);
    gtk_widget_set_halign(sidebar, GTK_ALIGN_START);

    GtkWidget *brand_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_top(brand_row, 12);
    gtk_widget_set_margin_start(brand_row, 10);
    gtk_widget_set_margin_bottom(brand_row, 12);
    GtkWidget *brand = gtk_label_new("Neko Wizard");
    gtk_widget_add_css_class(brand, "sidebar-brand");
    gtk_widget_set_halign(brand, GTK_ALIGN_START);
    gtk_widget_set_valign(brand, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(brand_row), brand);
    gtk_box_append(GTK_BOX(sidebar), brand_row);

    GtkWidget *start_section = gtk_label_new("HOME");
    gtk_widget_add_css_class(start_section, "sidebar-section-title");
    bind_ui_text(self, start_section, "HOME", UI_LABEL);
    gtk_widget_set_halign(start_section, GTK_ALIGN_START);
    gtk_widget_set_margin_start(start_section, 10);
    gtk_widget_set_margin_bottom(start_section, 2);
    gtk_box_append(GTK_BOX(sidebar), start_section);

    GtkWidget *overview = create_sidebar_button(self, "Overview", "welcome");
    GtkWidget *first_steps = create_sidebar_button(self, "First Steps", "first_steps");
    gtk_box_append(GTK_BOX(sidebar), overview);
    gtk_box_append(GTK_BOX(sidebar), first_steps);

    GtkWidget *section = gtk_label_new("TOOLS");
    gtk_widget_add_css_class(section, "sidebar-section-title");
    bind_ui_text(self, section, "TOOLS", UI_LABEL);
    gtk_widget_set_halign(section, GTK_ALIGN_START);
    gtk_widget_set_margin_start(section, 10);
    gtk_widget_set_margin_top(section, 12);
    gtk_widget_set_margin_bottom(section, 2);
    gtk_box_append(GTK_BOX(sidebar), section);
    gtk_box_append(GTK_BOX(sidebar), create_sidebar_button(self, "Software Manager", "apps_home"));
    gtk_box_append(GTK_BOX(sidebar), create_sidebar_button(self, "Driver Manager", "drivers"));
    gtk_box_append(GTK_BOX(sidebar), create_sidebar_button(self, "Software Sources", "mirror"));
    gtk_box_append(GTK_BOX(sidebar), create_sidebar_button(self, "Update Manager", "update"));

    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(sidebar), spacer);

    self->about_btn = gtk_button_new_with_label("About");
    gtk_widget_add_css_class(self->about_btn, "about-btn");
    g_signal_connect(self->about_btn, "clicked", G_CALLBACK(on_about_clicked), self);
    gtk_box_append(GTK_BOX(sidebar), self->about_btn);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(overview), TRUE);
    self->sidebar = sidebar;
    return sidebar;
}

static void on_finished_back_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    self->installing = FALSE;
    const char *page_name = self->finished_return_page_name
                          ? self->finished_return_page_name : "first_steps";
    GtkWidget *page = gtk_stack_get_child_by_name(GTK_STACK(self->stack), page_name);
    if (!page) {
        page_name = "first_steps";
        page = gtk_stack_get_child_by_name(GTK_STACK(self->stack), page_name);
    }
    if (page)
        gtk_stack_set_visible_child(GTK_STACK(self->stack), page);
    sync_sidebar_selection(self, page_name);
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
    gtk_label_set_wrap(GTK_LABEL(self->status_label), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(self->status_label), 48);

    gtk_box_append(GTK_BOX(self->finished_page), self->finished_label);
    gtk_box_append(GTK_BOX(self->finished_page), self->progress_bar);
    gtk_box_append(GTK_BOX(self->finished_page), self->status_label);
    self->finished_back_btn = gtk_button_new_with_label("Back");
    bind_ui_text(self, self->finished_back_btn, "Back", UI_BUTTON);
    gtk_widget_add_css_class(self->finished_back_btn, "suggested-action");
    gtk_widget_set_halign(self->finished_back_btn, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(self->finished_back_btn, 20);
    gtk_widget_set_visible(self->finished_back_btn, FALSE);
    g_signal_connect(self->finished_back_btn, "clicked", G_CALLBACK(on_finished_back_clicked), self);
    gtk_box_append(GTK_BOX(self->finished_page), self->finished_back_btn);
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
                char *status = g_strdup_printf(ui_tr((Language)self->language, "✓ Reachable (HTTP %s, %.0fms)"), parts[1], connect_time * 1000);
                g_object_set_data(G_OBJECT(self->mirror_status_label), "ui-status-source", NULL);
                gtk_label_set_text(GTK_LABEL(self->mirror_status_label), status);
                g_free(status);
            }
            g_strfreev(parts);
        } else if (g_str_equal(line, "FAIL")) {
            set_ui_status(self, self->mirror_status_label, "✗ Unreachable");
        } else {
            char *trunc = g_strndup(line, 80);
            g_object_set_data(G_OBJECT(self->mirror_status_label), "ui-status-source", NULL);
            gtk_label_set_text(GTK_LABEL(self->mirror_status_label), trunc);
            g_free(trunc);
        }
    }
}

static void test_finished_cb(gboolean success, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    const char *text = gtk_label_get_text(GTK_LABEL(self->mirror_status_label));
    if (!success || !text || strlen(text) == 0 ||
        g_str_equal(text, ui_tr((Language)self->language, "Testing connectivity..."))) {
        set_ui_status(self, self->mirror_status_label, "✗ Connection failed");
    }
}

static void on_test_mirror_clicked(GtkButton *btn, gpointer user_data) {
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);
    MirrorInfo *mirror = g_object_get_data(G_OBJECT(btn), "mirror");
    if (!mirror) return;

    set_ui_status(self, self->mirror_status_label, "Testing connectivity...");

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
    set_ui_status(self, self->mirror_status_label,
                  success ? "System mirror applied successfully." : "Could not apply the mirror. Check permissions and try again.");
}

static void on_mirror_next_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; // Required by the clicked signal.
    NekoStoreWindow *self = NEKO_STORE_WINDOW(user_data);

    if (!self->selected_mirror) {
        set_ui_status(self, self->mirror_status_label, "Please select a mirror first.");
        return;
    }

    char *command = g_strdup_printf("pkexec xmirror --set %s", self->selected_mirror->url);
    set_ui_status(self, self->mirror_status_label, "Setting system mirror...");
    install_app_async(command, test_progress_cb, on_mirror_set_finished_cb, self);
    g_free(command);
}

static void build_mirror_page(NekoStoreWindow *self);

static void build_mirror_page(NekoStoreWindow *self) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget *header = gtk_label_new("Software Sources");
    gtk_widget_add_css_class(header, "page-header");
    bind_ui_text(self, header, "Software Sources", UI_LABEL);
    gtk_widget_set_halign(header, GTK_ALIGN_START);
    gtk_widget_set_margin_top(header, PAGE_PAD);
    gtk_widget_set_margin_start(header, PAGE_PAD);
    gtk_widget_set_margin_end(header, PAGE_PAD);
    gtk_box_append(GTK_BOX(vbox), header);

    GtkWidget *desc = gtk_label_new("Select a system mirror and optionally test its connectivity before applying it.");
    gtk_widget_add_css_class(desc, "page-desc");
    bind_ui_text(self, desc, "Select a system mirror and optionally test its connectivity before applying it.", UI_LABEL);
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
            const char *tier_text = current_tier == 1
                ? "Tier 1 Mirrors (maintained by Void Linux Team)"
                : "Tier 2 Mirrors (community mirrors, sync from Tier 1)";
            GtkWidget *tier_header = gtk_label_new(tier_text);
            bind_ui_text(self, tier_header, tier_text, UI_LABEL);
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
        gtk_accessible_update_property(GTK_ACCESSIBLE(radio),
                                       GTK_ACCESSIBLE_PROPERTY_LABEL, mirror->name, -1);
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
        bind_ui_text(self, test_btn, "Test", UI_BUTTON);
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
    set_ui_status(self, self->mirror_status_label,
                  "Select a mirror and click Test to check connectivity");
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

    GtkWidget *next_btn = gtk_button_new_with_label("Apply mirror");
    bind_ui_text(self, next_btn, "Apply mirror", UI_BUTTON);
    gtk_widget_add_css_class(next_btn, "suggested-action");
    gtk_widget_add_css_class(next_btn, "install-selected-btn");
    g_signal_connect(next_btn, "clicked", G_CALLBACK(on_mirror_next_clicked), self);
    gtk_widget_set_halign(next_btn, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(footer), next_btn);

    gtk_box_append(GTK_BOX(vbox), footer);

    self->mirror_page = vbox;
}

static void neko_store_window_init (NekoStoreWindow *self) {
    self->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(self->stack), GTK_STACK_TRANSITION_TYPE_NONE);
    self->ui_text_bindings = g_ptr_array_new_with_free_func(g_free);

    build_welcome_page(self);
    build_mirror_page(self);

    self->selection_count_labels = g_ptr_array_new();
    self->category_count_labels = g_ptr_array_new();
    self->gaming_page = create_app_group_page(self, 1, "Gaming", GROUP_GAMING, &self->gaming_flowbox, G_CALLBACK(on_install_selected_clicked), "Install selected", TRUE);
    self->drawing_image_page = create_app_group_page(self, 2, "Drawing and images", GROUP_DRAWING_IMAGE, &self->drawing_image_flowbox, G_CALLBACK(on_install_selected_clicked), "Install selected", TRUE);
    self->audio_video_page = create_app_group_page(self, 3, "Audio and video", GROUP_AUDIO_VIDEO, &self->audio_video_flowbox, G_CALLBACK(on_install_selected_clicked), "Install selected", TRUE);
    self->text_documents_page = create_app_group_page(self, 4, "Documents and writing", GROUP_TEXT_DOCUMENTS, &self->text_documents_flowbox, G_CALLBACK(on_install_selected_clicked), "Install selected", TRUE);
    self->social_page = create_app_group_page(self, 5, "Internet and communication", GROUP_SOCIAL, &self->social_flowbox, G_CALLBACK(on_install_selected_clicked), "Install selected", TRUE);
    self->drivers_page = create_app_group_page(self, 6, "Driver Manager", GROUP_DRIVERS, &self->drivers_flowbox, G_CALLBACK(on_install_selected_clicked), "Install selected drivers", FALSE);
    self->security_page = create_app_group_page(self, 7, "Firewall", GROUP_SECURITY, &self->security_flowbox, G_CALLBACK(on_install_selected_clicked), "Install and enable firewall", TRUE);
    build_finished_page(self);
    build_apps_home_page(self);
    build_first_steps_page(self);
    build_update_page(self);

    gtk_stack_add_named(GTK_STACK(self->stack), self->welcome_page, "welcome");
    gtk_stack_add_named(GTK_STACK(self->stack), self->first_steps_page, "first_steps");
    gtk_stack_add_named(GTK_STACK(self->stack), self->apps_home_page, "apps_home");
    gtk_stack_add_named(GTK_STACK(self->stack), self->mirror_page, "mirror");
    gtk_stack_add_named(GTK_STACK(self->stack), self->drivers_page, "drivers");
    gtk_stack_add_named(GTK_STACK(self->stack), self->finished_page, "finished");
    gtk_stack_add_named(GTK_STACK(self->stack), self->update_page, "update");

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *sidebar = build_sidebar(self);
    gtk_box_append(GTK_BOX(content), sidebar);
    gtk_widget_set_hexpand(self->stack, TRUE);
    gtk_widget_set_vexpand(self->stack, TRUE);
    gtk_box_append(GTK_BOX(content), self->stack);
    gtk_window_set_child(GTK_WINDOW(self), content);
}

static void neko_store_window_dispose(GObject *object) {
    NekoStoreWindow *self = (NekoStoreWindow *)object;
    if (self->apps_to_install) {
        g_list_free(self->apps_to_install);
        self->apps_to_install = NULL;
    }
    g_clear_pointer(&self->selection_count_labels, g_ptr_array_unref);
    g_clear_pointer(&self->category_count_labels, g_ptr_array_unref);
    g_clear_pointer(&self->ui_text_bindings, g_ptr_array_unref);
    g_clear_pointer(&self->finished_return_page_name, g_free);
    g_clear_object(&theme_monitor);
    g_clear_object(&theme_provider);
    G_OBJECT_CLASS(neko_store_window_parent_class)->dispose(object);
}

/* --- Live theme reload -----------------------------------------------------
 * GTK reads ~/.config/gtk-4.0/gtk.css exactly once, at startup, and never
 * watches it afterwards. Noctalia rewrites the colours it pulls in whenever the
 * system theme changes, so without this the app keeps whatever palette it
 * launched with until it is restarted.
 *
 * Re-loading the same file into our own provider keeps the desktop's semantic
 * colors available. The app stylesheet is installed one step above that so
 * decorative widget rules (not palette values) remain consistent in themed desktops.
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
    // Only the event type is needed; preserve the GFileMonitor signal signature.
    (void)monitor;
    (void)file;
    (void)other_file;
    (void)user_data;
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

static void neko_store_window_class_init (NekoStoreWindowClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = neko_store_window_dispose;
}

NekoStoreWindow *neko_store_window_new (GtkApplication *app) {
    GtkIconTheme *icon_theme = gtk_icon_theme_get_for_display(gdk_display_get_default());
    char *icon_path = get_resource_path("resources/icons");
    gtk_icon_theme_add_search_path(icon_theme, icon_path);
    g_free(icon_path);

    NekoStoreWindow *window = g_object_new (NEKO_STORE_TYPE_WINDOW, "application", app, NULL);

    gtk_window_set_title (GTK_WINDOW (window), "Neko Wizard");
    gtk_window_set_default_size (GTK_WINDOW (window), 1040, 720);

    GtkCssProvider *provider = gtk_css_provider_new();
    char *css_path = get_resource_path("data/style.css");
    GFile *css_file = g_file_new_for_path(css_path);
    gtk_css_provider_load_from_file(provider, css_file);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                               GTK_STYLE_PROVIDER(provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_USER + 2);
    g_object_unref(provider);
    g_object_unref(css_file);
    g_free(css_path);

    watch_user_theme();

    gtk_window_present (GTK_WINDOW (window));
    return window;
}
