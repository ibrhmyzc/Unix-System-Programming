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

void wc(const char* filename);

int main(int argc, char **argv){
	wc(argv[1]);
    return 0;
}

void wc(const char* filename){
	int fd = 0, counter = 0, empty = 1;
	char tmp = ' ';
	
	/*opening the file */
	 if((fd = open(filename, O_RDONLY, 0777)) < 0){
        fprintf(stderr, "File could not be opened for counting the words\n");
		exit(1);
    }
    
     /*Saves everything except the last sequence which will be deleted soon*/
    while(read(fd, &tmp, sizeof(char)) > 0){
		empty = 0;
        if(tmp == '\n' || tmp == ' '){
            counter++;
        }
    }
    if(empty)
		fprintf(stderr, "%s contains %d words\n", filename, counter);
	else
		fprintf(stderr, "%s contains %d words\n", filename, ++counter);
    close(fd);
}
