/* 
 *   Copyright (c) 2012, Matti Katila
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Affero General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU Affero General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * Written by Matti Katila
 */

// MAKE: gcc triples.c -o triples `pkg-config glib-2.0 --cflags --libs ` && ./triples test add 1 2 3

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <glib.h>

static const char *APP;

static off_t file_len(FILE *fs)
{
	struct stat fstatus;
	fstat(fileno(fs), &fstatus);
	if (fstatus.st_size < 0) 
		return 0;
	return fstatus.st_size;
}

static off_t find_pos_rec(FILE *fs, char *line, off_t a, off_t b)
{
	if (b-a < 6)
		b = a;

	//printf("find %zd %zd\n", a, b);

	off_t half = a + (b - a) / 2;
	//printf("half: %zd\n", half);
	fseeko(fs, half, SEEK_SET);

	// fast forward to next new line (or eof)
	// except if we know we are in the beginning of the line
	int ch;
	if (half > 0L) {
		while (true) {
			ch = fgetc(fs);
			//printf("%c", ch);
			if (ch == '\n' || ch == EOF)
				break;
		}
	}
	//printf("\n");
	if (ch == EOF)
		return find_pos_rec(fs, line, a, half);
	
	// read next line
	char buff[1024*4];
	char *s = fgets(buff, sizeof(buff), fs);
	if (s == NULL && feof(fs))
		return find_pos_rec(fs, line, a, half);

	int cmp = strcmp(line, s);
	//printf("line: %d, %s", cmp, s);
	if (cmp == 0) // line exists already.
		// we should signal back
		return ftello(fs) - strlen(line);
	if (cmp < 0) {
		if (a == half)
			return a;
		return find_pos_rec(fs, line, a, half);
	}
	if (b <= ftello(fs))
		return ftello(fs);
	else if (cmp > 0)
		return find_pos_rec(fs, line, ftello(fs)-1, b);
}

static off_t find_pos(FILE *fs, char *line)
{
	off_t total_len = file_len(fs);
	off_t pos = find_pos_rec(fs, line, 0L, total_len);
	return pos;
}

void add(FILE *fs, char *subj, char *p, char *o)
{
	char line_add[1024*4];
	snprintf(line_add, sizeof(line_add), "%s %s %s\n", subj,p,o);

	off_t seek = find_pos(fs, line_add);
	char buff[1024*4];

	fseeko(fs, seek, SEEK_SET);
	char *s = fgets(buff, sizeof(buff), fs);
	if (strcmp(s, line_add) == 0)
		exit(0);

	//printf("pos: %zd\n", seek);


	// write rest of the file to temp
	FILE *rest = tmpfile();
	size_t r, w;
	fseeko(fs, seek, SEEK_SET);
	do {
		r = fread(buff, 1, sizeof(buff), fs);
		w = fwrite(buff, 1, r, rest);
	} while (w > 0);
	fflush(rest);

	// make addition
	fseeko(fs, seek, SEEK_SET);
	fwrite(line_add, strlen(line_add), 1, fs);

	// copy rest back
	fseeko(rest, 0L, SEEK_SET);
	do {
		r = fread(buff, 1, sizeof(buff), rest);
		w = fwrite(buff, 1, r, fs);
	} while (w > 0);
	fflush(fs);
}

void rm(FILE *fs, char *s, char *p, char *o)
{
	char line_rm[1024*4];
	snprintf(line_rm, sizeof(line_rm), "%s %s %s\n", s,p,o);

	off_t seek = find_pos(fs, line_rm);
	//printf("pos: %zd\n", seek);

	char buff[1024*4];

	// write rest of the file to temp
	FILE *rest = tmpfile();
	size_t r, w;
	fseeko(fs, seek, SEEK_SET);
	fseeko(fs, strlen(line_rm), SEEK_CUR);
	do {
		r = fread(buff, 1, sizeof(buff), fs);
		w = fwrite(buff, 1, r, rest);
	} while (w > 0);
	fflush(rest);

	// seek back
	fseeko(fs, seek, SEEK_SET);

	// copy rest back
	fseeko(rest, 0L, SEEK_SET);
	do {
		r = fread(buff, 1, sizeof(buff), rest);
		w = fwrite(buff, 1, r, fs);
	} while (w > 0);
	ftruncate(fileno(fs), ftello(fs));
	fflush(fs);
}

static void print_help()
{
	g_print("This is triple-git: an RDFstorage on top of git.\n\n"
		"Available commands are:\n"
		" %s <file> add <subj> <pred> <object or literal>\n", APP);
}

int main(int argc, char *argv[])
{
	APP = argv[0];
	if (argc < 3) {
		print_help();
		return 0;
	}
	char *file = argv[1];
	FILE *fs = fopen(file, "r+");
	if (fs == NULL) {
		g_print("Error opening file %s", file);
		return 1;
	}

	// <cmd> <file> add <subj> <pred> <obj/literal>
	if (g_strcmp0(argv[2], "add") == 0) {
		if (argc != 6) {
			g_print("Waiting for five arguments.\n\n");
			print_help();
			return 1;
		}
		char 
			*s = argv[3],
			*p = argv[4],
			*o = argv[5];
		add(fs, s,p,o);
	} else if (g_strcmp0(argv[2], "rm") == 0) {
		if (argc != 6) {
			g_print("Waiting for five arguments.\n\n");
			print_help();
			return 1;
		}
		char 
			*s = argv[3],
			*p = argv[4],
			*o = argv[5];
		rm(fs, s,p,o);
	} else {
		print_help();
		return 1;
	}
}
