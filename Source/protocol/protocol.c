#define _GNU_SOURCE /* sigset_t, POLLIN, POLL*, SIG*, esp. POLLRDHUP */


#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h> /* strcasecmp() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <inttypes.h>
#include <errno.h>

#include "protocol.h"
#include "../util/util.h"
#include "../logger/logger.h"
#include "../tokenizer/tokenizer.h"




/* processIncomingData shall be called when there is Data on an fd which is
 * processed by a protocol.
 * It does read from the fd, tries to get (all) tokens (that means lines here)
 * out of the data an processes them through processCommand.
 * Return Values are:
 *    -3: returning child process (error)
 *    -2: returning child process (success)
 *    -1: Fatal error. (abort and exit)
 *     0: ap->comfd hung up / closed from the other side (clean exit)
 *    +x: everything okay, continue
 * Note: Return Values blend with the return values of actions (see note below).
 *       Therefore it's okay to return the value returned by processCommand.
 * Note2: gtfsret can never be < 0 ...
 * 
 */
int processIncomingData(struct actionParameters *ap,
			 union additionalActionParameters *aap) {
	int gtfsret, pcret, readret;
	
	
	/* read once - poll told us there is data */
	if((readret = readToBuf(ap->comfd, &ap->combuf)) == -1) {
		logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL,
			"FATAL (processIncomingData): read()-error from fd.\n");
		return -1;
	} else if (readret == -2) {
		logmsg(ap->semid, ap->logfd, LOGLEVEL_WARN,
			"WARNING (processIncomingData): couldn't "
			"read (EINTR or EAGAIN). This shouldn't happen "
			"if signalfd() is used.\n");
		return 1;
	} else if (readret == 0) { /* might also throw a POLLHUP */
		logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE,
			"(processIncomingData) ap->comfd "
			"closed from other side.\n");
		return 0;
	}
	
	
	/* tokenize all lines recieved and process them */
	while((gtfsret = getTokenFromStreamBuffer(&ap->combuf,
			&ap->comline, "\r\n", "\n", (char *)NULL)) > 0) {
		if((pcret = processCommand(ap, aap)) <= 0) return pcret;
		/* NOTE: Remaining content in comline will be overwritten
		 * by getTokenFrom*(). */
	}
	if(gtfsret < 0) { /* token buffer too small */
		return gtfsret;
	}
	
	return 1;
}



/* processCommand shall be called, when a new command (line/token) came through
 * ap->comfd. It tries to find the first "word" in the line, feeds it to
 * validateToken and run the action returned by validateToken.
 * Return Value:
 *    2: if no "word" was found inside the line (means line was empty)
 *       this is positive, because we just want to ignore it.
 *    otherwise: return value of the action called
 *               (see note on return values of actions below)
 * 
 */
int processCommand(struct actionParameters *ap,
		    union additionalActionParameters *aap) {
	int gtfbret;
	
	
	logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE,
		"COMRECV: '%.*s'\n", ap->comline.buflen, ap->comline.buf);
	
	/* get the first word of the line */
	gtfbret = getTokenFromBuffer(&ap->comline,
					&ap->comword, " ", "\t", (char *)NULL);
	
	if(gtfbret < 0) {
		logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, "FATAL: "
			"getTokenFromBuffer() failed in processCommand.\n");
		return gtfbret;
	}
	else if (gtfbret == 0) return 2; /* command was empty */
	
	
	/* find (validate) and run the action and return its return value */
	return (*validateToken(&ap->comword, ap->prot)) (ap, aap);
}



int reply(int comfd, int logfd, int semid, int code, const char *msg) {
	logmsg(semid, logfd, LOGLEVEL_DEBUG, "SEND: %3d %s", code, msg);
	return writef(comfd, "%3d %s", code, msg);
}



action validateToken(struct buffer *token, struct protocol *prot) {
	int i;
	
	
	for(i = 0; i < prot->actionCount; i++) {
		if(strcasecmp(prot->actions[i].actionName, token->buf) == 0)
			return prot->actions[i].actionPtr;
	}
	
	return prot->defaultAction;
}




