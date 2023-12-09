#pragma once

#include <adwaita.h>

typedef struct _Panel Panel;
typedef struct _MessageTray MessageTray;

G_BEGIN_DECLS

struct _MessageTrayMediator;
#define MESSAGE_TRAY_MEDIATOR_TYPE message_tray_mediator_get_type()
G_DECLARE_FINAL_TYPE(MessageTrayMediator, message_tray_mediator, MESSAGE_TRAY,
                     MEDIATOR, GObject);

G_END_DECLS

// Sets a pointer to the Tray this MessageTrayMediator mediates signals for.
void message_tray_mediator_set_tray(MessageTrayMediator *mediator,
                                    MessageTray *tray);

// Emitted when the MessageTray is about to become visible.
// Includes the Panel which invoked its visibility.
void message_tray_mediator_emit_will_show(MessageTrayMediator *mediator,
                                          MessageTray *tray, Panel *panel);

// Emitted when the MessageTray has become visible.
// Includes the Panel which invoked its visibility.
void message_tray_mediator_emit_visible(MessageTrayMediator *mediator,
                                        MessageTray *tray, Panel *panel);

// Emitted when the MessageTray is hidden.
// And the last Panel which invoked it.
void message_tray_mediator_emit_hidden(MessageTrayMediator *mediator,
                                       MessageTray *tray, Panel *panel);

// Connects the MessageTrayMediator to all other Mediator's signals required.
void message_tray_mediator_connect(MessageTrayMediator *mediator);
