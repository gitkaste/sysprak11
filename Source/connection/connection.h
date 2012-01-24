#include <inttypes.h>
#include <protocol.h>
int createPassiveSocket(uint16_t *port);
int connectSocket(struct sockaddr *ip, uint16_t port);
int sendResult(int fd, struct actionParameters *ap, struct serverActionParameters *sap);
int recvResult(int fd, struct actionParameters *ap, struct array * results);
int recvFileList(int sfd, struct actionParameters *ap, struct serverActionParameters *sap);
int handleUpload(int upfd, int confd, struct actionParameters *ap);
int advFileCopy(int destfd, int srcfd, unsigned long size, const char *name, int semid, int logfd, int sigfd, int confd);
