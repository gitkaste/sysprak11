/* sys/signalfd.h is provided in glibc-2.8. The following stuff enables it on
 * kernels >= 2.6.22 for older glibc's */
/* This stuff is taken from a mail from Michael Kerrisk
 * http://groups.google.com/group/linux.kernel/msg/9397cab8551e3123
 * probalbly on the lkml
 * 
 */
#include "signalfd.h" /* signalfd.h already takes care about signal.h */
#include <unistd.h>   /* NULL ... */
#include <sys/signalfd.h>

int getSigfd(const sigset_t *mask) {
	return(sigprocmask(SIG_BLOCK, mask, NULL) == -1)? -2: signalfd(-1, mask, 0);
}
