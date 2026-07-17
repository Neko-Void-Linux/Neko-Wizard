#include "app_card.h"
#include "installer.h"

struct _NekoAppCard {
    GtkWidget parent_instance;
    AppInfo *info;
    GtkWidget *box;
    GtkWidget *icon_image;
    GtkWidget *name_label;
};

G_DEFINE_TYPE (NekoAppCard, neko_app_card, GTK_TYPE_WIDGET)

void neko_app_card_set_selected(NekoAppCard *self, gboolean selected) {
    self->info->selected = selected;
    if (selected) {
        gtk_widget_add_css_class(GTK_WIDGET(self), "selected");
    } else {
        gtk_widget_remove_css_class(GTK_WIDGET(self), "selected");
    }
}

gboolean neko_app_card_get_selected(NekoAppCard *self) {
    return self->info->selected;
}

static void on_card_click(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data) {
    NekoAppCard *self = NEKO_APP_CARD(user_data);
    neko_app_card_set_selected(self, !self->info->selected);
}

static void neko_app_card_dispose(GObject *object) {
    NekoAppCard *self = (NekoAppCard *)object;
    g_clear_pointer(&self->box, gtk_widget_unparent);
    G_OBJECT_CLASS(neko_app_card_parent_class)->dispose(object);
}

static void neko_app_card_class_init(NekoAppCardClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = neko_app_card_dispose;
    
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    gtk_widget_class_set_layout_manager_type(widget_class, GTK_TYPE_BIN_LAYOUT);
    gtk_widget_class_set_css_name(widget_class, "appcard");
}

static void neko_app_card_init(NekoAppCard *self) {
    self->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_valign(self->box, GTK_ALIGN_CENTER);
    gtk_widget_set_parent(self->box, GTK_WIDGET(self));

    // Small floor only: the flow box reflows the grid, so the card must be free
    // to shrink. Breathing room comes from the CSS padding, not from a fixed size.
    gtk_widget_set_size_request(GTK_WIDGET(self), 140, 150);

    self->icon_image = gtk_image_new();
    gtk_image_set_pixel_size(GTK_IMAGE(self->icon_image), 64);
    gtk_widget_set_halign(self->icon_image, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(self->icon_image, "card-icon");

    self->name_label = gtk_label_new("");
    gtk_widget_add_css_class(self->name_label, "card-title");
    gtk_label_set_wrap(GTK_LABEL(self->name_label), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(self->name_label), PANGO_WRAP_WORD_CHAR);
    // Cap at two lines so every card in a row ends up the same height.
    gtk_label_set_lines(GTK_LABEL(self->name_label), 2);
    gtk_label_set_ellipsize(GTK_LABEL(self->name_label), PANGO_ELLIPSIZE_END);
    gtk_label_set_max_width_chars(GTK_LABEL(self->name_label), 16);
    gtk_label_set_justify(GTK_LABEL(self->name_label), GTK_JUSTIFY_CENTER);
    gtk_widget_set_halign(self->name_label, GTK_ALIGN_CENTER);

    gtk_box_append(GTK_BOX(self->box), self->icon_image);
    gtk_box_append(GTK_BOX(self->box), self->name_label);

    GtkGesture *click_gesture = gtk_gesture_click_new();
    g_signal_connect(click_gesture, "pressed", G_CALLBACK(on_card_click), self);
    gtk_widget_add_controller(GTK_WIDGET(self), GTK_EVENT_CONTROLLER(click_gesture));
}

GtkWidget *neko_app_card_new(AppInfo *info) {
    NekoAppCard *card = g_object_new(NEKO_TYPE_APP_CARD, NULL);
    card->info = info;
    
    char *rel_path = g_build_filename("resources", info->icon_path, NULL);
    char *full_path = get_resource_path(rel_path);
    
    gtk_image_set_from_file(GTK_IMAGE(card->icon_image), full_path);
    
    g_free(rel_path);
    g_free(full_path);
    
    gtk_label_set_text(GTK_LABEL(card->name_label), info->name);
    
    return GTK_WIDGET(card);
}
