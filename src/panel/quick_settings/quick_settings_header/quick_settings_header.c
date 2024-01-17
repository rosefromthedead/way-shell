#include "quick_settings_header.h"

#include <adwaita.h>

#include "../quick_settings.h"
#include "../quick_settings_mediator.h"
#include "gtk/gtkrevealer.h"
#include "quick_settings_battery_button.h"
#include "quick_settings_power_menu.h"

enum signals { signals_n };

typedef struct _QuickSettingsHeader {
    GObject parent_instance;
    GtkBox *container;
    GtkCenterBox *center_box;
    QuickSettingsBatteryButton *battery_button;
    QuickSettingsPowerMenu *power_menu;
    GtkButton *power_button;
    GtkRevealer *revealer;
} QuickSettingsHeader;
G_DEFINE_TYPE(QuickSettingsHeader, quick_settings_header, G_TYPE_OBJECT);

// stub out empty dispose, finalize, class_init, and init methods for this
// GObject.
static void quick_settings_header_dispose(GObject *gobject) {
    g_debug("quick_settings_header.c:quick_settings_header_dispose() called.");

    // Chain-up
    G_OBJECT_CLASS(quick_settings_header_parent_class)->dispose(gobject);
};

static void quick_settings_header_finalize(GObject *gobject) {
    // Chain-up
    G_OBJECT_CLASS(quick_settings_header_parent_class)->finalize(gobject);
};

static void quick_settings_header_class_init(QuickSettingsHeaderClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = quick_settings_header_dispose;
    object_class->finalize = quick_settings_header_finalize;
};

static void on_reveal_finish(GtkRevealer *revealer, GParamSpec *spec,
                             QuickSettingsHeader *self) {
    gboolean revealed =
        gtk_revealer_get_reveal_child(GTK_REVEALER(self->revealer));
    if (!revealed) {
        QuickSettingsMediator *mediator = quick_settings_get_global_mediator();
        quick_settings_mediator_req_shrink(mediator);
    }
}

static void on_power_button_click(GtkButton *button,
                                  QuickSettingsHeader *self) {
    g_debug("quick_settings.c:on_power_button_click() called.");
    gboolean revealed =
        gtk_revealer_get_reveal_child(GTK_REVEALER(self->revealer));
    gtk_revealer_set_reveal_child(self->revealer, !revealed);
};

static void quick_settings_header_init_layout(QuickSettingsHeader *self) {
    // create main layouts
    self->container = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_name(GTK_WIDGET(self->container),
                        "quick-settings-header-container");
    self->center_box = GTK_CENTER_BOX(gtk_center_box_new());
    gtk_box_append(self->container, GTK_WIDGET(self->center_box));
    gtk_widget_set_name(GTK_WIDGET(self->center_box), "quick-settings-header");

    // create GtkRevealer to reveal power menu
    self->revealer = GTK_REVEALER(gtk_revealer_new());
    gtk_revealer_set_transition_type(self->revealer,
                                     GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
    gtk_revealer_set_transition_duration(self->revealer, 250);
    gtk_revealer_set_reveal_child(self->revealer, FALSE);
    gtk_box_append(self->container, GTK_WIDGET(self->revealer));
    // wire into child-revealed GObject notify signal to shrink QuickSettings
    // when unrevealed.
    g_signal_connect(self->revealer, "notify::child-revealed",
                     G_CALLBACK(on_reveal_finish), self);

    // attach power menu to GtkRevealer
    gtk_revealer_set_child(
        self->revealer, quick_settings_power_menu_get_widget(self->power_menu));

    // create power button
    self->power_button =
        GTK_BUTTON(gtk_button_new_from_icon_name("system-shutdown-symbolic"));
    gtk_widget_add_css_class(GTK_WIDGET(self->power_button), "circular");
    g_signal_connect(self->power_button, "clicked",
                     G_CALLBACK(on_power_button_click), self);

    gtk_center_box_set_start_widget(
        self->center_box,
        quick_settings_battery_button_get_widget(self->battery_button));

    gtk_center_box_set_end_widget(self->center_box,
                                  GTK_WIDGET(self->power_button));
}

static void quick_settings_header_init(QuickSettingsHeader *self) {
    // load up children depedencies.
    self->battery_button =
        g_object_new(QUICK_SETTINGS_BATTERY_BUTTON_TYPE, NULL);
    self->power_menu = g_object_new(QUICK_SETTINGS_POWER_MENU_TYPE, NULL);

    // initialize layout
    quick_settings_header_init_layout(self);
}

void quick_settings_header_reinitialize(QuickSettingsHeader *self) {
    // destroy our signals
    g_signal_handlers_disconnect_by_func(
        self->power_button, G_CALLBACK(on_power_button_click), self);

    // call reinitialize on children widget
    quick_settings_battery_button_reinitialize(self->battery_button);
    quick_settings_power_menu_reinitialize(self->power_menu);

    // call layout reinit for ourselves
    quick_settings_header_init_layout(self);
}

GtkWidget *quick_settings_header_get_widget(QuickSettingsHeader *self) {
    return GTK_WIDGET(self->container);
}
