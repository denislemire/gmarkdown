/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026 Denis Lemire */

#ifndef GMARKDOWN_MARKDOWN_H
#define GMARKDOWN_MARKDOWN_H

#include <glib.h>

typedef struct {
	char *id;
	char *title;
	int level;
} GMarkdownHeading;

typedef struct {
	char *body_html;
	GPtrArray *headings;
	gboolean has_mermaid;
} GMarkdownDocument;

void gmarkdown_markdown_init(void);

GMarkdownDocument *gmarkdown_document_parse(const char *markdown, gssize len);
void gmarkdown_document_free(GMarkdownDocument *doc);

char *gmarkdown_document_wrap_html(const GMarkdownDocument *doc,
    const char *title, const char *theme);

#endif
