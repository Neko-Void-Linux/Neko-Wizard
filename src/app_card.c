#include "app_card.h"
#include "installer.h"

static void on_card_toggled(GtkToggleButton *button, gpointer user_data) {
    AppInfo *info = user_data;
    gboolean selected = gtk_toggle_button_get_active(button);
    info->selected = selected;
    GtkWidget *indicator = g_object_get_data(G_OBJECT(button), "selection-indicator");
    if (indicator)
        gtk_widget_set_visible(indicator, selected);
}

void neko_app_card_set_selected(GtkWidget *card, gboolean selected) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(card), selected);
}

gboolean neko_app_card_get_selected(GtkWidget *card) {
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(card));
}

GtkWidget *neko_app_card_new(AppInfo *info) {
    GtkWidget *card = gtk_toggle_button_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *overlay = gtk_overlay_new();
    GtkWidget *icon_image = gtk_image_new();
    GtkWidget *name_label = gtk_label_new(info->name);
    GtkWidget *selection_indicator = gtk_label_new("✓");

    gtk_widget_add_css_class(card, "app-card");
    gtk_widget_set_size_request(card, 204, 72);
    gtk_widget_set_hexpand(box, TRUE);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);

    gtk_image_set_pixel_size(GTK_IMAGE(icon_image), 40);
    gtk_widget_set_size_request(icon_image, 40, 40);
    gtk_widget_set_halign(icon_image, GTK_ALIGN_START);
    gtk_widget_add_css_class(icon_image, "card-icon");

    gtk_widget_add_css_class(name_label, "card-title");
    gtk_label_set_wrap(GTK_LABEL(name_label), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(name_label), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_max_width_chars(GTK_LABEL(name_label), 28);
    gtk_label_set_xalign(GTK_LABEL(name_label), 0.0);
    gtk_widget_set_halign(name_label, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(name_label, TRUE);

    gtk_box_append(GTK_BOX(box), icon_image);
    gtk_box_append(GTK_BOX(box), name_label);
    gtk_overlay_set_child(GTK_OVERLAY(overlay), box);
    gtk_widget_add_css_class(selection_indicator, "card-check");
    gtk_widget_set_halign(selection_indicator, GTK_ALIGN_END);
    gtk_widget_set_valign(selection_indicator, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_end(selection_indicator, 8);
    gtk_widget_set_visible(selection_indicator, info->selected);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), selection_indicator);
    gtk_button_set_child(GTK_BUTTON(card), overlay);
    g_object_set_data(G_OBJECT(card), "selection-indicator", selection_indicator);
    gtk_accessible_update_property(GTK_ACCESSIBLE(card),
                                   GTK_ACCESSIBLE_PROPERTY_LABEL, info->name,
                                   -1);
    g_object_set_data(G_OBJECT(card), "app-info", info);
    g_signal_connect(card, "toggled", G_CALLBACK(on_card_toggled), info);
    
    char *rel_path = g_build_filename("resources", info->icon_path, NULL);
    char *full_path = get_resource_path(rel_path);
    
    gtk_image_set_from_file(GTK_IMAGE(icon_image), full_path);
    
    g_free(rel_path);
    g_free(full_path);
    
    neko_app_card_set_selected(card, info->selected);
    
    return GTK_WIDGET(card);
}
