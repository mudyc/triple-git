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

#include <glib.h>


void add(char *s, char *p, char *o)
{
  g_debug("%s %s %s\n", s, p, o);
  
}

int main(int argc, char *argv[])
{
  if (argc < 2)
    return 0;
  
  if (g_strcmp0(argv[1], "add") == 0 && argc == 5) {
    g_debug("add triple");
    // <cmd> add <subj> <pred> <obj/literal>
    char 
      *s = argv[2],
      *p = argv[3],
      *o = argv[4];
    add(s,p,o);
  } else {
    g_print("The given command did not match any available commands.\n"
	    " %s add <subj> <pred> <object or literal>\n", argv[0]);
    return 1;
  }
}
