#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_SIZE 1024

char LS_PATH[MAX_SIZE] = "";
char WC_PATH[MAX_SIZE] = "";
char CAT_PATH[MAX_SIZE] = "";
char CUR_DIR[MAX_SIZE] = "";

void setter();
int get_command(const char line[]);
void ls();
void wc(const char line[]);
void pwd(const char line[]);
void help();
void cd(const char line[]);
int isDir(const char *path);
void cat(const char line[]);
void handler(int nix);
int nth(const char line[]);

int main(int argc, char **argv) {
    /*START OF MAIN*/
    int go_on = 1, command_no = 0, counter = 0, n = 0, control_for_nth = 0;
    char command_history[MAX_SIZE][MAX_SIZE];
    char line[MAX_SIZE] = "";

    if(argc != 1){
        fprintf(stderr, "Usage: ./shell\n");
        exit(1);
    }

    /*set*/
    setter();

    /*ctrl c handler*/
    signal(SIGINT, handler);

    /*loop for shell*/
    while(go_on){
        if(control_for_nth == 1){
            if(counter - n >= 0)
                strcpy(line, command_history[counter - n]);
            else
                strcpy(line, command_history[0]);
            control_for_nth = 0;
        }else{
            fprintf(stderr, "%s ~$ ", CUR_DIR);
            fgets(line, MAX_SIZE, stdin);

            /*getting rid of \n*/
            if(line[strlen(line) - 1] == '\n'){
                line[strlen(line) - 1] = '\0';
            }

            /*storing the command*/
            if(counter < MAX_SIZE){
                strcpy(command_history[counter], line);
                ++counter;
            }else{
                fprintf(stderr, "Too many comments are entered.History has been reset\n");
                counter = 0;
            }
        }

        //fprintf(stderr, "Command: %s\n", line);

        command_no = get_command(line);
        switch (command_no){
            case 0:             /*exit command*/
                go_on = 0;
                break;
            case 1:             /*ls command*/
                ls();
                break;
            case 2:             /*pwd command*/
                pwd(line);
                break;
            case 3:             /*help command*/
                help();
                break;
            case 4:              /*cd command*/
                cd(line);
                break;
            case 5:              /*wc command*/
                wc(line);
                break;
            case 6:             /*cat command*/
                cat(line);
                break;
            case 7:
                n = nth(line);
                if(n != -1)
                    control_for_nth = 1;
                break;
            default:
                fprintf(stderr, "Invalid command\n");
                break;
        }
    }

    return 0;
    /*END OF MAIN*/
}

void setter(){
    char present_working_dir[MAX_SIZE] = "";
    if(getcwd(present_working_dir, MAX_SIZE * sizeof(char)) != NULL){
        sprintf(CUR_DIR, "%s", present_working_dir);
        sprintf(LS_PATH, "%s/ls", present_working_dir);
        sprintf(WC_PATH, "%s/wc", present_working_dir);
        sprintf(CAT_PATH, "%s/cat", present_working_dir);
    }
}

int get_command(const char line[]){
    const char space[2] = " ";
    char temp_line[MAX_SIZE] = "";
    char *token = NULL;

    /*preserving the original line*/
    strcpy(temp_line, line);
    token = strtok(temp_line, space);
    strcpy(temp_line, token);

    temp_line[strlen(temp_line)] = '\0';

    if((strcmp(temp_line, "exit") == 0) && strlen(line) == 4){
        return 0;
    }else if((strcmp(temp_line, "ls") == 0) && strlen(line) == 2){
        return 1;
    }else if((strcmp(temp_line, "pwd") == 0) && strlen(line) >= 3){
        return 2;
    }else if((strcmp(temp_line, "help") == 0) && strlen(line) == 4){
        return 3;
    }else if((strcmp(temp_line, "cd") == 0) && strlen(line) > 3){
        return 4;
    }else if((strcmp(temp_line, "wc") == 0) && strlen(line) > 3){
        return 5;
    }else if((strcmp(temp_line, "cat") == 0) && strlen(line) > 4){
        return 6;
    }else if((strcmp(temp_line, "nth") == 0) && strlen(line) > 4){
        return 7;
    }else{
        return -1;
    }
}

void pwd(const char line[]){
	int fd = 0, i = 0;
	char filename[MAX_SIZE] = "";

	char present_working_dir[MAX_SIZE] = "";
    if(getcwd(present_working_dir, MAX_SIZE * sizeof(char)) != NULL){
        fprintf(stderr, "PWD = %s\n", present_working_dir);
    }

	
	if(line[4] == '>'){
		/*extracting the filename out of the line*/
    	for(i = 6; i < (int)strlen(line); ++i){
		    filename[i-6] = line[i];
		    if(line[i] == '\0'){
		        break;
		    }
    	}

		/*Openning a log file for generated sequences*/
		if((fd = open(filename, O_APPEND | O_CREAT | O_WRONLY, 0777)) < 0){
		    fprintf(stderr, "File could not be opened for writing\n");
		}	

		write(fd, present_working_dir, strlen(present_working_dir));

		close(fd);
	}
}

