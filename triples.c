
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
