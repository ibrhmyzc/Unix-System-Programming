#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#define MAX_SIZE 1024

void ls(const char* path);

int main(int argc, char **argv){
	ls(argv[1]);
    return 0;
}

void ls(const char* path){
    char file_name[MAX_SIZE] = "";
    struct stat fileStat;
    struct dirent *direntp = NULL;
    DIR *dirp = NULL;

    file_name[0] = '\0';
    if((dirp = opendir(path)) == NULL){
        perror("Failed to open directory\n");
    }

    while((direntp = readdir(dirp)) != NULL){
        /*File name*/
        if(strcmp(direntp->d_name, ".") != 0 && strcmp(direntp->d_name, "..") != 0 ){
            fprintf(stderr, "%s ", direntp->d_name);
            if(stat(direntp->d_name, &fileStat) < 0){
                perror("file stat error\n");
            }
            fprintf(stderr, "%ld ", fileStat.st_size);
            fprintf(stderr, (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
            fprintf(stderr, (fileStat.st_mode & S_IRUSR) ? "r" : "-");
            fprintf(stderr, (fileStat.st_mode & S_IWUSR) ? "w" : "-");
            fprintf(stderr, (fileStat.st_mode & S_IXUSR) ? "x" : "-");
            fprintf(stderr, (fileStat.st_mode & S_IRGRP) ? "r" : "-");
            fprintf(stderr, (fileStat.st_mode & S_IWGRP) ? "w" : "-");
            fprintf(stderr, (fileStat.st_mode & S_IXGRP) ? "x" : "-");
            fprintf(stderr, (fileStat.st_mode & S_IROTH) ? "r" : "-");
            fprintf(stderr, (fileStat.st_mode & S_IWOTH) ? "w" : "-");
            fprintf(stderr, (fileStat.st_mode & S_IXOTH) ? "x" : "-");
            fprintf(stderr, "\n");
        }
    }
    dirp = NULL;
}
