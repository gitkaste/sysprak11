/*******************************************************************************
*                                                                              *
*        A  C  T  I  O  N  S                                                   *
*                                                                              *
*******************************************************************************/

/* Note on Return Value of Actions:
 *        1 - everything okay, continue
 *        0 - clean exit (hangup, quit)
 *       -1 - abort and exit
 *       -2 - child (ok): cleanup and exit
 *       -3 - child (error): cleanup and exit
 */
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE /* signal.h sigset_t */
#endif


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/wait.h> /* for signalfd (Signalhandler) */
#include <signal.h>
#include <poll.h> /* poll() */

/* for inet_ntoa(). Probably this is just for some very verbose log-messages. */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <inttypes.h>
#include <errno.h>

#include "protocol.h"
#include "server_protocol.h"
#include "../util/util.h"
#include "../logger/logger.h"
#include "../tokenizer/tokenizer.h"
#include "../connection/connection.h"
#include "../directoryParser/directoryParser.h"

#include "../util/writef.h"
/*#include "../util/signalfd.h"*/




struct protocol server_protocol = {
	&unknownCommandAction,
	3,
	{
		{"STATUS", "STATUS returns Server-Status.\n", &statusAction},
		{"QUIT", "QUIT closes the connection cleanly (probably).\n",
			&quitAction},
		{"HELP", "HELP prints this help.\n", &helpAction}
	}
};






int initializeServerProtocol(struct actionParameters *ap) {
	
	ap->prot = &server_protocol;
	
	
	
	return 1;
}




int unknownCommandAction(struct actionParameters *ap,
		union additionalActionParameters *aap) {
	char *msg;
	
	
	msg = stringBuilder("Command \"%s\" not understood.\n",
		ap->comword.buf);
	reply(ap->comfd, ap->logfd, ap->semid, REP_WARN, msg);
	free(msg);
	
	return 1;
}



int statusAction(struct actionParameters *ap,
		union additionalActionParameters *aap) {
	
	<insert code here>
	
	return 1;
}



int quitAction(struct actionParameters *ap,
		union additionalActionParameters *aap) {
	reply(ap->comfd, ap->logfd, ap->semid, REP_TEXT,
		"Closing connection. Have a nice day ;)\n");
	
	/* Returning 0: main-loop shall break and shouldn't accept any
	 * further commands */
	return 0;
}



int helpAction(struct actionParameters *ap,
		union additionalActionParameters *aap) {
	
	<insert code here>
	
	return 1;
}
