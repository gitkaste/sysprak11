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


/* ------- SWITCHES ------- */
/* Uncomment this if you have a kernel >= 2.6.22 but a glibc < 2.8 */
/*#define _SIGFD_KERNELSUPPORT*/

/* Uncomment this if your kernel is < 2.6.22 */
/*#define _SIGFD_NOSUPPORT*/



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



#if defined (_SIGFD_KERNELSUPPORT) || defined (_SIGFD_NOSUPPORT)

#include <stdint.h>


/* The struct needed by signalfd */
struct signalfd_siginfo {
    uint32_t  ssi_signo;
    int32_t   ssi_errno;
    int32_t   ssi_code;
    uint32_t  ssi_pid;
    uint32_t  ssi_uid;
    int32_t   ssi_fd;
    uint32_t  ssi_tid;
    uint32_t  ssi_band;
    uint32_t  ssi_overrun;
    uint32_t  ssi_trapno;
    int32_t   ssi_status;
    int32_t   ssi_int;
    uint64_t  ssi_ptr;
    uint64_t  ssi_utime;
    uint64_t  ssi_stime;
    uint64_t  ssi_addr;
    uint8_t  __pad[48];

};



/* NOTE: check http://www.xmailserver.org/signafd-test.c 
 * it actually seems to work with 321 on 64-bit as well. */
/*
 * These were good at the time of 2.6.22-rc3 ...
 */
/*#ifndef __NR_signalfd
#if defined(__x86_64__)
#define __NR_signalfd 282
#elif defined(__i386__)
#define __NR_signalfd 321
#else
#error Cannot detect your architecture!
#endif
#endif*/


/* The number of the syscall signalfd */
#if defined(__i386__)
#define __NR_signalfd 321
#endif

/* This is our pseudo-implementation of signalfd.
*/
int signalfd(int ufd, sigset_t const *mask, int flags);

#else



/* If we have support for signalfd's, we can just include the standard-header */
#include<sys/signalfd.h>

#endif



#endif

