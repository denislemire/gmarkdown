/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026 Denis Lemire */

#include "markdown.h"
#include "gmarkdown.h"

#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>
#include <string.h>
#include <ctype.h>

static const char *const gfm_extensions[] = {
	"table",
	"strikethrough",
	"autolink",
	"tagfilter",
	"tasklist",
	NULL
};

void
gmarkdown_markdown_init(void)
{
	cmark_gfm_core_extensions_ensure_registered();
}

static char *
heading_text(cmark_node *heading)
{
	GString *text;
	cmark_iter *iter;
	cmark_event_type ev;
	cmark_node *node;

	text = g_string_new(NULL);
	iter = cmark_iter_new(heading);
	while ((ev = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
		node = cmark_iter_get_node(iter);
		if (node == heading && ev == CMARK_EVENT_EXIT)
			break;
		if (ev == CMARK_EVENT_ENTER &&
		    (cmark_node_get_type(node) == CMARK_NODE_TEXT ||
		    cmark_node_get_type(node) == CMARK_NODE_CODE)) {
			const char *lit = cmark_node_get_literal(node);
			if (lit != NULL)
				g_string_append(text, lit);
		}
	}
	cmark_iter_free(iter);
	return (g_string_free(text, FALSE));
}

static char *
github_slug(const char *title, GHashTable *used)
{
	GString *slug;
	const guchar *p;
	gboolean dash = FALSE;
	char *result;
	char *candidate;
	guint n = 2;

	slug = g_string_new(NULL);
	for (p = (const guchar *)title; *p != '\0'; p++) {
		if (g_ascii_isalnum(*p)) {
			g_string_append_c(slug, g_ascii_tolower(*p));
			dash = FALSE;
		} else if (*p >= 0x80) {
			g_string_append_c(slug, (char)*p);
			dash = FALSE;
		} else if (!dash && slug->len > 0) {
			g_string_append_c(slug, '-');
			dash = TRUE;
		}
	}
	while (slug->len > 0 && slug->str[slug->len - 1] == '-')
		g_string_truncate(slug, slug->len - 1);
	if (slug->len == 0)
		g_string_assign(slug, "section");
	candidate = g_strdup(slug->str);
	while (g_hash_table_contains(used, candidate)) {
		g_free(candidate);
		candidate = g_strdup_printf("%s-%u", slug->str, n++);
	}
	g_hash_table_add(used, g_strdup(candidate));
	g_string_free(slug, TRUE);
	result = candidate;
	return (result);
}

static void
collect_headings(cmark_node *doc, GPtrArray *headings)
{
	cmark_iter *iter;
	cmark_event_type ev;
	cmark_node *node;
	GHashTable *used;

	used = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	iter = cmark_iter_new(doc);
	while ((ev = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
		node = cmark_iter_get_node(iter);
		if (ev == CMARK_EVENT_ENTER &&
		    cmark_node_get_type(node) == CMARK_NODE_HEADING) {
			GMarkdownHeading *h = g_new0(GMarkdownHeading, 1);
			h->level = cmark_node_get_heading_level(node);
			h->title = heading_text(node);
			h->id = github_slug(h->title, used);
			g_ptr_array_add(headings, h);
		}
	}
	cmark_iter_free(iter);
	g_hash_table_destroy(used);
}

static char *
inject_heading_ids(char *html, GPtrArray *headings)
{
	GString *out;
	const char *p;
	guint i = 0;

	out = g_string_sized_new(strlen(html) + 64);
	p = html;
	while (*p != '\0') {
		if (p[0] == '<' && p[1] == 'h' && p[2] >= '1' && p[2] <= '6' &&
		    p[3] == '>' && i < headings->len) {
			GMarkdownHeading *h = headings->pdata[i++];
			g_string_append_printf(out, "<h%c id=\"%s\">", p[2], h->id);
			p += 4;
			continue;
		}
		g_string_append_c(out, *p++);
	}
	g_free(html);
	return (g_string_free(out, FALSE));
}

static const char *
alert_label(const char *kind)
{
	if (g_ascii_strcasecmp(kind, "NOTE") == 0)
		return ("Note");
	if (g_ascii_strcasecmp(kind, "TIP") == 0)
		return ("Tip");
	if (g_ascii_strcasecmp(kind, "IMPORTANT") == 0)
		return ("Important");
	if (g_ascii_strcasecmp(kind, "WARNING") == 0)
		return ("Warning");
	if (g_ascii_strcasecmp(kind, "CAUTION") == 0)
		return ("Caution");
	return (kind);
}

static char *
rewrite_alerts(char *html)
{
	GString *out;
	GMatchInfo *info = NULL;
	g_autoptr(GRegex) re = NULL;
	const char *p;
	gint start, end;

	re = g_regex_new(
	    "<blockquote>\\s*<p>\\[!(NOTE|TIP|IMPORTANT|WARNING|CAUTION)\\]\\s*",
	    G_REGEX_CASELESS, 0, NULL);
	if (re == NULL)
		return (html);
	out = g_string_sized_new(strlen(html) + 64);
	p = html;
	if (g_regex_match(re, html, 0, &info)) {
		do {
			g_autofree char *kind = NULL;
			const char *label;
			g_autofree char *klass = NULL;

			g_match_info_fetch_pos(info, 0, &start, &end);
			g_string_append_len(out, p, html + start - p);
			kind = g_match_info_fetch(info, 1);
			label = alert_label(kind);
			klass = g_ascii_strdown(kind, -1);
			g_string_append_printf(out,
			    "<div class=\"markdown-alert markdown-alert-%s\" role=\"note\">"
			    "<p class=\"markdown-alert-title\">%s</p><p>",
			    klass, label);
			p = html + end;
		} while (g_match_info_next(info, NULL));
	}
	if (info != NULL)
		g_match_info_free(info);
	g_string_append(out, p);
	g_free(html);
	html = g_string_free(out, FALSE);

	/* Close alert divs: replace the first </blockquote> after each alert open.
	   Nested quotes inside alerts are uncommon in the GFM alert form. */
	out = g_string_sized_new(strlen(html));
	p = html;
	while (*p != '\0') {
		const char *div = strstr(p, "markdown-alert markdown-alert-");
		const char *close;
		if (div == NULL) {
			g_string_append(out, p);
			break;
		}
		g_string_append_len(out, p, div - p);
		close = strstr(div, "</blockquote>");
		if (close == NULL) {
			g_string_append(out, div);
			break;
		}
		g_string_append_len(out, div, close - div);
		g_string_append(out, "</div>");
		p = close + strlen("</blockquote>");
	}
	g_free(html);
	return (g_string_free(out, FALSE));
}

static char *
rewrite_mermaid(char *html, gboolean *has_mermaid)
{
	g_autoptr(GRegex) re = NULL;
	g_autofree char *out = NULL;

	re = g_regex_new(
	    "<pre><code class=\"language-mermaid\">([\\s\\S]*?)</code></pre>",
	    0, 0, NULL);
	if (re == NULL)
		return (html);
	if (g_regex_match(re, html, 0, NULL))
		*has_mermaid = TRUE;
	out = g_regex_replace(re, html, -1, 0,
	    "<pre class=\"mermaid\">\\1</pre>", 0, NULL);
	if (out == NULL)
		return (html);
	g_free(html);
	return (g_steal_pointer(&out));
}

static void
heading_free(gpointer data)
{
	GMarkdownHeading *h = data;

	g_free(h->id);
	g_free(h->title);
	g_free(h);
}

GMarkdownDocument *
gmarkdown_document_parse(const char *markdown, gssize len)
{
	GMarkdownDocument *doc;
	cmark_parser *parser;
	cmark_node *root;
	cmark_llist *exts;
	int options;
	gsize n;
	guint i;

	g_return_val_if_fail(markdown != NULL, NULL);
	n = len < 0 ? strlen(markdown) : (gsize)len;
	options = CMARK_OPT_DEFAULT | CMARK_OPT_FOOTNOTES |
	    CMARK_OPT_GITHUB_PRE_LANG | CMARK_OPT_SMART |
	    CMARK_OPT_VALIDATE_UTF8 | CMARK_OPT_UNSAFE;

	parser = cmark_parser_new(options);
	for (i = 0; gfm_extensions[i] != NULL; i++) {
		cmark_syntax_extension *ext =
		    cmark_find_syntax_extension(gfm_extensions[i]);
		if (ext != NULL)
			cmark_parser_attach_syntax_extension(parser, ext);
	}
	cmark_parser_feed(parser, markdown, n);
	root = cmark_parser_finish(parser);
	exts = cmark_parser_get_syntax_extensions(parser);

	doc = g_new0(GMarkdownDocument, 1);
	doc->headings = g_ptr_array_new_with_free_func(heading_free);
	collect_headings(root, doc->headings);
	doc->body_html = cmark_render_html(root, options, exts);
	doc->body_html = inject_heading_ids(doc->body_html, doc->headings);
	doc->body_html = rewrite_alerts(doc->body_html);
	doc->body_html = rewrite_mermaid(doc->body_html, &doc->has_mermaid);

	cmark_node_free(root);
	cmark_parser_free(parser);
	return (doc);
}

void
gmarkdown_document_free(GMarkdownDocument *doc)
{
	if (doc == NULL)
		return;
	g_free(doc->body_html);
	if (doc->headings != NULL)
		g_ptr_array_unref(doc->headings);
	g_free(doc);
}

char *
gmarkdown_document_wrap_html(const GMarkdownDocument *doc, const char *title,
    const char *theme)
{
	const char *hl = "vendor/github.min.css";
	g_autofree char *escaped = NULL;

	g_return_val_if_fail(doc != NULL, NULL);
	if (theme != NULL && g_strcmp0(theme, "dark") == 0)
		hl = "vendor/github-dark.min.css";
	escaped = g_markup_escape_text(title != NULL ? title : "GMarkdown", -1);
	return (g_strdup_printf(
	    "<!DOCTYPE html>\n"
	    "<html lang=\"en\" data-theme=\"%s\">\n"
	    "<head>\n"
	    "<meta charset=\"utf-8\">\n"
	    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
	    "<title>%s</title>\n"
	    "<link rel=\"stylesheet\" href=\"" GMARKDOWN_SCHEME
	    "://bundle/gmarkdown.css\">\n"
	    "<link rel=\"stylesheet\" href=\"" GMARKDOWN_SCHEME
	    "://bundle/%s\">\n"
	    "</head>\n"
	    "<body>\n"
	    "<article class=\"markdown-body\">\n%s\n</article>\n"
	    "<script src=\"" GMARKDOWN_SCHEME "://bundle/vendor/highlight.min.js\"></script>\n"
	    "%s"
	    "<script src=\"" GMARKDOWN_SCHEME "://bundle/app.js\"></script>\n"
	    "</body>\n"
	    "</html>\n",
	    theme != NULL ? theme : "light",
	    escaped,
	    hl,
	    doc->body_html != NULL ? doc->body_html : "",
	    doc->has_mermaid ?
		"<script src=\"" GMARKDOWN_SCHEME
		"://bundle/vendor/mermaid.min.js\"></script>\n" : ""));
}
