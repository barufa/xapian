/** @file handler_libetonyek_numbers.cc
 * @brief Extract text from Apple Numbers documents using libtonyek.
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
#include "handler.h"

#include <iostream>
#include <memory>

#include <librevenge-generators/librevenge-generators.h>
#include <libetonyek/libetonyek.h>

using std::shared_ptr;
using libetonyek::EtonyekDocument;
using namespace librevenge;
using namespace std;

static
void clear_text(string& str, const char* text)
{
    if (text) {
	for (int i = 0; i < text[i]!='\0'; ++i)
	    if (!isspace(text[i]) || (i && !isspace(text[i - 1])))
		str.push_back(text[i]);
    }
}

bool
extract(const string& filename,
	string& dump,
	string& title,
	string& keywords,
	string& author,
	string& pages)
{
    try {
	shared_ptr<RVNGInputStream> input;
	if (RVNGDirectoryStream::isDirectory(filename.c_str()))
	    input.reset(new RVNGDirectoryStream(filename.c_str()));
	else
	    input.reset(new RVNGFileStream(filename.c_str()));

	EtonyekDocument::Type type = EtonyekDocument::TYPE_UNKNOWN;
	const EtonyekDocument::Confidence confidence =
		EtonyekDocument::isSupported(input.get(), &type);
	if ((EtonyekDocument::CONFIDENCE_NONE == confidence) ||
	    (EtonyekDocument::TYPE_NUMBERS != type)) {
	    cerr << "Libetonyek Error: The format is not supported" << endl;
	    return false;
	}

	if (EtonyekDocument::CONFIDENCE_SUPPORTED_PART == confidence)
	    input.reset(RVNGDirectoryStream::createForParent(filename.c_str()));

	RVNGStringVector content;
	RVNGTextSpreadsheetGenerator document(content);

	if (!EtonyekDocument::parse(input.get(), &document)) {
	    cerr << "Libetonyek Error: Fail to extract text" << endl;
	    return false;
	}

	unsigned size = content.size();
	for (unsigned i = 0; i < size; ++i) {
	    clear_text(dump, content[i].cstr());
	    dump.push_back('\n');
	}
	(void)title;
	(void)keywords;
	(void)author;
	(void)pages;
    } catch (...) {
	cerr << "Libetonyek threw an exception" << endl;
	return false;
    }
    return true;
}
