/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026 Denis Lemire */

#include "markdown.h"

#include <stdio.h>
#include <string.h>

static int
fail(const char *msg)
{
	fprintf(stderr, "FAIL: %s\n", msg);
	return (1);
}

int
main(void)
{
	const char *path;
	gchar *md = NULL;
	gsize len = 0;
	GMarkdownDocument *doc;
	char *html;
	guint i;
	gboolean saw_h1 = FALSE;

	gmarkdown_markdown_init();
	path = g_getenv("GMARKDOWN_FIXTURE");
	if (path == NULL || !g_file_get_contents(path, &md, &len, NULL))
		return (fail("fixture missing"));
	doc = gmarkdown_document_parse(md, (gssize)len);
	g_free(md);
	if (doc == NULL || doc->body_html == NULL)
		return (fail("parse"));
	if (strstr(doc->body_html, "<table>") == NULL)
		return (fail("table"));
	if (strstr(doc->body_html, "checkbox") == NULL)
		return (fail("tasklist"));
	if (strstr(doc->body_html, "class=\"mermaid\"") == NULL)
		return (fail("mermaid"));
	if (strstr(doc->body_html, "markdown-alert") == NULL)
		return (fail("alert"));
	if (strstr(doc->body_html, "footnote") == NULL &&
	    strstr(doc->body_html, "footnotes") == NULL)
		return (fail("footnote"));
	if (strstr(doc->body_html, "<del>") == NULL)
		return (fail("strikethrough"));
	if (!doc->has_mermaid)
		return (fail("has_mermaid"));
	for (i = 0; i < doc->headings->len; i++) {
		GMarkdownHeading *h = doc->headings->pdata[i];
		if (h->level == 1 && g_strcmp0(h->title, "Fixture") == 0)
			saw_h1 = TRUE;
	}
	if (!saw_h1)
		return (fail("heading"));
	html = gmarkdown_document_wrap_html(doc, "Fixture", "light");
	if (html == NULL || strstr(html, "markdown-body") == NULL)
		return (fail("wrap"));
	if (strstr(html, "mermaid.min.js") == NULL)
		return (fail("wrap mermaid script"));
	g_free(html);
	gmarkdown_document_free(doc);
	fprintf(stderr, "PASS gfm fixture\n");
	return (0);
}
