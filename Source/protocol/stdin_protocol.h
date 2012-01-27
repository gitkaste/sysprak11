#include "protocol.h"

/* Initialisiert das Protokoll */
int initializeStdinProtocol(struct actionParameters *ap);

/* Dies ist die Standaraktion defaultAction im Rahmen der Spezifikation des 
 * Protokolls. Die Eingabe wird einfach an den Server weitergschickt. Beachten 
 * Sie, dass Sie die Eingabe unter Umständen rekonstruieren müssen. */
int passOnAction(struct actionParameters *ap, 
		union additionalActionParameters *aap);

/* Prints out the progress from all active down- and upload progresses. */
int stdin_showAction(struct actionParameters *ap, 
		union additionalActionParameters *aap);

/* Diese Funktion gibt die aktuellen Resultate einer Suche aus dem results-Array
 * aus.*/
int stdin_resultsAction(struct actionParameters *ap, 
		union additionalActionParameters *aap);

/* Diese Routine schließt das Kommunikations-Socket von Client-Seite aus, 
 * ohne dem Server ein QUIT- Kommando zu senden. */
int stdin_exitAction(struct actionParameters *ap, 
		union additionalActionParameters *aap);

/* Diese Funktion zeigt, die Hilfetexte aller spezifizierten Aktionen des 
 * Protokolls an und sendet danach das HELP-Kommando an den Server, um auch 
 * dessen Hilfetexte auszugeben.*/
int stdin_helpAction(struct actionParameters *ap, 
		union additionalActionParameters *aap);

/* ... wird durch Eingabe von DOWNLOAD <nr> aufgerufen. Das Argument <nr> 
 * bezeichnet dabei die Nummer des Array-Elements der Resultate (Erinnerung:
 * diese wird bei Resultatausgabe mit ausgegeben). */
int stdin_downloadAction(struct actionParameters *ap, 
		union additionalActionParameters *aap);
