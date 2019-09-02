/** @file handler_markdown.cc
 * @brief Extract text from markdown.
 */
/* Copyright (C) 2019 Bruno Baruffaldi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */
#include <config.h>
#include "handler.h"

#include "myhtmlparse.h"
#include <string.h>

extern "C" {
#include <mkdio.h>
}


using namespace std;

static void
extract_from_html(string& text)
{
    MyHtmlParser parser;
    string charset = "UTF-8";
    try {
	parser.ignore_metarobots();
	parser.parse_html(text, charset, false);
	text = parser.dump;
    } catch (const string& newcharset) {
	parser.reset();
	parser.ignore_metarobots();
	parser.parse_html(text, newcharset, true);
	text = parser.dump;
    }
}

bool
extract(const string& filename,
	string& dump,
	string& title,
	string& keywords,
	string& author,
	string& pages,
	string& error)
{
    char* buffer;
    FILE* fp = fopen(filename.c_str(), "r");

    if (fp == NULL) {
	error = "Markdown Error: fail to open " + filename;
	return false;
    }

    MMIOT* doc = mkd_in(fp, 0);

    if (doc == NULL || !mkd_compile(doc, 0)) {
	error = "Markdown Error: fail to compile " + filename;
	return false;
    }

    buffer = NULL;
    if ((buffer = mkd_doc_title(doc)) != NULL) {
	title = buffer;
    }
    buffer = NULL;
    if ((buffer = mkd_doc_author(doc)) != NULL) {
	author = buffer;
    }
    buffer = NULL;
    if (mkd_document(doc, &buffer)) {
	dump = buffer;
	extract_from_html(dump);
    }
    (void)pages;
    (void)keywords;
    return true;
}
