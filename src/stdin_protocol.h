#include "protocol.h"

/* Initialisiert das Protokoll */
int initializeStdinProtocol(struct actionParameters *ap);

/* This is the default action 
 * The input will just be passed on to the server, please note, that 
 * the input may need to be reconstructed  */
int passOnAction(struct actionParameters *ap, 
		union additionalActionParameters *aap);

/* Prints out the progress from all active down- and upload progresses. */
int stdin_showAction(struct actionParameters *ap, 
		union additionalActionParameters *aap);

/* This action prints out the results from a previous search run*/
int stdin_resultsAction(struct actionParameters *ap, 
		union additionalActionParameters *aap);

/* This routine closes the com socket from the client side without sending
 * the server a QUIT action. */
int stdin_exitAction(struct actionParameters *ap, 
		union additionalActionParameters *aap);

/* This action shows the help text of all registered actions of the protocol
 * and then passes the command on to the server for it to do the same */
int stdin_helpAction(struct actionParameters *ap, 
		union additionalActionParameters *aap);

/* This action is called by entering DOWNLOAD <no>, where <no> is the number of
 * the array element of the search result set which is printed upon return */
int stdin_downloadAction(struct actionParameters *ap, 
		union additionalActionParameters *aap);
