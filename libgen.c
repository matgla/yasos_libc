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

/* POSIX basename: the last component, trailing slashes removed in place.
   "usr/lib" -> "lib", "/usr/" -> "usr", "usr" -> "usr", "/" -> "/",
   NULL or "" -> ".". It used to return null for NULL and "" for "/usr/". */
char *basename(char *path)
{
	static char dot[] = ".";
	size_t i;
	if (path == NULL || path[0] == '\0')
		return dot;
	i = strlen(path) - 1;
	/* trailing slashes; one is kept when the path is nothing else */
	for (; i > 0 && path[i] == '/'; i--)
		path[i] = '\0';
	/* back to the slash before the last component */
	for (; i > 0 && path[i - 1] != '/'; i--)
		;
	return path + i;
}
