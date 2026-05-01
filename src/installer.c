#include "installer.h"
#include <gio/gio.h>
#include <stdio.h>

/* Log file written alongside the install session for post-mortem debugging. */
static FILE *log_file = NULL;

static void
log_open (void)
{
    if (!log_file)
        log_file = fopen ("/tmp/neko-void-install.log", "a");
}

static void
log_line (const char *line)
{
    if (log_file) {
        fputs (line, log_file);
        fputc ('\n', log_file);
        fflush (log_file);
    }
}

/* ── Reference-counted install task ─────────────────── */

typedef struct {
    gint                    ref_count;
    InstallProgressCallback progress_cb;
    InstallFinishedCallback finished_cb;
    gpointer                user_data;
    GSubprocess            *subprocess;
    GDataInputStream       *stream;
} InstallTask;

static InstallTask *
install_task_ref (InstallTask *task)
{
    g_atomic_int_inc (&task->ref_count);
    return task;
}

static void
install_task_unref (InstallTask *task)
{
    if (g_atomic_int_dec_and_test (&task->ref_count)) {
        g_clear_object (&task->stream);
        g_clear_object (&task->subprocess);
        g_free (task);
    }
}

/* ── Async callbacks ─────────────────────────────────── */

static void
on_line_read (GObject *source, GAsyncResult *res, gpointer user_data)
{
    (void) source;
    InstallTask *task  = user_data;
    GError      *error = NULL;
    gsize        length;

    char *line = g_data_input_stream_read_line_finish (
        task->stream, res, &length, &error);

    if (line != NULL) {
        log_line (line);
        if (task->progress_cb)
            task->progress_cb (line, task->user_data);
        g_free (line);
        g_data_input_stream_read_line_async (task->stream, G_PRIORITY_DEFAULT,
                                             NULL, on_line_read, task);
    } else {
        g_clear_error (&error);
        install_task_unref (task);
    }
}

static void
on_process_finished (GObject *source, GAsyncResult *res, gpointer user_data)
{
    InstallTask *task    = user_data;
    GError      *error   = NULL;
    gboolean     success = FALSE;

    g_subprocess_wait_finish (G_SUBPROCESS (source), res, &error);

    if (error) {
        log_line (error->message);
        g_warning ("Subprocess wait error: %s", error->message);
        g_clear_error (&error);
    } else {
        success = g_subprocess_get_if_exited (task->subprocess)
               && (g_subprocess_get_exit_status (task->subprocess) == 0);
    }

    char status_msg[64];
    g_snprintf (status_msg, sizeof (status_msg),
                "[exit: %s]", success ? "OK" : "FAIL");
    log_line (status_msg);

    if (task->finished_cb)
        task->finished_cb (success, task->user_data);

    install_task_unref (task);
}

/* ── Public API ──────────────────────────────────────── */

void
install_app_async (const char              *command,
                   InstallProgressCallback  progress_cb,
                   InstallFinishedCallback  finished_cb,
                   gpointer                 user_data)
{
    log_open ();
    log_line ("---");
    log_line (command);

    GError      *error = NULL;
    InstallTask *task  = g_new0 (InstallTask, 1);

    task->ref_count   = 1;
    task->progress_cb = progress_cb;
    task->finished_cb = finished_cb;
    task->user_data   = user_data;

    /*
     * STDIN_DEVNULL: prevents interactive tools (xbps-install, flatpak)
     * from blocking waiting for keyboard input since there is no tty.
     */
    task->subprocess = g_subprocess_new (
        G_SUBPROCESS_FLAGS_STDIN_INHERIT |
        G_SUBPROCESS_FLAGS_STDOUT_PIPE   |
        G_SUBPROCESS_FLAGS_STDERR_MERGE,
        &error,
        "bash", "-c", command, NULL);

    if (error) {
        log_line (error->message);
        g_warning ("Could not launch subprocess: %s", error->message);
        g_clear_error (&error);
        if (finished_cb)
            finished_cb (FALSE, user_data);
        install_task_unref (task);
        return;
    }

    GInputStream *stdout_stream = g_subprocess_get_stdout_pipe (task->subprocess);
    task->stream = g_data_input_stream_new (stdout_stream);

    install_task_ref (task);
    g_data_input_stream_read_line_async (task->stream, G_PRIORITY_DEFAULT,
                                         NULL, on_line_read, task);

    install_task_ref (task);
    g_subprocess_wait_async (task->subprocess, NULL, on_process_finished, task);

    install_task_unref (task);
}
