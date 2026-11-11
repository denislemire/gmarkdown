/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026 Denis Lemire */

#include "gmarkdown.h"
#include "markdown.h"
#include "window.h"
#include "config.h"

#include <adwaita.h>
#include <webkit/webkit.h>
#include <unistd.h>

static gboolean
path_is_safe(const char *path)
{
	if (path == NULL || path[0] != '/')
		return (FALSE);
	if (strstr(path, "..") != NULL)
		return (FALSE);
	return (TRUE);
}

static const char *
mime_for_path(const char *path)
{
	if (g_str_has_suffix(path, ".css"))
		return ("text/css");
	if (g_str_has_suffix(path, ".js"))
		return ("application/javascript");
	if (g_str_has_suffix(path, ".svg"))
		return ("image/svg+xml");
	return ("application/octet-stream");
}

static GBytes *
load_bundle_bytes(const char *path)
{
	g_autofree char *res = NULL;
	g_autofree char *dev = NULL;
	g_autofree char *inst = NULL;
	const char *rel;
	GBytes *bytes;
	gchar *contents;
	gsize len;

	rel = (path[0] == '/') ? path + 1 : path;
	res = g_strconcat(GMARKDOWN_RESOURCE_PATH, "/", rel, NULL);
	bytes = g_resources_lookup_data(res, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
	if (bytes != NULL)
		return (bytes);
	dev = g_build_filename(GMARKDOWN_DEV_DATADIR, rel, NULL);
	inst = g_build_filename(GMARKDOWN_PKGDATADIR, rel, NULL);
	if (g_file_get_contents(dev, &contents, &len, NULL) ||
	    g_file_get_contents(inst, &contents, &len, NULL))
		return (g_bytes_new_take(contents, len));
	return (NULL);
}

static void
scheme_request(WebKitURISchemeRequest *request, gpointer data)
{
	const char *path;
	g_autoptr(GBytes) bytes = NULL;
	g_autoptr(GInputStream) stream = NULL;
	g_autoptr(GError) error = NULL;

	(void)data;
	path = webkit_uri_scheme_request_get_path(request);
	if (!path_is_safe(path)) {
		error = g_error_new(G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME,
		    "invalid path");
		webkit_uri_scheme_request_finish_error(request, error);
		return;
	}
	bytes = load_bundle_bytes(path);
	if (bytes == NULL) {
		error = g_error_new(G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
		    "missing %s", path);
		webkit_uri_scheme_request_finish_error(request, error);
		return;
	}
	stream = g_memory_input_stream_new_from_bytes(bytes);
	webkit_uri_scheme_request_finish(request, stream,
	    (gint64)g_bytes_get_size(bytes), mime_for_path(path));
}

static void
register_bundle_scheme(void)
{
	WebKitWebContext *ctx = webkit_web_context_get_default();
	WebKitSecurityManager *sec = webkit_web_context_get_security_manager(ctx);

	webkit_web_context_register_uri_scheme(ctx, GMARKDOWN_SCHEME,
	    scheme_request, NULL, NULL);
	webkit_security_manager_register_uri_scheme_as_local(sec, GMARKDOWN_SCHEME);
	webkit_security_manager_register_uri_scheme_as_secure(sec, GMARKDOWN_SCHEME);
}

static void
set_accels(GtkApplication *app)
{
	gtk_application_set_accels_for_action(app, "win.open",
	    (const char *[]){ "<Ctrl>o", NULL });
	gtk_application_set_accels_for_action(app, "win.print",
	    (const char *[]){ "<Ctrl>p", NULL });
	gtk_application_set_accels_for_action(app, "win.export-pdf",
	    (const char *[]){ "<Ctrl><Shift>p", NULL });
	gtk_application_set_accels_for_action(app, "win.find",
	    (const char *[]){ "<Ctrl>f", NULL });
	gtk_application_set_accels_for_action(app, "win.toc",
	    (const char *[]){ "F9", NULL });
	gtk_application_set_accels_for_action(app, "win.zoom-in",
	    (const char *[]){ "<Ctrl>plus", "<Ctrl>equal", NULL });
	gtk_application_set_accels_for_action(app, "win.zoom-out",
	    (const char *[]){ "<Ctrl>minus", NULL });
	gtk_application_set_accels_for_action(app, "win.zoom-reset",
	    (const char *[]){ "<Ctrl>0", NULL });
	gtk_application_set_accels_for_action(app, "app.quit",
	    (const char *[]){ "<Ctrl>q", NULL });
}

static void
quit_app(GSimpleAction *action, GVariant *param, gpointer data)
{
	(void)action;
	(void)param;
	g_application_quit(G_APPLICATION(data));
}

static void
activate(GApplication *app, gpointer data)
{
	GtkWindow *window;

	(void)data;
	window = gtk_application_get_active_window(GTK_APPLICATION(app));
	if (window == NULL)
		window = GTK_WINDOW(gmarkdown_window_new(GTK_APPLICATION(app)));
	gtk_window_present(window);
}

static void
open_files(GApplication *app, GFile **files, gint n_files, const char *hint,
    gpointer data)
{
	gint i;

	(void)hint;
	(void)data;
	for (i = 0; i < n_files; i++) {
		GtkWidget *window = gmarkdown_window_new(GTK_APPLICATION(app));
		gmarkdown_window_open_file(GMARKDOWN_WINDOW(window), files[i]);
		gtk_window_present(GTK_WINDOW(window));
	}
}

static void
startup(GApplication *app, gpointer data)
{
	static const GActionEntry app_actions[] = {
		{ "quit", quit_app, NULL, NULL, NULL, { 0 } },
	};

	(void)data;
	register_bundle_scheme();
	g_action_map_add_action_entries(G_ACTION_MAP(app), app_actions,
	    G_N_ELEMENTS(app_actions), app);
	set_accels(GTK_APPLICATION(app));
}

static void
maybe_daemonize(void)
{
	pid_t pid;

	if (g_getenv("GMARKDOWN_FOREGROUND") != NULL)
		return;
	if (!isatty(STDOUT_FILENO))
		return;
	pid = fork();
	if (pid < 0)
		return;
	if (pid > 0)
		_exit(0);
	setsid();
}

int
main(int argc, char **argv)
{
	g_autoptr(AdwApplication) app = NULL;
	int status;

	if (g_getenv("WEBKIT_DISABLE_COMPOSITING_MODE") == NULL)
		g_setenv("WEBKIT_DISABLE_COMPOSITING_MODE", "1", FALSE);

	maybe_daemonize();
	gmarkdown_markdown_init();
	app = adw_application_new(GMARKDOWN_APP_ID, G_APPLICATION_HANDLES_OPEN);
	g_signal_connect(app, "startup", G_CALLBACK(startup), NULL);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	g_signal_connect(app, "open", G_CALLBACK(open_files), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	return (status);
}
