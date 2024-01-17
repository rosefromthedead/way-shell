#include "quick_settings.h"

#include <adwaita.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h>
#include <upower.h>

#include "../panel.h"
#include "quick_settings_audio_scales.h"
#include "quick_settings_grid/quick_settings_grid.h"
#include "quick_settings_header/quick_settings_header.h"
#include "quick_settings_mediator.h"

// global QuickSettingsMediator.
static QuickSettingsMediator *mediator = NULL;

// global QuickSettingsMediator subsystem.
static QuickSettings *qs = NULL;

struct _QuickSettings {
    GObject parent_instance;
    AdwWindow *win;
    AdwAnimation *animation;
    GtkBox *container;
    QuickSettingsHeader *header;
    QuickSettingsGrid *qs_grid;
    AudioScales audio_scales;
    AdwWindow *underlay;
    Panel *panel;
};
G_DEFINE_TYPE(QuickSettings, quick_settings, G_TYPE_OBJECT);

void static opacity_animation(double value, QuickSettings *qs) {
    gtk_widget_set_opacity(GTK_WIDGET(qs->win), value);
}

// stub out dispose, finalize, class_init and init methods.
static void quick_settings_dispose(GObject *gobject) {
    QuickSettings *self = QS_QUICK_SETTINGS(gobject);

    g_object_unref(self->header);
    g_object_unref(self->qs_grid);
    g_object_unref(self->underlay);

    // Chain-up
    G_OBJECT_CLASS(quick_settings_parent_class)->dispose(gobject);
};

static void quick_settings_finalize(GObject *gobject) {
    // Chain-up
    G_OBJECT_CLASS(quick_settings_parent_class)->finalize(gobject);
};

static void quick_settings_class_init(QuickSettingsClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = quick_settings_dispose;
    object_class->finalize = quick_settings_finalize;
};

static void animation_close_done(AdwAnimation *animation, QuickSettings *self) {
    g_debug("quick_settings.c:animation_close_done() called.");

    // disconnect done signal
    g_signal_handlers_disconnect_by_func(self->animation, animation_close_done,
                                         self);

    // make animation forward
    adw_timed_animation_set_reverse(ADW_TIMED_ANIMATION(self->animation),
                                    FALSE);

    // hide qs
    gtk_widget_set_visible(GTK_WIDGET(self->win), false);
    gtk_widget_set_visible(GTK_WIDGET(self->underlay), false);

    // emit hidden signal
    quick_settings_mediator_emit_hidden(mediator, self, self->panel);

    // clear panel
    self->panel = NULL;
};

void quick_settings_set_hidden(QuickSettings *self, Panel *panel) {
    int anim_state = 0;

    g_debug("quick_settings.c:quick_settings_set_hidden() called.");

    // this is required, since external callers will call this method as expect
    // a no-op of there is no attached self->panel.
    if (!self || !self->win || !self->panel || !panel) {
        return;
    }

    // this ensures there are no in-flight animation done callbacks on the
    // event-loop before starting a new animation, and avoids timing bugs.
    anim_state = adw_animation_get_state(self->animation);
    if (anim_state != ADW_ANIMATION_IDLE &&
        anim_state != ADW_ANIMATION_FINISHED) {
        return;
    }

    // reverse animation
    adw_timed_animation_set_reverse(ADW_TIMED_ANIMATION(self->animation), TRUE);

    // connect signal
    g_signal_connect(self->animation, "done", G_CALLBACK(animation_close_done),
                     self);

    // run animation
    adw_animation_play(self->animation);
}

static void on_underlay_click(GtkButton *button, QuickSettings *qs) {
    g_debug("quick_settings.c:on_click() called.");
    quick_settings_set_hidden(qs, qs->panel);
};

