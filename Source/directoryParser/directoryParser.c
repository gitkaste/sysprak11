#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "directoryParser.h"
#include "util.h"

char * path_join(const char * basedir, const char * subdir){
	char * fulldirname = malloc(strlen(basedir) + strlen(subdir) +2);
	if (!fulldirname) return NULL;
	return strcat(strcat(strcpy(fulldirname,basedir),"/"), subdir);
}

int parseDirToFD(int fd, const char * basedir, const char * subdir){

	/* construct full dir name */
	char * fulldirname = path_join(basedir, subdir);
	DIR * dir = opendir(fulldirname);
	if (!dir) {
		free(fulldirname);
		return -1;
	}
	struct dirent * entry;
	struct stat statentry;
	char * entrypath;
	errno = 0;
	while ( (entry = readdir(dir)) != NULL ){
		/* ignore hidden files/dirs */
		if (entry->d_name[0] == '.') continue;

		entrypath = path_join(fulldirname, entry->d_name);
		if ( !(entrypath) ){
			free(fulldirname);
			return -1;
		}
	 if ( stat(entrypath,&statentry) == -1) {
			free(fulldirname);
			free(entrypath);
			return -1;
	 }
		if (S_ISDIR(statentry.st_mode)){
			char * nsubdir = path_join(subdir,entry->d_name);
			if (!nsubdir) {
				free(fulldirname);
				free(entrypath);
				return -1;
			}
			if (parseDirToFD(fd, basedir, nsubdir) == -1) {
				free(fulldirname);
				free(nsubdir);
				free(entrypath);
				return -1;
			}
		} else if(S_ISREG(statentry.st_mode)){
			char * subfile = path_join(subdir,entry->d_name);
			if (!(subfile)) { 
				free(fulldirname);
				free(entrypath);
				return -1;
			}
			if (-1 == writef(fd, "%s\n%d\n", subfile, statentry.st_size)) return -1;
		}
		/* Ignores other st_modes by default */
	}
	free(fulldirname);
	if (errno) 
		return -1;
	else
		return 1;
}
