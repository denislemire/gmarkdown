/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026 Denis Lemire */

#ifndef GMARKDOWN_PRINT_H
#define GMARKDOWN_PRINT_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>

void gmarkdown_print_dialog(WebKitWebView *view, GtkWindow *parent);
void gmarkdown_export_pdf(WebKitWebView *view, GtkWindow *parent);

#endif