static void quick_settings_init_underlay(QuickSettings *self) {
    self->underlay = ADW_WINDOW(adw_window_new());
    gtk_widget_add_css_class(GTK_WIDGET(self->underlay), "underlay");
    gtk_layer_init_for_window(GTK_WINDOW(self->underlay));
    gtk_layer_set_layer((GTK_WINDOW(self->underlay)),
                        GTK_LAYER_SHELL_LAYER_TOP);

    gtk_layer_set_anchor(GTK_WINDOW(self->underlay), GTK_LAYER_SHELL_EDGE_TOP,
                         true);
    gtk_layer_set_anchor(GTK_WINDOW(self->underlay),
                         GTK_LAYER_SHELL_EDGE_BOTTOM, true);
    gtk_layer_set_anchor(GTK_WINDOW(self->underlay), GTK_LAYER_SHELL_EDGE_LEFT,
                         true);
    gtk_layer_set_anchor(GTK_WINDOW(self->underlay), GTK_LAYER_SHELL_EDGE_RIGHT,
                         true);

    GtkWidget *button = gtk_button_new();
    gtk_widget_set_hexpand(GTK_WIDGET(button), true);
    gtk_widget_set_vexpand(GTK_WIDGET(button), true);
    adw_window_set_content(self->underlay, GTK_WIDGET(button));

    g_signal_connect(button, "clicked", G_CALLBACK(on_underlay_click), self);
};

// fwd declare
static void on_window_destroy(GtkWindow *win, QuickSettings *self);

static void quick_settings_init_layout(QuickSettings *self) {
    self->panel = NULL;

    // init underlay
    quick_settings_init_underlay(self);

    // setup ADW window
    self->win = ADW_WINDOW(adw_window_new());
    g_signal_connect(self->win, "destroy", G_CALLBACK(on_window_destroy), self);
    gtk_widget_set_size_request(GTK_WIDGET(self->win), 400, 200);

    // configure layer shell properties of window.
    gtk_layer_init_for_window(GTK_WINDOW(self->win));
    gtk_layer_set_layer((GTK_WINDOW(self->win)), GTK_LAYER_SHELL_LAYER_OVERLAY);
    gtk_widget_set_name(GTK_WIDGET(self->win), "quick-settings");

    gtk_layer_set_anchor(GTK_WINDOW(self->win), GTK_LAYER_SHELL_EDGE_TOP, true);
    gtk_layer_set_anchor(GTK_WINDOW(self->win), GTK_LAYER_SHELL_EDGE_RIGHT,
                         true);
    gtk_layer_set_margin(GTK_WINDOW(self->win), GTK_LAYER_SHELL_EDGE_TOP, 10);
    gtk_layer_set_margin(GTK_WINDOW(self->win), GTK_LAYER_SHELL_EDGE_RIGHT, 30);

    // create vertical container box
    self->container = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));

    // attach header
    gtk_box_append(self->container,
                   quick_settings_header_get_widget(self->header));

    // attach audio scales
    quick_settings_audio_scales_init(&self->audio_scales);
    gtk_box_append(self->container, GTK_WIDGET(self->audio_scales.container));

    // attach quick settings grid
    gtk_box_append(self->container,
                   quick_settings_grid_get_widget(self->qs_grid));

    // animation controller
    AdwAnimationTarget *target = adw_callback_animation_target_new(
        (AdwAnimationTargetFunc)opacity_animation, self, NULL);
    self->animation =
        adw_timed_animation_new(GTK_WIDGET(self->win), 0, 1, 250, target);

    adw_window_set_content(self->win, GTK_WIDGET(self->container));
}

static void on_window_destroy(GtkWindow *win, QuickSettings *self) {
    g_debug("quick_settings.c:on_window_destroy() called.");
    quick_settings_reinitialize(self);
};

void quick_settings_reinitialize(QuickSettings *self) {
    g_debug("quick_settings.c:quick_settings_reinitialize() called.");

    // kill our signals
    g_signal_handlers_disconnect_by_func(self->win, on_window_destroy, self);

    // reset mediator's pointer to us
    quick_settings_mediator_set_qs(mediator, self);

    // reinitialize dependent widgets
    quick_settings_grid_reinitialize(self->qs_grid);
    quick_settings_header_reinitialize(self->header);

    // reinit our layout
    quick_settings_init_layout(self);
}

