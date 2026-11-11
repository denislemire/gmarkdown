/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026 Denis Lemire */

#ifndef GMARKDOWN_WINDOW_H
#define GMARKDOWN_WINDOW_H

#include <adwaita.h>

#define GMARKDOWN_TYPE_WINDOW (gmarkdown_window_get_type())
G_DECLARE_FINAL_TYPE(GMarkdownWindow, gmarkdown_window, GMARKDOWN, WINDOW,
    AdwApplicationWindow)

GtkWidget *gmarkdown_window_new(GtkApplication *app);
void gmarkdown_window_open_file(GMarkdownWindow *self, GFile *file);

#endif
