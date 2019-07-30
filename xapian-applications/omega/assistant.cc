/** @file assistant.cc
 * @brief Worker module for putting text extraction into a separate process.
 */
/* Copyright (C) 2011 Olly Betts
 * Copyright (C) 2019 Bruno Baruffaldi
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
#include "worker_comms.h"

#include <csignal>
#include <iostream>
#include "safeunistd.h"
#include <sstream>
#include <fstream>

using namespace std;

const int FD = 3;
const int time_limit = 300;

#if defined HAVE_ALARM

static void
timeout_handler(int sig) {
    _Exit(sig);
}

static void
set_timeout() {
    // Causes the system to generate a SIGALRM signal for the process after
    // time_limit of seconds
    alarm(time_limit);
    // Set a signal handler for SIGALRM
    signal(SIGALRM, timeout_handler);
}

static void
stop_timeout() {
    // Set SIGALRM to be ignore
    signal(SIGALRM, SIG_IGN);
    // A pending alarm request, if any, is cancelled by calling alarm(0)
    alarm(0);
}

#else

static void set_timeout() { }
static void stop_timeout() { }

#endif

string create_html(const string& title,
		   const string& author,
		   const string& keywords,
		   const string& dump)
{
    string html = "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"UTF-8\">\n";
    if (!title.empty())
	html += "<title>" + title + "</title>\n";
    if (!author.empty())
	html += "<meta name=\"author\" content=\"" + author + "\">\n";
    if (!keywords.empty())
	html += "<meta name=\"keywords\" content=\"" + keywords + "\">\n";
    html += "</head>\n<body>\n";
    if (!dump.empty())
	html += "<pre>" + dump + "</pre>\n";
    html += "</body>\n</html>";
    return html;
}

static void
command_extract(const char* filename, ostream& stream)
{
    string dump, title, keywords, author, pages, err;
    // Setting a timeout for avoid infinity loops
    set_timeout();
    bool succeed = extract(filename, dump, title, keywords, author, pages, err);
    stop_timeout();
    if (succeed)
	stream << create_html(title, author, keywords, dump) << endl;
    _Exit(!succeed);
}

// FIXME: Restart filter every N files processed?

int main(int argc, char ** argv)
{
    if (argc == 2) {
	ios::sync_with_stdio(0);
	command_extract(argv[1], std::cout);
    } else if (argc == 3) {
	std::filebuf fb;
	fb.open(argv[2], std::ios::out);
	std::ostream os(&fb);
	command_extract(argv[1], os);
    }

    string filename;
    FILE* sockt = fdopen(FD, "r+");

    while (true) {
	// Read filename.
	if (!read_string(sockt, filename)) break;
	string dump, title, keywords, author, pages, error;
	// Setting a timeout for avoid infinity loops
	set_timeout();
	bool succeed =
	    extract(filename, dump, title, keywords, author, pages, error);
	stop_timeout();
	if (!succeed) {
	    if (!write_string(sockt, string(1, MSG_NON_FATAL_ERROR) + error))
		break;
	} else {
	    if (!write_string(sockt, string(1, MSG_OK)) ||
		!write_string(sockt, dump) ||
		!write_string(sockt, title) ||
		!write_string(sockt, keywords) ||
		!write_string(sockt, author) ||
		!write_string(sockt, pages)) {
		break;
	    }
	}
    }

    return 0;
}
