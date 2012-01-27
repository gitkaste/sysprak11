#include <inttypes.h> /* uintX_t */
#include <stdio.h> /* FILENAME_MAX */
#include <netinet/in.h> /* struct in_addr */
#include <sys/socket.h> 
#include <sys/types.h> 
#ifndef _CONFIG_H
#define _CONFIG_H
#define SCHEDULER_RR 1
#define SCHEDULER_PRI 2

/* struct config
 * configuration structure for _both_, server and client (merged). */
struct config {
	/* unified for both v4 and v6, port is still used extra though for convenience */
    struct sockaddr  ip;
		uint16_t         port;
    uint8_t          loglevel;
		uint16_t         logMask;
    char             logfile[FILENAME_MAX];
		char             networkDumpLogFile[FILENAME_MAX];
    char             share[FILENAME_MAX];
    char             workdir[FILENAME_MAX];
    uint32_t         shm_size;
		uint8_t          forceIpVersion;
		struct sockaddr  bc_ip;
		uint16_t         bc_port;
		struct sockaddr  bc_broadcast;
		uint16_t         bc_interval;
		char             scheduler;
		uint16_t         schedTimeSlice;
};
extern struct config * conf;

void confDefaults(struct config *conf);
int parseConfig (int conffd, struct config *conf);
void writeConfig (int fd, struct config *conf);
int initConf(char * conffilename, struct config *conf, char error[256]);
#endif 
