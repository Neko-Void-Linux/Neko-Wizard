#include "privileged.h"
#include <gio/gio.h>
#include <glib/gstdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

/* A single privileged session lives for the whole app run. It is a root bash
 * reading base64-encoded commands from a FIFO and writing each command's output
 * to a per-request file, so a user-level `pkexec` shim can reach it. */
static gboolean started = FALSE;
static char *priv_dir = NULL;
static int keepalive_fd = -1;

/* Runs as root. Reads "<id> <base64-command>" lines and executes each one. The
 * background `cat` exits the daemon the moment the app closes the keepalive
 * pipe (normal quit or crash), so no root process is ever left behind. */
static const char *DAEMON_SCRIPT =
    "R=\"$1\"\n"
    "[ -p \"$R/in\" ] || exit 1\n"
    "umask 022\n"
    ": > \"$R/ready\"\n"
    "( cat \"$R/keepalive\" >/dev/null 2>&1; kill -TERM $$ 2>/dev/null ) &\n"
    "while true; do\n"
    "    if IFS= read -r line < \"$R/in\"; then\n"
    "        id=\"${line%% *}\"\n"
    "        b64=\"${line#* }\"\n"
    "        [ -z \"$id\" ] && continue\n"
    "        cmd=$(printf '%s' \"$b64\" | base64 -d 2>/dev/null)\n"
    "        out=\"$R/out.$id\"\n"
    "        rc=\"$R/rc.$id\"\n"
    "        rm -f \"$rc\"\n"
    "        bash -c \"$cmd\" > \"$out\" 2>&1\n"
    "        printf '%s\\n' \"$?\" > \"$rc\"\n"
    "    fi\n"
    "done\n";

/* Runs as the user, in place of pkexec. Forwards to the persistent session when
 * it is ready, otherwise falls back to the real pkexec (one-off prompt). */
static char *build_shim(const char *real_pkexec) {
    return g_strdup_printf(
        "#!/usr/bin/env bash\n"
        "R=\"${NEKO_PRIV_DIR:-}\"\n"
        "if [ -z \"$R\" ] || [ ! -p \"$R/in\" ] || [ ! -f \"$R/ready\" ]; then\n"
        "    exec %s \"$@\"\n"
        "fi\n"
        "id=\"$$.$RANDOM\"\n"
        "cmd=$(printf '%%q ' \"$@\")\n"
        "b64=$(printf '%%s' \"$cmd\" | base64 -w0)\n"
        "rm -f \"$R/rc.$id\" \"$R/out.$id\"\n"
        "printf '%%s %%s\\n' \"$id\" \"$b64\" > \"$R/in\"\n"
        "while [ ! -f \"$R/rc.$id\" ]; do\n"
        "    sleep 0.1\n"
        "done\n"
        "cat \"$R/out.$id\" 2>/dev/null\n"
        "rc=$(cat \"$R/rc.$id\" 2>/dev/null)\n"
        "rm -f \"$R/out.$id\" \"$R/rc.$id\"\n"
        "exit \"${rc:-1}\"\n",
        real_pkexec);
}

static void on_daemon_exited(GObject *source, GAsyncResult *res, gpointer user_data) {
    (void)user_data;
    GError *error = NULL;
    g_subprocess_wait_finish(G_SUBPROCESS(source), res, &error);
    if (error != NULL) {
        g_warning("Privileged session ended: %s", error->message);
        g_clear_error(&error);
    }
}

static gboolean write_script(const char *path, const char *contents) {
    GError *error = NULL;
    if (!g_file_set_contents(path, contents, -1, &error)) {
        g_warning("Could not write %s: %s", path, error->message);
        g_clear_error(&error);
        return FALSE;
    }
    chmod(path, 0700);
    return TRUE;
}

void privileged_start_async(void) {
    if (started) return;
    started = TRUE;

    const char *runtime = g_get_user_runtime_dir();
    if (runtime == NULL || *runtime == '\0') return;

    priv_dir = g_build_filename(runtime, "neko-priv", NULL);
    if (g_mkdir_with_parents(priv_dir, 0700) != 0) {
        g_warning("Could not create privileged session dir %s", priv_dir);
        return;
    }
    chmod(priv_dir, 0700);

    char *fifo = g_build_filename(priv_dir, "in", NULL);
    unlink(fifo);
    if (mkfifo(fifo, 0600) != 0 && errno != EEXIST) {
        g_warning("Could not create command FIFO %s: %s", fifo, g_strerror(errno));
        g_free(fifo);
        return;
    }
    g_free(fifo);

    char *keepalive = g_build_filename(priv_dir, "keepalive", NULL);
    unlink(keepalive);
    if (mkfifo(keepalive, 0600) != 0 && errno != EEXIST) {
        g_warning("Could not create keepalive FIFO %s: %s", keepalive, g_strerror(errno));
        g_free(keepalive);
        return;
    }
    // Held open for the app's lifetime; the daemon exits when this closes.
    keepalive_fd = open(keepalive, O_RDWR | O_CLOEXEC);
    g_free(keepalive);
    if (keepalive_fd < 0) {
        g_warning("Could not open keepalive FIFO: %s", g_strerror(errno));
        return;
    }

    char *real_pkexec = g_find_program_in_path("pkexec");
    if (real_pkexec == NULL) {
        g_warning("pkexec not found; privileged session disabled");
        return;
    }

    char *daemon_path = g_build_filename(priv_dir, "daemon.sh", NULL);
    char *shim_path = g_build_filename(priv_dir, "pkexec", NULL);
    char *shim = build_shim(real_pkexec);

    gboolean ok = write_script(daemon_path, DAEMON_SCRIPT) &&
                  write_script(shim_path, shim);
    g_free(shim);
    if (!ok) {
        g_free(daemon_path);
        g_free(shim_path);
        g_free(real_pkexec);
        return;
    }

    // One polkit prompt: launch the persistent root shell. Children resolve
    // `pkexec` to our shim afterwards, so no further prompt appears.
    GError *error = NULL;
    GSubprocess *proc = g_subprocess_new(
        G_SUBPROCESS_FLAGS_STDOUT_SILENCE | G_SUBPROCESS_FLAGS_STDERR_SILENCE,
        &error, real_pkexec, "bash", daemon_path, priv_dir, NULL);
    if (error != NULL) {
        g_warning("Could not start privileged session: %s", error->message);
        g_clear_error(&error);
    } else {
        g_subprocess_wait_async(proc, NULL, on_daemon_exited, NULL);
        g_object_unref(proc);
    }

    const char *old_path = g_getenv("PATH");
    char *new_path = g_strdup_printf("%s:%s", priv_dir,
                                     (old_path && *old_path) ? old_path : "/usr/bin:/bin");
    g_setenv("PATH", new_path, TRUE);
    g_free(new_path);
    g_setenv("NEKO_PRIV_DIR", priv_dir, TRUE);

    g_free(daemon_path);
    g_free(shim_path);
    g_free(real_pkexec);
}

gboolean privileged_is_ready(void) {
    if (priv_dir == NULL) return FALSE;
    char *ready = g_build_filename(priv_dir, "ready", NULL);
    gboolean ok = g_file_test(ready, G_FILE_TEST_EXISTS);
    g_free(ready);
    return ok;
}

void privileged_shutdown(void) {
    // Closing the keepalive pipe makes the daemon's watcher exit it. The OS
    // closes this fd on process exit too, so the daemon never outlives the app.
    if (keepalive_fd >= 0) {
        close(keepalive_fd);
        keepalive_fd = -1;
    }
}
