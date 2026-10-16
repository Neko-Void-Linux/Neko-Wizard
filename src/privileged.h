#ifndef PRIVILEGED_H
#define PRIVILEGED_H

#include <glib.h>

/* Starts a single, long-lived privileged session once per app run.
 *
 * The session is a root shell obtained through one `pkexec` prompt. Every child
 * process the app spawns has its PATH rewritten so the `pkexec` name resolves to
 * a shim that forwards the command to that persistent root shell instead of
 * opening a new polkit prompt. This is what lets an install run ask for the
 * password only once. */
void privileged_start_async(void);

/* TRUE once the persistent root shell has authenticated and signalled ready. */
gboolean privileged_is_ready(void);

/* Ends the privileged session (the root daemon exits and no prompt remains).
 * Safe to call more than once. Also happens automatically when the process
 * exits, because the keepalive pipe closes with it. */
void privileged_shutdown(void);

#endif
