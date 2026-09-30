#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static unsigned tmpid;
static const char digs[] = "0123456789abcdefghijklmnopqrstuvwxyz";

/* Replace the trailing "XXXXXX" of t with the next candidate name. NULL when t
   does not end in six X's (EINVAL, as POSIX requires). */
static char *next_name(char *t)
{
	size_t len = strlen(t);
	char *x;
	unsigned n;
	int i;
	if (len < 6 || strcmp(t + len - 6, "XXXXXX") != 0) {
		errno = EINVAL;
		return NULL;
	}
	x = t + len - 6;
	n = ++tmpid + (unsigned)getpid() * 7919u;
	for (i = 5; i >= 0; i--) {
		x[i] = digs[n % 36];
		n /= 36;
	}
	return x;
}

/* Give up after this many names already taken: the directory is full of our
   candidates, or it is not EEXIST that keeps failing. */
#define TMP_ATTEMPTS 1000

int mkstemp(char *t)
{
	char *x;
	int attempt;
	if (!(x = next_name(t)))
		return -1;
	for (attempt = 0; attempt < TMP_ATTEMPTS; attempt++) {
		int fd = open(t, O_RDWR | O_EXCL | O_CREAT, 0600);
		if (fd >= 0)
			return fd;
		if (errno != EEXIST)
			break;
		memcpy(x, "XXXXXX", 6);
		next_name(t);
	}
	memcpy(x, "XXXXXX", 6);
	return -1;
}

char *mkdtemp(char *t)
{
	char *x;
	int attempt;
	if (!(x = next_name(t)))
		return NULL;
	for (attempt = 0; attempt < TMP_ATTEMPTS; attempt++) {
		if (mkdir(t, 0700) == 0)
			return t;
		if (errno != EEXIST)
			break;
		memcpy(x, "XXXXXX", 6);
		next_name(t);
	}
	memcpy(x, "XXXXXX", 6);
	return NULL;
}
