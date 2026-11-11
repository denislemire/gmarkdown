/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026 Denis Lemire */

#include "window.h"
#include "gmarkdown.h"
#include "markdown.h"
#include "print.h"

#include <webkit/webkit.h>

struct _GMarkdownWindow {
	AdwApplicationWindow parent;
	WebKitWebView *view;
	GtkWidget *toc_list;
	GtkWidget *split;
	GtkWidget *title_label;
	GtkWidget *find_entry;
	GtkWidget *find_revealer;
	GFile *file;
	GFileMonitor *monitor;
	char *base_uri;
	guint reload_source;
};

G_DEFINE_FINAL_TYPE(GMarkdownWindow, gmarkdown_window, ADW_TYPE_APPLICATION_WINDOW)

static const char *
current_theme(void)
{
	AdwStyleManager *sm = adw_style_manager_get_default();

	return (adw_style_manager_get_dark(sm) ? "dark" : "light");
}

static void
toc_clear(GMarkdownWindow *self)
{
	GtkWidget *child;

	while ((child = gtk_widget_get_first_child(self->toc_list)) != NULL)
		gtk_list_box_remove(GTK_LIST_BOX(self->toc_list), child);
}

static void
toc_activated(GtkListBox *box, GtkListBoxRow *row, gpointer data)
{
	GMarkdownWindow *self = data;
	const char *id;
	g_autofree char *script = NULL;

	(void)box;
	id = g_object_get_data(G_OBJECT(row), "heading-id");
	if (id == NULL)
		return;
	script = g_strdup_printf(
	    "(function(){var e=document.getElementById(\"%s\");"
	    "if(e) e.scrollIntoView();})();",
	    id);
	webkit_web_view_evaluate_javascript(self->view, script, -1, NULL, NULL,
	    NULL, NULL, NULL);
}

static void
fill_toc(GMarkdownWindow *self, GMarkdownDocument *doc)
{
	guint i;

	toc_clear(self);
	for (i = 0; i < doc->headings->len; i++) {
		GMarkdownHeading *h = doc->headings->pdata[i];
		GtkWidget *label;
		GtkWidget *row;
		g_autofree char *pad = NULL;

		pad = g_strnfill((gsize)MAX(0, h->level - 1) * 2, ' ');
		label = gtk_label_new(NULL);
		gtk_label_set_xalign(GTK_LABEL(label), 0);
		gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
		{
			g_autofree char *text = g_strdup_printf("%s%s", pad,
			    h->title);
			gtk_label_set_text(GTK_LABEL(label), text);
		}
		row = gtk_list_box_row_new();
		gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
		g_object_set_data_full(G_OBJECT(row), "heading-id",
		    g_strdup(h->id), g_free);
		gtk_list_box_append(GTK_LIST_BOX(self->toc_list), row);
	}
}

static void
load_markdown_bytes(GMarkdownWindow *self, const char *text, gssize len,
    const char *title)
{
	g_autoptr(GMarkdownDocument) doc = NULL;
	g_autofree char *html = NULL;

	doc = gmarkdown_document_parse(text, len);
	if (doc == NULL)
		return;
	html = gmarkdown_document_wrap_html(doc, title, current_theme());
	fill_toc(self, doc);
	gtk_label_set_text(GTK_LABEL(self->title_label),
	    title != NULL ? title : "GMarkdown");
	webkit_web_view_load_html(self->view, html,
	    self->base_uri != NULL ? self->base_uri : "about:blank");
}

static gboolean
reload_file(gpointer data)
{
	GMarkdownWindow *self = data;
	g_autoptr(GError) error = NULL;
	g_autofree char *contents = NULL;
	g_autofree char *title = NULL;
	gsize len = 0;

	self->reload_source = 0;
	if (self->file == NULL)
		return (G_SOURCE_REMOVE);
	if (!g_file_load_contents(self->file, NULL, &contents, &len, NULL, &error)) {
		g_warning("reload failed: %s", error->message);
		return (G_SOURCE_REMOVE);
	}
	title = g_file_get_basename(self->file);
	load_markdown_bytes(self, contents, (gssize)len, title);
	return (G_SOURCE_REMOVE);
}

static void
file_changed(GFileMonitor *monitor, GFile *file, GFile *other,
    GFileMonitorEvent event, gpointer data)
{
	GMarkdownWindow *self = data;

	(void)monitor;
	(void)file;
	(void)other;
	if (event != G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT &&
	    event != G_FILE_MONITOR_EVENT_CREATED &&
	    event != G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED)
		return;
	if (self->reload_source != 0)
		g_source_remove(self->reload_source);
	self->reload_source = g_timeout_add(120, reload_file, self);
}