static void quick_settings_init(QuickSettings *self) {
    // initialize depedent widgets
    self->header = g_object_new(QUICK_SETTINGS_HEADER_TYPE, NULL);
    self->qs_grid = g_object_new(QUICK_SETTINGS_GRID_TYPE, NULL);

    quick_settings_init_layout(self);
};

static void animation_open_done(AdwAnimation *animation, QuickSettings *qs) {
    g_debug("quick_settings.c:animation_open_done() called.");

    // disconnect done signal
    g_signal_handlers_disconnect_by_func(qs->animation, animation_open_done,
                                         qs);
    // emit visible signal
    quick_settings_mediator_emit_visible(mediator, qs, qs->panel);
};

void quick_settings_set_visible(QuickSettings *self, Panel *panel) {
    GdkMonitor *monitor = NULL;
    int anim_state = 0;

    g_debug("quick_settings.c:quick_settings_set_visible() called.");

    if (!self || !panel) return;

    // determine current state
    if (self->panel && self->panel == panel) {
        // panel is already opened on the requested panel, just return.
        return;
    }
    if (self->panel && self->panel != panel) {
        // panel is opened on another monitor, close it first
        quick_settings_set_hidden(self, self->panel);
    }

    // this ensures there are no in-flight animation done callbacks on the
    // event-loop before starting a new animation, and avoids timing bugs.
    anim_state = adw_animation_get_state(self->animation);
    g_debug("message_tray.c:message_tray_set_visible() anim_state: %d",
            anim_state);
    if (anim_state != ADW_ANIMATION_IDLE &&
        anim_state != ADW_ANIMATION_FINISHED) {
        return;
    }

    monitor = panel_get_monitor(panel);
    if (!monitor) return;

    // move underlay and win to new monitor
    gtk_layer_set_monitor(GTK_WINDOW(self->underlay), monitor);
    gtk_layer_set_monitor(GTK_WINDOW(self->win), monitor);

    // emit will show signal before we open any windows.
    quick_settings_mediator_emit_will_show(mediator, self, panel);

    // show underlay
    // gtk_window_present(GTK_WINDOW(self->underlay));

    // present the window
    gtk_window_present(GTK_WINDOW(self->win));

    // ref and store pointer to panel we attached to.
    self->panel = panel;

    // connect animation done signal and play animation
    g_signal_connect(self->animation, "done", G_CALLBACK(animation_open_done),
                     self);
    adw_animation_play(self->animation);
}

void quick_settings_toggle(QuickSettings *self, Panel *panel) {
    g_debug("quick_settings.c:quick_settings_toggle() called.");

    if (!self || !panel) return;

    if (!self->panel) {
        return quick_settings_set_visible(self, panel);
    }
    // qs is open on same panel requesting, hide it.
    if (self->panel == panel) {
        return quick_settings_set_hidden(self, panel);
    }
    // qs is displayed on another monitor, hide it first.
    if (self->panel != panel) {
        quick_settings_set_hidden(self, self->panel);
    }
    return quick_settings_set_visible(self, panel);
}

QuickSettingsMediator *quick_settings_get_global_mediator() {
    return mediator;
};

void quick_settings_activate(AdwApplication *app, gpointer user_data) {
    qs = g_object_new(QUICK_SETTINGS_TYPE, NULL);
    mediator = g_object_new(QUICK_SETTINGS_MEDIATOR_TYPE, NULL);
    quick_settings_mediator_set_qs(mediator, qs);
};

Panel *quick_settings_get_panel(QuickSettings *qs) { return qs->panel; };

void quick_settings_shrink(QuickSettings *qs) {
    g_debug("quick_settings.c:quick_settings_shrink() called.");
    gtk_window_set_default_size(GTK_WINDOW(qs->win), 400, 200);
};
