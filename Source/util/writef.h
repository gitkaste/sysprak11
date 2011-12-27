#include <sys/types.h>
#include <stdarg.h>

ssize_t writef (int fd, char *fmt, ...);
char *stringBuilder (const char *fmt, ...);
char *vStringBuilder (const char *fmt, va_list ap);