void ls(){
    pid_t childPid = 0;
    char path[MAX_SIZE] = "";
    char *argv[3];

    /*argv[0] is the path of ls program*/
    argv[0] = LS_PATH;

    /*argv[1] is the path where we look for the files to be listed*/
    if(getcwd(path, MAX_SIZE * sizeof(char)) != NULL){
        argv[1] = path;
    }

    /*args must be null terminated*/
    argv[2] = NULL;

    childPid = fork();
    switch(childPid){
        /*in case fork fails*/
        case -1:
            fprintf(stderr, "Fork failed\n");
            break;
        case 0: /*CHILD*/
            execv(argv[0], argv);
            _exit(EXIT_SUCCESS);
        default: /*PARENT*/
            wait(NULL);
            break;
    }
}

void help(){
    fprintf(stderr, "Supported commands are listed below\n");
    fprintf(stderr, "\tls\n");
    fprintf(stderr, "\tpwd (only this supports '>')\n");
    fprintf(stderr, "\tcd (must be followed by a directory name or ..)\n");
    fprintf(stderr, "\thelp\n");
    fprintf(stderr, "\tcat\n");
    fprintf(stderr, "\twc\n");
    fprintf(stderr, "\texit\n");
    fprintf(stderr, "\tnth (nth must be followed by an integer)\n");
}

void cd(const char line[]){
    char temp_line[MAX_SIZE] = "";
    char tmp[MAX_SIZE] = "";
    char path[MAX_SIZE] = "";
    char change_dir[MAX_SIZE] = "";
    int i = 0, counter = 0, control = 0;

    /*Finding current working directory*/
    getcwd(path, MAX_SIZE * sizeof(char));

    /*getting rid of "cd" */
    for(i = 0; i < (int)strlen(line) - 3; ++i){
        temp_line[i] = line[i+3];
    }

    if(strcmp(temp_line, "..") == 0){
        for(i = 0; i < (int)strlen(CUR_DIR); ++i){
            if(CUR_DIR[i] == '/'){
                counter = i;
            }
        }
        CUR_DIR[counter] = '\0';
        control = chdir(CUR_DIR);
        if(control == -1){
            fprintf(stderr, "no such directory\n");
        }

    }else{
        if(isDir(change_dir))
        {
            strcpy(tmp, CUR_DIR);
            sprintf(change_dir, "%s/%s", path, temp_line);
            sprintf(CUR_DIR, "%s/%s", CUR_DIR, temp_line);
            
            control = chdir(CUR_DIR);
            if(control == -1){
                strcpy(CUR_DIR, tmp);
                fprintf(stderr, "no such directory\n");
            }
        }
    }
}

int isDir(const char *path){
    struct stat s;
    if (stat(path, &s) != 0)
        return 1;
    return 0;
}

void wc(const char line[]){
    pid_t childPid = 0;
    char path[MAX_SIZE] = "";
    char filename[MAX_SIZE] = "";
    char *argv[3];
    int i = 0;

    /*argv[0] is the path of wc program*/
    argv[0] = WC_PATH;

    /*extracting the filename out of the line*/
    for(i = 3; i < (int)strlen(line); ++i){
        filename[i-3] = line[i];
        if(line[i] == '\0'){
            break;
        }
    }

    /*argv[1] is the path where we look for the files to be listed*/
    if(getcwd(path, MAX_SIZE * sizeof(char)) != NULL){
        sprintf(path, "%s/%s", path, filename);
        argv[1] = path;
    }
    /*args must be null terminated*/
    argv[2] = NULL;

    childPid = fork();
    switch(childPid){
        /*in case fork fails*/
        case -1:
            fprintf(stderr, "Fork failed\n");
            break;
        case 0: /*CHILD*/
            execv(argv[0], argv);
            _exit(EXIT_SUCCESS);
        default: /*PARENT*/
            wait(NULL);
            break;
    }
}

void cat(const char line[]){
    pid_t childPid = 0;
    char path[MAX_SIZE] = "";
    char filename[MAX_SIZE] = "";
    char *argv[3];
    int i = 0;

    /*argv[0] is the path of cat program*/
    argv[0] = CAT_PATH;

    /*extracting the filename out of the line*/
    for(i = 4; i < (int)strlen(line); ++i){
        filename[i-4] = line[i];
        if(line[i] == '\0'){
            break;
        }
    }

    /*argv[1] is the path where we look for the files to be printed*/
    if(getcwd(path, MAX_SIZE * sizeof(char)) != NULL){
        sprintf(path, "%s/%s", path, filename);
        argv[1] = path;
    }
    /*args must be null terminated*/
    argv[2] = NULL;

    childPid = fork();
    switch(childPid){
        /*in case fork fails*/
        case -1:
            fprintf(stderr, "Fork failed\n");
            break;
        case 0: /*CHILD*/
            execv(argv[0], argv);
            _exit(EXIT_SUCCESS);
        default: /*PARENT*/
            wait(NULL);
            break;
    }
}

void handler(int nix){
    fprintf(stderr, "ctrl c is entered. Program is closing\n");
    exit(0);
}

int nth(const char line[]){
    int no = 0;
    char number[MAX_SIZE] = "";
    int i = 0;

    for(i = 0; i < (int)strlen(line); ++i){
        number[i] = line[3+i];
    }

    sscanf(number, "%d", &no);
    /*fprintf(stderr, "%d\n", no);*/

    if(no < MAX_SIZE - 1 && no != 0)
        return no + 1;
    else
        return -1;
}