static void
watch_file(GMarkdownWindow *self)
{
	g_autoptr(GError) error = NULL;

	g_clear_object(&self->monitor);
	if (self->file == NULL)
		return;
	self->monitor = g_file_monitor_file(self->file, G_FILE_MONITOR_NONE,
	    NULL, &error);
	if (self->monitor == NULL) {
		g_warning("monitor: %s", error->message);
		return;
	}
	g_signal_connect(self->monitor, "changed", G_CALLBACK(file_changed), self);
}

void
gmarkdown_window_open_file(GMarkdownWindow *self, GFile *file)
{
	g_autoptr(GError) error = NULL;
	g_autofree char *contents = NULL;
	g_autofree char *title = NULL;
	g_autoptr(GFile) parent = NULL;
	gsize len = 0;

	g_return_if_fail(GMARKDOWN_IS_WINDOW(self));
	g_return_if_fail(G_IS_FILE(file));
	if (!g_file_load_contents(file, NULL, &contents, &len, NULL, &error)) {
		AdwDialog *dialog = adw_alert_dialog_new("Could not open file",
		    error->message);
		adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "ok", "OK");
		adw_dialog_present(dialog, GTK_WIDGET(self));
		return;
	}
	g_set_object(&self->file, file);
	g_clear_pointer(&self->base_uri, g_free);
	parent = g_file_get_parent(file);
	if (parent != NULL) {
		g_autofree char *uri = g_file_get_uri(parent);
		self->base_uri = g_strconcat(uri, "/", NULL);
	}
	title = g_file_get_basename(file);
	{
		g_autofree char *uri = g_file_get_uri(file);
		gtk_recent_manager_add_item(gtk_recent_manager_get_default(),
		    uri);
	}
	load_markdown_bytes(self, contents, (gssize)len, title);
	watch_file(self);
}

static void
open_chosen(GObject *source, GAsyncResult *result, gpointer data)
{
	GMarkdownWindow *self = data;
	g_autoptr(GError) error = NULL;
	g_autoptr(GFile) file = NULL;

	file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), result, &error);
	if (file != NULL)
		gmarkdown_window_open_file(self, file);
}

static void
action_open(GSimpleAction *action, GVariant *param, gpointer data)
{
	GMarkdownWindow *self = data;
	g_autoptr(GtkFileDialog) dialog = NULL;
	g_autoptr(GtkFileFilter) filter = NULL;
	g_autoptr(GListStore) filters = NULL;

	(void)action;
	(void)param;
	dialog = gtk_file_dialog_new();
	gtk_file_dialog_set_title(dialog, "Open Markdown");
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Markdown");
	gtk_file_filter_add_mime_type(filter, "text/markdown");
	gtk_file_filter_add_suffix(filter, "md");
	gtk_file_filter_add_suffix(filter, "markdown");
	gtk_file_filter_add_suffix(filter, "mdown");
	filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
	g_list_store_append(filters, filter);
	gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
	gtk_file_dialog_open(dialog, GTK_WINDOW(self), NULL, open_chosen, self);
}

static void
action_print(GSimpleAction *action, GVariant *param, gpointer data)
{
	GMarkdownWindow *self = data;

	(void)action;
	(void)param;
	gmarkdown_print_dialog(self->view, GTK_WINDOW(self));
}

static void
action_export(GSimpleAction *action, GVariant *param, gpointer data)
{
	GMarkdownWindow *self = data;

	(void)action;
	(void)param;
	gmarkdown_export_pdf(self->view, GTK_WINDOW(self));
}

static void
action_find(GSimpleAction *action, GVariant *param, gpointer data)
{
	GMarkdownWindow *self = data;

	(void)action;
	(void)param;
	gtk_revealer_set_reveal_child(GTK_REVEALER(self->find_revealer), TRUE);
	gtk_widget_grab_focus(self->find_entry);
}

static void
action_toc(GSimpleAction *action, GVariant *param, gpointer data)
{
	GMarkdownWindow *self = data;
	gboolean show;

	(void)action;
	(void)param;
	show = !adw_overlay_split_view_get_show_sidebar(
	    ADW_OVERLAY_SPLIT_VIEW(self->split));
	adw_overlay_split_view_set_show_sidebar(
	    ADW_OVERLAY_SPLIT_VIEW(self->split), show);
}

static void
action_zoom(GSimpleAction *action, GVariant *param, gpointer data)
{
	GMarkdownWindow *self = data;
	const char *name = g_action_get_name(G_ACTION(action));
	gdouble zoom = webkit_web_view_get_zoom_level(self->view);

	(void)param;
	if (g_strcmp0(name, "zoom-in") == 0)
		zoom *= 1.1;
	else if (g_strcmp0(name, "zoom-out") == 0)
		zoom /= 1.1;
	else
		zoom = 1.0;
	webkit_web_view_set_zoom_level(self->view, CLAMP(zoom, 0.5, 3.0));
}

