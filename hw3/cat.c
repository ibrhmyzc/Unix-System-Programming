#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

#define MAX_SIZE 1024

void cat(const char* filename);

int main(int argc, char **argv){
	cat(argv[1]);
    return 0;
}

void cat(const char* filename){
	int fd = 0, counter = 0, empty = 1;
	char tmp = ' ';
	
	/*opening the file */
	 if((fd = open(filename, O_RDONLY, 0777)) < 0){
        fprintf(stderr, "File could not be opened for printing the content\n");
		exit(1);
    }
    
     /*Saves everything except the last sequence which will be deleted soon*/
    while(read(fd, &tmp, sizeof(char)) > 0){
		fprintf(stderr, "%c", tmp);
    }
    close(fd);
}
