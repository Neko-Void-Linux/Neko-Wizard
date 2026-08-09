#include "installer.h"
#include <gio/gio.h>

typedef struct {
    InstallProgressCallback progress_cb;
    InstallFinishedCallback finished_cb;
    gpointer user_data;
    GSubprocess *subprocess;
    GDataInputStream *stream;
    GCancellable *cancellable;
    gboolean wait_done;
    gboolean stream_done;
    gboolean exit_success;
} InstallTask;

/* Runs exactly once, after BOTH the process has exited and the stdout stream
 * has reached EOF (or the pending read was cancelled). Without this the stream
 * could be freed while on_line_read() is still queued, and finished_cb could
 * fire twice, which would advance the install queue twice and skip apps. */
static void maybe_finish(InstallTask *task) {
    if (!task->wait_done || !task->stream_done) return;

    if (task->finished_cb) {
        task->finished_cb(task->exit_success, task->user_data);
    }
    g_clear_object(&task->cancellable);
    g_clear_object(&task->stream);
    g_clear_object(&task->subprocess);
    g_free(task);
}

static void on_line_read(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    InstallTask *task = user_data;
    gsize length;
    GError *error = NULL;
    char *line = g_data_input_stream_read_line_finish(task->stream, res, &length, &error);

    if (line != NULL) {
        if (task->progress_cb) {
            task->progress_cb(line, task->user_data);
        }
        g_free(line);
        g_data_input_stream_read_line_async(task->stream, G_PRIORITY_DEFAULT,
                                            task->cancellable, on_line_read, task);
        return;
    }

    // EOF (or cancelled/error): no more lines will arrive.
    g_clear_error(&error);
    task->stream_done = TRUE;
    maybe_finish(task);
}

static void on_process_finished(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    InstallTask *task = user_data;
    GError *error = NULL;
    gboolean success = g_subprocess_wait_finish(G_SUBPROCESS(source_object), res, &error);

    if (error) {
        g_warning("Process error: %s", error->message);
        g_clear_error(&error);
        success = FALSE;
    } else {
        success = g_subprocess_get_if_exited(task->subprocess)
                  && (g_subprocess_get_exit_status(task->subprocess) == 0);
    }

    task->exit_success = success;
    task->wait_done = TRUE;
    // Stop any still-pending line read so stream_done can complete.
    g_cancellable_cancel(task->cancellable);
    maybe_finish(task);
}

void install_app_async(const char *command, InstallProgressCallback progress_cb, InstallFinishedCallback finished_cb, gpointer user_data) {
    InstallTask *task = g_new0(InstallTask, 1);
    task->progress_cb = progress_cb;
    task->finished_cb = finished_cb;
    task->user_data = user_data;
    task->cancellable = g_cancellable_new();

    GError *error = NULL;
    task->subprocess = g_subprocess_new(G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_MERGE,
                                        &error,
                                        "bash", "-c", command, NULL);

    if (error) {
        g_warning("Could not launch subprocess: %s", error->message);
        g_clear_error(&error);
        if (finished_cb) finished_cb(FALSE, user_data);
        g_clear_object(&task->cancellable);
        g_free(task);
        return;
    }

    GInputStream *stdout_stream = g_subprocess_get_stdout_pipe(task->subprocess);
    task->stream = g_data_input_stream_new(stdout_stream);

    g_data_input_stream_read_line_async(task->stream, G_PRIORITY_DEFAULT,
                                        task->cancellable, on_line_read, task);
    g_subprocess_wait_async(task->subprocess, task->cancellable, on_process_finished, task);
}
