/* struct flEntry
 * Represents a file in the network.
*/
struct flEntry {
	struct in_addr ip;
	uint16_t port;
	char filename[FILENAME_MAX];
	unsigned long size;
};
