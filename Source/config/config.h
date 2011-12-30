#include <inttypes.h> /* uintX_t */
#include <stdio.h> /* FILENAME_MAX */
#include <netinet/in.h> /* struct in_addr */

/* struct config
 * configuration structure for _both_, server and client (merged). */
struct config {
    struct in_addr   ip;
    uint16_t         port;
    char             logfile[FILENAME_MAX];
    uint8_t          loglevel;
    char             share[FILENAME_MAX];
    uint32_t         shm_size;
    /* ... */
};

void confDefaults(struct config *conf);
int parseConfig (int conffd, struct config *conf);
int writeConfig (int fd, struct config *conf);
