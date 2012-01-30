#ifndef _protocol_h
#define _protocol_h

#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "util.h"
#include "config.h"
#include "directoryParser.h"

/* Defines for used resources */
/* ap */
#define APRES_SIGFD 1             /* Bit  0 */
#define APRES_LOGFD 2             /* Bit  1 */
#define APRES_SEMID 4             /* Bit  2 */
/*#define SERVERRES_PROTOCOL 8*/    /* Bit  3 - neccessary only if malloced */
#define APRES_COMFD 16            /* Bit  4 */
#define APRES_COMBUF 32           /* Bit  5 */
#define APRES_COMLINE 64          /* Bit  6 */
#define APRES_COMWORD 128         /* Bit  7 */

/* sap */
#define SAPRES_FILELIST 1         /* Bit  0 */
#define SAPRES_FILELISTSHMID 2    /* Bit  1 */

/* cap */
#define CAPRES_OUTFD 1             /* Bit 0 */
#define CAPRES_RESULTSSHMID 2      /* Bit 1 */
#define CAPRES_RESULTS 4           /* Bit 2 */
#define CAPRES_SERVERFD 8          /* Bit 3 */
#define CAPRES_CPA 16              /* Bit 4 */

/* define return codes for protocol */
#define REP_OK 100 /* This was intended to be sent without any text */
#define REP_TEXT 200
#define REP_COMMAND 300
#define REP_WARN 400
#define REP_FATAL 500

/* struct actionParameters
 * This is the structure holding the mainly used resources used in actions. 
 * It contains the communication filedescriptor (One for sockets, two for
 * pipes), logger and semaphore information (logfd and semid), the signal
 * filedescriptor (sigfd returned by signalfd()), several buffers for incoming
 * data and tokens, a pointer to the protocol structure used for the actions
 * and the remote ip/port (this may be unused by a protocol).
 * 
*/
struct actionParameters { /* elwms */
	uint32_t usedres; /* Used Resources */
	int comfd;
	int semid;
	int logfd;
	pid_t logpid;
	int sigfd;
	struct config *conf;
	
	struct buffer combuf;
	struct buffer comline;
	struct buffer comword;
	
	/* This is the protocol used on comfd and its buffers */
	struct protocol *prot;
	/* ip of communication partner ('peer') */
	struct sockaddr_storage comip;
	uint16_t comport;
};


/* serverActionPrameters
 * Additional action parameters for the server. Contains the shared mem for
 * the filelist.
*/
struct serverActionParameters {
	uint32_t usedres; /* Used Resources */
	int shmid_filelist;
	struct array *filelist; /* Filelist Array, Elements are flEntries */
};

/* clientActionParameters
 * Additional action parameters used by the client. Contains consoler,
 * shared mem for results, serverfd (used by stdin-protocol as it's different
 * from its comfd) and the child process array, an array to track all spawned
 * childs of the client.
*/
struct clientActionParameters {
	uint32_t usedres; /* Used Resources */
	int outfd; /* Consoler */
	int infd; /* Consoler */
	pid_t conpid;
	int shmid_results;
	struct array *results;
	/* server connection */
	int serverfd; /* Socket to server
			(needed for stdin-protocol and its passOnAction) */
	
	/* This is the Child-Process-Array. Needed in some
	 * stdin-protocol-actions. */
	struct array *cpa;
};

/* Just a union to make universal actions possible */
union additionalActionParameters {
	struct serverActionParameters *sap;
	struct clientActionParameters *cap;
};

/* typedef for a pointer to an action */
typedef int(*action) (struct actionParameters *ap,
			union additionalActionParameters *aap);

/* struct action
 * each action has a name and a description. 
*/
struct action {
	char actionName[16];
	char description[1024];
	action actionPtr;
};

/* struct protocol
 * collects all actions used by a protocol 
*/
struct protocol {
	action defaultAction;
	int actionCount;
	struct action actions[32]; /* maximum of 32 actions supported */
	/*struct action *actions;*/ /* this only works for dynamically
					allocated memory */
};

/* processIncomingData
 * This function shall be called when there is data on an fd which is
 * used by the protocol.
 * It does read from the fd, tries to get (all) tokens (that means lines here)
 * out of the data and processes them through processCommand.
 * Return Values are:
 *    -3: returning child process (error)
 *    -2: returning child process (success)
 *    -1: Fatal error. (abort and exit)
 *     0: ap->comfd hung up / closed from the other side (clean exit)
 *    +x: everything okay, continue (usually 1)
 * Note: Return Values blend with the return values of actions (see note below).
 *       Therefore it's okay to return the value returned by processCommand.
 * Note2: gtfsret can never be < 0 ...
 * 
 */
int processIncomingData(struct actionParameters *ap,
		union additionalActionParameters *aap);

/* processCommand
 * This function excepts a tokenized line in ap->comline which it parses into
 * a word (comword) and finds the corresponding action via validateToken
 * (if no action is found, the defaultAction is used). It's mostly used by
 * processIncomingData(). 
 * Return values are the same as those for actions plus:
 *  2 - line was empty (just continue)
*/
int processCommand(struct actionParameters *ap,
		union additionalActionParameters *aap);

/* validateToken
 * This function takes a token and searches a protocol structure for
 * a corresponding action (actionName). If no matching action is found,
 * it uses the defaultAction defined in the protocol structure.
 * Returns a pointer to the action function.
*/
action validateToken(struct buffer *token, struct protocol *prot);

/* reply
 * sends a reply to the client (including a numerical code)
*/
int reply(int comfd, int logfd, int semid, int code, const char *msg);
int initap(struct actionParameters *ap, char error[256], int semcount);
void freeap(struct actionParameters *ap);
int recvFileList(int sfd, struct actionParameters *ap,
		struct serverActionParameters *sap);

/* *************************************************************************** *
 *                                                                             *
 *        A  C  T  I  O  N  S                                                  *
 *                                                                             *
 * ****************************************************************************/
/* Actions process one protocol command.
 * 
*/
/* Note on Return Value of Actions:
 *        1 - everything okay, continue
 *        0 - clean exit
 *       -1 - abort and exit
 *       -2 - child: cleanup and exit ???
 *       -3 - child: cleanup and exit with error
*/

#endif
