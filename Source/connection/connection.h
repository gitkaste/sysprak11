#include <inttypes.h>
#include <protocol.h>
int createPassiveSocket(uint16_t *port);
int connectSocket(struct in_addr *ip, uint16_t port);
int sendResult(int fd, struct actionParameters *ap, struct serverActionParameters *sap);
int recvResult(int fd, struct actionParameters *ap,struct array * results);
