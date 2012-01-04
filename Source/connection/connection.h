#include <inttypes.h>
int createPassiveSocket(uint16_t *port);
int connectSocket(struct in_addr *ip, uint16_t port);