static void
find_changed(GtkEditable *editable, gpointer data)
{
	GMarkdownWindow *self = data;
	const char *text = gtk_editable_get_text(editable);
	WebKitFindController *fc = webkit_web_view_get_find_controller(self->view);

	if (text == NULL || text[0] == '\0') {
		webkit_find_controller_search_finish(fc);
		return;
	}
	webkit_find_controller_search(fc, text,
	    WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE |
	    WEBKIT_FIND_OPTIONS_WRAP_AROUND, 1000);
}

static gboolean
decide_policy(WebKitWebView *view, WebKitPolicyDecision *decision,
    WebKitPolicyDecisionType type, gpointer data)
{
	WebKitNavigationAction *action;
	WebKitURIRequest *request;
	const char *uri;

	(void)view;
	(void)data;
	if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION &&
	    type != WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION)
		return (FALSE);
	action = webkit_navigation_policy_decision_get_navigation_action(
	    WEBKIT_NAVIGATION_POLICY_DECISION(decision));
	if (webkit_navigation_action_get_navigation_type(action) !=
	    WEBKIT_NAVIGATION_TYPE_LINK_CLICKED)
		return (FALSE);
	request = webkit_navigation_action_get_request(action);
	uri = webkit_uri_request_get_uri(request);
	webkit_policy_decision_ignore(decision);
	if (g_str_has_prefix(uri, "http://") || g_str_has_prefix(uri, "https://") ||
	    g_str_has_prefix(uri, "mailto:")) {
		g_autoptr(GtkUriLauncher) launcher = gtk_uri_launcher_new(uri);
		gtk_uri_launcher_launch(launcher, NULL, NULL, NULL, NULL);
	}
	return (TRUE);
}

static void
theme_changed(AdwStyleManager *sm, GParamSpec *pspec, gpointer data)
{
	GMarkdownWindow *self = data;

	(void)sm;
	(void)pspec;
	if (self->file != NULL)
		reload_file(self);
}

static gboolean
on_drop(GtkDropTarget *target, const GValue *value, double x, double y,
    gpointer data)
{
	GMarkdownWindow *self = data;

	(void)target;
	(void)x;
	(void)y;
	if (G_VALUE_HOLDS(value, G_TYPE_FILE)) {
		gmarkdown_window_open_file(self, g_value_get_object(value));
		return (TRUE);
	}
	return (FALSE);
}

static void
gmarkdown_window_dispose(GObject *object)
{
	GMarkdownWindow *self = GMARKDOWN_WINDOW(object);

	if (self->reload_source != 0) {
		g_source_remove(self->reload_source);
		self->reload_source = 0;
	}
	g_clear_object(&self->monitor);
	g_clear_object(&self->file);
	g_clear_pointer(&self->base_uri, g_free);
	G_OBJECT_CLASS(gmarkdown_window_parent_class)->dispose(object);
}

static void
gmarkdown_window_class_init(GMarkdownWindowClass *klass)
{
	G_OBJECT_CLASS(klass)->dispose = gmarkdown_window_dispose;
}

static const GActionEntry win_actions[] = {
	{ "open", action_open, NULL, NULL, NULL, { 0 } },
	{ "print", action_print, NULL, NULL, NULL, { 0 } },
	{ "export-pdf", action_export, NULL, NULL, NULL, { 0 } },
	{ "find", action_find, NULL, NULL, NULL, { 0 } },
	{ "toc", action_toc, NULL, NULL, NULL, { 0 } },
	{ "zoom-in", action_zoom, NULL, NULL, NULL, { 0 } },
	{ "zoom-out", action_zoom, NULL, NULL, NULL, { 0 } },
	{ "zoom-reset", action_zoom, NULL, NULL, NULL, { 0 } },
};

