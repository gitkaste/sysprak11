/* sys/signalfd.h is provided in glibc-2.8. The following stuff enables it on
 * kernels >= 2.6.22 for older glibc's */
/* This stuff is taken from a mail from Michael Kerrisk
 * http://groups.google.com/group/linux.kernel/msg/9397cab8551e3123
 * probalbly on the lkml
 * 
 */
#include "signalfd.h"
/* signalfd.h already takes care about signal.h */


/* --------------- KERNEL >= 2.6.22, glibc < 2.8 --------------- */
#if defined _SIGFD_KERNELSUPPORT



#include <sys/syscall.h>
#include <unistd.h>



int signalfd(int ufd, sigset_t const *mask, int flags) {
#define SIZEOF_SIG (_NSIG / 8)
#define SIZEOF_SIGSET (SIZEOF_SIG > sizeof(sigset_t) ? \
                       sizeof(sigset_t): SIZEOF_SIG)
	return syscall(__NR_signalfd, ufd, mask, SIZEOF_SIGSET);

}




int getSigfd(const sigset_t *mask) {
	
	
	if(sigprocmask(SIG_BLOCK, mask, NULL) == -1) {
		return -2;
	}
	
	return signalfd(-1, mask, 0);

}



/* --------------- KERNEL < 2.6.22 - no support --------------- */
#elif defined _SIGFD_NOSUPPORT


#include <unistd.h>
#include <string.h> /* memcpy() */



/* This is a pseudo-implementation for signalfd. 
 * This is just a make-it-work approach, it certainly has a lot of
 * problems (like the pipe-buffer getting full for many signals). 
 * Limitations are: - just one fd per process
 *                  - pipe won't be closed after fork (writing-end)
 *                  - struct signalfd_siginfo is just partly filled
 *                  - Signals are not blocked - they may interrupt
 *                        system calls although we set SA_RESTART
 *                        (esp. poll())
 *                  - signalhandler won't be removed after forking
 *                      (WARNING: This might be quite a problem !)
 *                  - ...
 *
 *   */

/* This is the pipe, sighandler uses. -1 means it isn't initialized.
 * NOTE: With this implementation it's not possible to use more than ONE fd for
 * handling signals. */
static int signalfdpipe = -1;



/* The signal handler that catches signals, fills information into the
 * signalfd_siginfo structure and writes it into the pipe */
void signalfd_sighandler(int signo, siginfo_t *info, void *context) {
	int written, wret;
	struct signalfd_siginfo ssi;
	
	/* Filling in the stuff we can get from info
	 * (this is evil and error-prone) */
	ssi.ssi_signo = signo;
	ssi.ssi_errno = info->si_errno;
	ssi.ssi_code = info->si_code;
	ssi.ssi_pid = (uint32_t)info->si_pid;
	ssi.ssi_uid = info->si_uid;
	ssi.ssi_status = info->si_status;
	ssi.ssi_band = info->si_band;
	/*ssi.ssi_fd = info->si_fd;
	ssi.ssi_tid = info->si_tid;
	ssi.ssi_overrun = info->si_overrun;
	ssi.ssi_trapno = info->si_trapno;
	ssi.ssi_int = info->si_int;
	ssi.ssi_ptr = info->si_ptr;
	ssi.ssi_utime = info->si_utime;
	ssi.ssi_stime = info->si_stime;
	ssi.ssi_addr = info->si_addr;*/
	
	written = 0;
	
	while(written < sizeof(struct signalfd_siginfo)) {
		if((wret = write(signalfdpipe, &ssi + written,
				sizeof(struct signalfd_siginfo) - written)) < 0)
			break;
		written += wret;
	}
}



/* We may not mask any signals in this getSigfd, as they have to be caught by
 * our signal-handler */
int getSigfd(const sigset_t *mask) {
	return signalfd(-1, mask, 0);
}



/* This setups the pipe and the signalhandler.
 * SA_RESTART is set to achieve a bahaviour as close as possible to the
 * signalfd()-syscall. This isn't 100% though (check comments below)
*/
int signalfd(int ufd, sigset_t const *mask, int flags) {
	int pfd[2];
	struct sigaction sa;
	int signum;
	
	
	/* everything else is unsupported */
	if(ufd != -1) return -1;
	
	if(pipe(pfd) == -1) return -1;
	
	if(signalfdpipe != -1) close(signalfdpipe);
	signalfdpipe = pfd[1];
	
	/* SA_RESTART restarts *some* system calls. This is as close as possible
	 * to blocking signals & signalfd.
	 * WARNING: SA_RESTART does not "work" on all system calls. For a list
	 * see man 7 signal (man-pages v3.11+) or
	 * http://www.kernel.org/doc/man-pages/online/pages/man7/signal.7.html
	 * Especially poll() won't be restarted !
	 * SA_SIGINFO tells sigaction to use sa_sigaction instead of sa_handler
	 * (we need this to fill the structures we send through the pipe) */
	sa.sa_flags = SA_RESTART | SA_SIGINFO;
	memcpy(&sa.sa_mask, mask, sizeof(sigset_t));
	sa.sa_handler = NULL;
	sa.sa_sigaction = signalfd_sighandler;
	
	/* According to man 7 signal there are signals from 1 to 29.
	 * Number 28 doesn't exist, but we silently ignore this ;) 
	 * UPDATE: There seem to be at least 31 (arch specific) */
	for(signum = 1; signum <= 31; signum++) {
		if(sigismember(mask, signum) == 1)
			sigaction(signum, &sa, NULL);
	}
	
	
	return pfd[0];
}


#else


/* --------------- FULL SUPPORT --------------- */


#include <unistd.h> /* NULL ... */
#include <sys/signalfd.h>



int getSigfd(const sigset_t *mask) {
	
	
	if(sigprocmask(SIG_BLOCK, mask, NULL) == -1) {
		return -2;
	}
	
	return signalfd(-1, mask, 0);

}



#endif





