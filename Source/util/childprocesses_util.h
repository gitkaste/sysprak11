#define CHLDPROC_LOGGER 1
#define CHLDPROC_CONSOLER 2
#define CHLDPROC_SENDLIST 3
#define CHLDPROC_RESULT 4
#define CHLDPROC_DOWNLOAD 5
#define CHLDPROC_UPLOAD 6


/* struct childProcess
 * type shall be one of the defines above.
*/
struct childProcess {
	unsigned char type;
	pid_t pid;
};


/* Functions for managing child processes (utilizing arrays)
 * These are similiar to the array functions, except the are specifically
 * tailored for struct childProcess items.
*/
int addChildProcess(struct array *cpa, unsigned char type, pid_t pid);
int remChildProcess(struct array *cpa, pid_t pid);

/* sendSignalToChildren
 * This function can send signals to every child of type type.
*/
int sendSignalToChildren(struct array *cpa, unsigned char type, int sig);
