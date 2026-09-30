#include <libgen.h>

#include <string.h>

/* POSIX dirname: the parent of the last component, trailing slashes ignored,
   modifying path in place. "usr/lib" -> "usr", "/usr/" -> "/", "usr" -> ".",
   "/" -> "/", NULL or "" -> ".". */
char *dirname(char *path)
{
	static char dot[] = ".";
	size_t i;
	if (path == NULL || path[0] == '\0')
		return dot;
	i = strlen(path) - 1;
	/* trailing slashes */
	while (i > 0 && path[i] == '/')
		i--;
	/* the last component */
	while (i > 0 && path[i] != '/')
		i--;
	if (path[i] != '/')
		return dot;
	/* slashes separating it from its parent */
	while (i > 0 && path[i] == '/')
		i--;
	path[i + 1] = '\0';
	return path;
}
