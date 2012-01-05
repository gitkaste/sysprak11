/* sys/signalfd.h is provided in glibc-2.8. The following code cares about
 * enabling a similar behaviour on systems with an older glibc or even a
 * kernel taht doesn't support the signalfd()-syscall (<2.6.22) */
/* Some of this stuff is taken from a mail from Michael Kerrisk
 * http://groups.google.com/group/linux.kernel/msg/9397cab8551e3123
 * probalbly on the lkml
 * 
 */
#ifndef _LINUX_SIGNALFD_H_PRE28
#define _LINUX_SIGNALFD_H_PRE28

/* POSIX and GNU are also needed for syscall */
/* sigset_t is a POSIX-type */
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif

/* This is needed for siginfo_t (NOSUPPORT-case) */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <signal.h> /* sigset_t ? */

/* getSigfd
 * This function expects a sigset of signals, which shall be caught by a
 * signalfd. It cares about blocking the signals and returns the new fd
 * returned by a signalfd()-call.
 * If there is no kernel/glibc-suport for signalfd(), it still returns a
 * fd that is useable like a signalfd()-fd. There are small differences if
 * the signalfd-emulation via a signal handler is turned on - in this case
 * it's not possible to simply block the signals. That means, certain blocking
 * calls (read, write, accept, poll) will be interrupted and return with -1
 * and errno == EINTR. The code using this should be able to handle such
 * behaviour.
 * On another note, blocking calls (except poll) are evil in a signalfd case
 * anyway, as signals should be processed ASAP. That's why most fd's should
 * be set nonblocking - this makes it possible, that normally blocking calls
 * (write, read) return with -1 and errno == EAGAIN. poll isn't affected of
 * course.
 * Return values are: -1 on error, the new signalfd on success.
*/
int getSigfd(const sigset_t *mask);
#include<sys/signalfd.h>
#endif

