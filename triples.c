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
#include <stdlib.h>

#include <glib.h>

static const char *APP;
static const char *RDF_FILE;

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
	if (s == NULL && feof(fs)) {
		return find_pos_rec(fs, line, a, half);
	}
	int cmp = strcmp(line, s);
	//printf("line: %d, %s", cmp, s);
	if (cmp == 0) // line exists already.
		// we should signal back
		return ftello(fs) - strlen(line);
	if (cmp < 0) {
		if (a == half) {
			if (a == 0)
				return 0;
			return a + 1;
		}
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
	if (total_len == 0)
		return 0;
	off_t pos = find_pos_rec(fs, line, 0L, total_len);
	return pos;
}

static size_t smin(size_t a, off_t b) {
	return a <= b? a: (size_t)b;
}

static int add(FILE *fs, char *subj, char *p, char *o)
{
	char line_add[1024*4];
	snprintf(line_add, sizeof(line_add), "%s %s %s\n", subj,p,o);

	off_t seek = find_pos(fs, line_add);
	char buff[1024*4];

	fseeko(fs, seek, SEEK_SET);
	char *s = fgets(buff, sizeof(buff), fs);
	if (s != NULL && strcmp(s, line_add) == 0)
		exit(0);

	// generate a new file
	char *new_file = tmpnam(NULL);
	FILE *new = fopen(new_file, "w");
	size_t r, w;
	fseeko(fs, 0L, SEEK_SET);
	do {
		r = fread(buff, 1, smin(sizeof(buff), seek-ftello(fs)), fs);
		w = fwrite(buff, 1, r, new);
	} while (w > 0);

	// make addition
	fwrite(line_add, strlen(line_add), 1, new);

	// write rest of the file to new file
	FILE *rest = tmpfile();
	fseeko(fs, seek, SEEK_SET);
	do {
		r = fread(buff, 1, sizeof(buff), fs);
		w = fwrite(buff, 1, r, new);
	} while (w > 0);
	fflush(new);

	int ret = rename(new_file, RDF_FILE);
	remove(new_file);
	return ret;
}

static int rm(FILE *fs, char *s, char *p, char *o)
{
	char tmp[1024];
	char line_rm[1024*4];
	snprintf(line_rm, sizeof(line_rm), "%s %s %s\n", s,p,o);

	off_t seek = find_pos(fs, line_rm);
	//printf("pos: %zd\n", seek);

	char buff[1024*4];

	// check that we actually found a one
	fseeko(fs, seek, SEEK_SET);
	char *to_be_removed = fgets(buff, sizeof(buff), fs);
	if (to_be_removed == NULL || strcmp(line_rm, to_be_removed) != 0)
		return 1;

	// generate a new file
	char *new_file = tmpnam(tmp);
	FILE *new = fopen(new_file, "w");
	size_t r, w;
	fseeko(fs, 0L, SEEK_SET);
	do {
		r = fread(buff, 1, smin(sizeof(buff), seek-ftello(fs)), fs);
		w = fwrite(buff, 1, r, new);
	} while (w > 0);

	// move forward removed line.
	fseeko(fs, seek, SEEK_SET);
	fseeko(fs, strlen(line_rm), SEEK_CUR);

	// write rest of the file to new file
	do {
		r = fread(buff, 1, sizeof(buff), fs);
		w = fwrite(buff, 1, r, new);
	} while (w > 0);
	fflush(new);

	int ret = rename(new_file, RDF_FILE);
	remove(new_file);
	return ret;
}

static void fetch_xaa(FILE *fs)
{
	char buff[1024*4];
	
	fseeko(fs, 0L, SEEK_SET);
	while (!feof(fs)) {
		char *s = fgets(buff, sizeof(buff), fs);
		if (s == NULL)
			break;
		char *sp = strchr(s, ' ');
		size_t len = sp-s;
		fwrite(s, 1, len, stdout);
		printf("\n");
	}
	fflush(stdout);
}

static void fetch_1xa(FILE *fs, char *subj)
{
	char buff[1024*4];
	char line[1024*4];
	off_t seek = find_pos(fs, subj);
	
	fseeko(fs, seek, SEEK_SET);
	while (!feof(fs)) {
		char *s = fgets(buff, sizeof(buff), fs);
		if (s == NULL)
			break;
		char *sp = strchr(s, ' ');
		size_t len = sp-s + 1;
		snprintf(line, len, "%s", s);
		//printf("%s %s %d\n", line, subj, len);
		if (strcmp(subj, line) != 0)
			break;
		s = ++sp;
		sp = strchr(sp, ' ');
		len = sp-s;
		fwrite(s, 1, len, stdout);
		printf("\n");
	}
	fflush(stdout);
}

static void fetch_11x(FILE *fs, char *subj, char *pred)
{
	char buff[1024*4];
	char line[1024*4];
	char test[1024*4];
	snprintf(test, sizeof(test), "%s %s ", subj, pred);
	off_t seek = find_pos(fs, test);
	
	fseeko(fs, seek, SEEK_SET);
	while (!feof(fs)) {
		const char *s = fgets(buff, sizeof(buff), fs);
		if (s == NULL)
			break;
		char *sp = strchr(s, ' ');
		sp++;
		sp = strchr(sp, ' ');
		sp++;
		size_t len = sp-s + 1;
		snprintf(line, len, "%s", s);
		//printf("%s %s %d\n", line, test, len);
		if (strcmp(test, line) != 0)
			break;
		len = strlen(s) - (sp - s);
		fwrite(sp, 1, len, stdout);
		// last has new line printf("\n");
	}
	fflush(stdout);
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
	RDF_FILE = file;
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
		return add(fs, s,p,o);
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
		return rm(fs, s,p,o);
	} else if (g_strcmp0(argv[2], "xaa") == 0) {
		if (argc != 3) {
			g_print("Waiting for two arguments.\n\n");
			print_help();
			return 1;
		}
		fetch_xaa(fs);
	} else if (g_strcmp0(argv[2], "1xa") == 0) {
		if (argc != 4) {
			g_print("Waiting for three arguments.\n\n");
			print_help();
			return 1;
		}
		fetch_1xa(fs, argv[3]);
	} else if (g_strcmp0(argv[2], "11x") == 0) {
		if (argc != 5) {
			g_print("Waiting for four arguments.\n\n");
			print_help();
			return 1;
		}
		fetch_11x(fs, argv[3], argv[4]);
	} else {
		print_help();
		return 1;
	}
	return 0;
}
