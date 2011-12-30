#define _ISOC99_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>

#include <errno.h>

#include <termios.h> /* Terminal stuff */

#include "consoler.h"
#include "../util/util.h"
#include "../tokenizer/tokenizer.h"



int raw (int fd, struct termios *new_io, struct termios *old_io);
int getchar_wrapper(struct buffer *stdinbuf, int semid);





/* Idea from http://www.pronix.de */
int raw (int fd, struct termios *new_io, struct termios *old_io) {
	/*Sichern unseres Terminals*/
	if ((tcgetattr (fd, old_io)) == -1)
		return -1;
	memcpy(new_io, old_io, sizeof(struct termios));

	/*Wir verändern jetzt die Flags für den raw-Modus*/
	new_io->c_iflag =
		new_io->c_iflag & ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	new_io->c_oflag = new_io->c_iflag & ~(OPOST);
	new_io->c_cflag = new_io->c_cflag & ~(CSIZE | PARENB);
	new_io->c_lflag = new_io->c_lflag & ~(ECHO|ICANON|IEXTEN|ISIG);
	new_io->c_cflag = new_io->c_cflag | CS8;
	new_io->c_cc[VMIN]  = 1;
	new_io->c_cc[VTIME] = 0;
  

	/*Jetzt setzten wir den raw-Modus*/
	if ((tcsetattr (fd, TCSAFLUSH, new_io)) == -1)
		return -1;
	return 0;
}



int getcharWrapper(struct buffer *stdinbuf) {
	char c;
	char esc[5];
	int r;
	
	
	
	/* c = getchar(); */ /* getchar is evil ! (buffered) */
	if ((r = read(STDIN_FILENO, &c, 1)) != 1) {
		return -1;
	}
	
	if(c == 27) { /* escape */
		/* Special characters (like arrow keys) return the escape-
		 * character (27) with up to 4 more chars. See 
		 * http://en.wikipedia.org/wiki/ANSI_escape_code
		 * for details. */
		if((r = read(STDIN_FILENO, &esc, 4)) < 0) return -1;
		else esc[r] = 0; /* null-terminate */
		
		/* Arrow keys (as an example) */
		if(strcmp(esc, "[A") == 0) return 2;
		else if(strcmp(esc, "[B") == 0) return 3;
		else if(strcmp(esc, "[C") == 0) return 4;
		else if(strcmp(esc, "[D") == 0) return 5;
		else { /* ignore everything unknown*/
		}
	} else if (c == 13) { /* Return */
		stdinbuf->buf[stdinbuf->buflen] = '\n';
		stdinbuf->buflen++;
		putchar('\r');
		putchar('\n');
		fflush(stdout);
		return 1;
	} else if (c == 127) { /* Backspace */
		if(stdinbuf->buflen > 0) {
			stdinbuf->buflen--;
			stdinbuf->buf[stdinbuf->buflen] = 0;
			printf("\r>> %s \b", stdinbuf->buf);
			fflush(stdout);
		} else {
			printf("\a");
		}
	} else if (c >= 0x20) { /* ASCII: Control characters up to 0x1F */
		stdinbuf->buf[stdinbuf->buflen] = c;
		stdinbuf->buflen++;
		putchar(c);
		fflush(stdout);
	}
	
	return 0;
}






/* Consoler
 * A fork for handling the console.
 * This is neccessary, because we want to write stuff to the console
 * from many child processes and we want a prompt for entering
 * commands. This means we need to remove the actual prompt
 * including all typed characters before we print a message, then
 * print a message (a whole line) and finally restore the prompt as it
 * was originally.
 * 
 *                                 
 */
int consoler(int infd, int outfd) {
	struct buffer stdinbuf;
	struct buffer stdoutbuf;
	struct buffer stdoutline;
	int gwret, gtfbret, readret;
	int ret = 1;
	struct termios new_io, old_io;

	/* poll Stuff */
	struct pollfd pollfds[2];
	int pollret;

	
	/* Set Terminal raw */
	raw(STDIN_FILENO, &new_io, &old_io);

	
	createBuf(&stdoutbuf, 16384);
	createBuf(&stdinbuf, 16384);
	createBuf(&stdoutline, 1024);
	
	
	
	/* Setting up stuff for Polling */
	pollfds[0].fd = infd;
	pollfds[0].events = POLLIN | POLLHUP;
	/*pollfds[0].revents = 0;*/
	pollfds[1].fd = STDIN_FILENO;
	pollfds[1].events = POLLIN;
	/*pollfds[1].revents = 0;*/


	/* initial Prompt */
	writef(STDOUT_FILENO, "\r>> ");

	
	while(1) {

		pollret = poll(pollfds, 2, -1);
		
		if(pollret < 0 || pollret == 0) {
			if(errno == EINTR) continue; /* Signals */
			
			printf("POLLING Error in Consoler.\r\n");
			ret = -1;
			break;
		}
		
		
		
		if(pollfds[0].revents & POLLIN) {
			/* Stuff for STDOUT came thorugh infd-pipe */
			
	
			if((readret = readToBuf(infd, &stdoutbuf)) < 0) {
				ret = -1;
				break;
			} else if (readret == 0) {
				ret = 0;
				break;
			}
			
			
			/* remove prompt */
			writef(STDOUT_FILENO, "\r%*s\r",
				stdinbuf.buflen + 3, "");
			
			/* Get all lines and print them */
			while((gtfbret = getTokenFromStreamBuffer(&stdoutbuf,
					&stdoutline, "\r\n", "\n",
					(char *)NULL)) > 0) {
				writeBuf(STDOUT_FILENO, &stdoutline);
				writef(STDOUT_FILENO, "\r\n");
			}
			if(gtfbret < 0) {
				ret = -1;
				break;
			}
			
			/* restore prompt */
			writef(STDOUT_FILENO, "\r>> %.*s", stdinbuf.buflen,
				stdinbuf.buf);
		}
		else if(pollfds[1].revents & POLLIN) {
			/* Stuff from STDIN wants to go to outfd */
			
			gwret = getcharWrapper(&stdinbuf);
			if (gwret == 1) { /* New Line ! */
				writef(STDOUT_FILENO, "\r>> ");
				if(writeBuf(outfd, &stdinbuf) < 0) {
					/* probably other side of pipe
					 * was closed */
					ret = -1;
					break;
				}
			} else if (gwret < 0) {
				flushBuf(&stdinbuf);
			}
		} else if(pollfds[0].revents & POLLHUP) {
			/* All write-ends of pipe are closed. */
			break;
		} else {
			/* this shouldn't happen */
			printf("\r\nUnhandled poll-behaviour in consoler\r\n");
			break;
		}
	}
	
	
	close(infd);
	close(outfd);
	
	/* Prompt vernichten */
	writef(STDOUT_FILENO, "\r%*s\r", stdinbuf.buflen + 3, "");
	
	/* Reset terminal to previous state */
	tcsetattr (STDIN_FILENO, TCSAFLUSH, &old_io);
	
	
	freeBuf(&stdinbuf);
	freeBuf(&stdoutbuf);
	freeBuf(&stdoutline);
	
	return ret;
}






int consolemsg(int semid, int pipefd, const char *fmt, ...) {
	char *p;
	va_list ap;
	
	
	va_start(ap, fmt);
	p = vStringBuilder(fmt, ap);
	va_end(ap);

	
	//if(semWait(semid, SEM_CONSOLER) == -1) return -1;
	
	if(writeWrapper(pipefd, p, strlen(p)) < 0) return -1;

	//if(semSignal(semid, SEM_CONSOLER) == -1) return -1;
	
	free(p);
	return 1;
}