static void
gmarkdown_window_init(GMarkdownWindow *self)
{
	GtkWidget *toolbar;
	GtkWidget *header;
	GtkWidget *open_btn;
	GtkWidget *menu_btn;
	GtkWidget *find_box;
	GtkWidget *sidebar;
	GtkWidget *scrolled;
	GtkWidget *drop_host;
	GtkDropTarget *drop;
	GMenu *menu;
	WebKitSettings *settings;
	AdwStyleManager *sm;

	g_action_map_add_action_entries(G_ACTION_MAP(self), win_actions,
	    G_N_ELEMENTS(win_actions), self);

	gtk_window_set_default_size(GTK_WINDOW(self), 960, 720);
	gtk_window_set_title(GTK_WINDOW(self), "GMarkdown");

	self->view = WEBKIT_WEB_VIEW(webkit_web_view_new());
	settings = webkit_web_view_get_settings(self->view);
	webkit_settings_set_enable_javascript(settings, TRUE);
	webkit_settings_set_enable_page_cache(settings, FALSE);
	webkit_settings_set_allow_file_access_from_file_urls(settings, TRUE);
	webkit_settings_set_enable_smooth_scrolling(settings, TRUE);
	g_signal_connect(self->view, "decide-policy", G_CALLBACK(decide_policy),
	    self);

	self->title_label = gtk_label_new("GMarkdown");
	gtk_label_set_ellipsize(GTK_LABEL(self->title_label), PANGO_ELLIPSIZE_END);
	gtk_widget_add_css_class(self->title_label, "title");

	open_btn = gtk_button_new_from_icon_name("document-open-symbolic");
	gtk_actionable_set_action_name(GTK_ACTIONABLE(open_btn), "win.open");
	gtk_widget_set_tooltip_text(open_btn, "Open Markdown");

	menu = g_menu_new();
	g_menu_append(menu, "Print…", "win.print");
	g_menu_append(menu, "Export PDF…", "win.export-pdf");
	g_menu_append(menu, "Find", "win.find");
	g_menu_append(menu, "Contents", "win.toc");
	menu_btn = gtk_menu_button_new();
	gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_btn),
	    "open-menu-symbolic");
	gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_btn), G_MENU_MODEL(menu));
	g_object_unref(menu);

	header = adw_header_bar_new();
	adw_header_bar_set_title_widget(ADW_HEADER_BAR(header), self->title_label);
	adw_header_bar_pack_start(ADW_HEADER_BAR(header), open_btn);
	adw_header_bar_pack_end(ADW_HEADER_BAR(header), menu_btn);

	self->find_entry = gtk_search_entry_new();
	gtk_widget_set_hexpand(self->find_entry, TRUE);
	g_signal_connect(self->find_entry, "search-changed",
	    G_CALLBACK(find_changed), self);
	find_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_set_margin_start(find_box, 12);
	gtk_widget_set_margin_end(find_box, 12);
	gtk_widget_set_margin_bottom(find_box, 6);
	gtk_box_append(GTK_BOX(find_box), self->find_entry);
	self->find_revealer = gtk_revealer_new();
	gtk_revealer_set_child(GTK_REVEALER(self->find_revealer), find_box);

	self->toc_list = gtk_list_box_new();
	gtk_widget_add_css_class(self->toc_list, "navigation-sidebar");
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(self->toc_list),
	    GTK_SELECTION_SINGLE);
	g_signal_connect(self->toc_list, "row-activated", G_CALLBACK(toc_activated),
	    self);
	scrolled = gtk_scrolled_window_new();
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), self->toc_list);
	sidebar = adw_toolbar_view_new();
	adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(sidebar), scrolled);

	drop_host = gtk_scrolled_window_new();
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(drop_host),
	    GTK_WIDGET(self->view));
	drop = gtk_drop_target_new(G_TYPE_FILE, GDK_ACTION_COPY);
	g_signal_connect(drop, "drop", G_CALLBACK(on_drop), self);
	gtk_widget_add_controller(drop_host, GTK_EVENT_CONTROLLER(drop));

	self->split = adw_overlay_split_view_new();
	adw_overlay_split_view_set_sidebar(ADW_OVERLAY_SPLIT_VIEW(self->split),
	    sidebar);
	adw_overlay_split_view_set_content(ADW_OVERLAY_SPLIT_VIEW(self->split),
	    drop_host);
	adw_overlay_split_view_set_show_sidebar(ADW_OVERLAY_SPLIT_VIEW(self->split),
	    FALSE);
	adw_overlay_split_view_set_max_sidebar_width(
	    ADW_OVERLAY_SPLIT_VIEW(self->split), 280);

	toolbar = adw_toolbar_view_new();
	adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), header);
	adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), self->find_revealer);
	adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), self->split);
	adw_application_window_set_content(ADW_APPLICATION_WINDOW(self), toolbar);

	sm = adw_style_manager_get_default();
	g_signal_connect_object(sm, "notify::dark", G_CALLBACK(theme_changed),
	    self, 0);

	load_markdown_bytes(self,
	    "# GMarkdown\n\nOpen a Markdown file, or drop one here.\n",
	    -1, "GMarkdown");
}

GtkWidget *
gmarkdown_window_new(GtkApplication *app)
{
	return (g_object_new(GMARKDOWN_TYPE_WINDOW, "application", app, NULL));
}
