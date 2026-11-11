/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026 Denis Lemire */

#include "print.h"

void
gmarkdown_print_dialog(WebKitWebView *view, GtkWindow *parent)
{
	WebKitPrintOperation *op;

	g_return_if_fail(WEBKIT_IS_WEB_VIEW(view));
	op = webkit_print_operation_new(view);
	webkit_print_operation_run_dialog(op, parent);
	g_object_unref(op);
}

static void
export_chosen(GObject *source, GAsyncResult *result, gpointer data)
{
	WebKitWebView *view = data;
	g_autoptr(GError) error = NULL;
	g_autoptr(GFile) file = NULL;
	g_autoptr(GtkPrintSettings) settings = NULL;
	g_autoptr(WebKitPrintOperation) op = NULL;
	g_autofree char *uri = NULL;

	file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(source), result, &error);
	if (file == NULL)
		return;
	uri = g_file_get_uri(file);
	settings = gtk_print_settings_new();
	gtk_print_settings_set_printer(settings, "Print to File");
	gtk_print_settings_set(settings, GTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT, "pdf");
	gtk_print_settings_set(settings, GTK_PRINT_SETTINGS_OUTPUT_URI, uri);
	op = webkit_print_operation_new(view);
	webkit_print_operation_set_print_settings(op, settings);
	webkit_print_operation_print(op);
}

void
gmarkdown_export_pdf(WebKitWebView *view, GtkWindow *parent)
{
	g_autoptr(GtkFileDialog) dialog = NULL;
	g_autoptr(GtkFileFilter) filter = NULL;

	g_return_if_fail(WEBKIT_IS_WEB_VIEW(view));
	dialog = gtk_file_dialog_new();
	gtk_file_dialog_set_title(dialog, "Export PDF");
	gtk_file_dialog_set_initial_name(dialog, "document.pdf");
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "PDF");
	gtk_file_filter_add_suffix(filter, "pdf");
	gtk_file_dialog_set_default_filter(dialog, filter);
	gtk_file_dialog_save(dialog, parent, NULL, export_chosen, view);
}
