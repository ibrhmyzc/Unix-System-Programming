#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <malloc.h>
#include <math.h>

#define PI 3.14159265

int getNumberOfLines(const char *filename);
void generateSequence(int lengthOfSequence, int maximumLineNumber, const char *filename);
void writeToFile(double *sequence, int lengthOfSequence, const char *filename);
void solver(const char *filename, int lengthOfSequence);
double* getLastSequence(const char *filename, int lengthOfSequence);
void replaceText(const char *filename);
void stringToDouble(char* realNumberAsString, double *sequence, int lengthOfSequence);
char* dft(const double *sequence, int lengthOfSequence);
void print(double* sequence, int lengthOfSequence);
void logParent(const char *generated);
void logChild(const char *result, double *sequence, int lengthOfSequence);
void handler(int nix);

pid_t childPid = 0;
char *dftResult = NULL;
double *solverSequence = NULL;
double *lastSequence = NULL;
double *genSequence = NULL;
char *solverResult = NULL;

int main(int argc, char *argv[]) {
    char filename[1024] = "";
    int N = 0, M = 0;
    int status = 1;

    /*Usage check*/
    if(argc != 7){
        fprintf(stderr, "Usage ./multiprocess_DFT -N 5 -X file.dat -M 100\n");
        return 1;
    }

    N = atoi(argv[2]);
    M = atoi(argv[6]);
    strcpy(filename, argv[4]);
    //fprintf(stderr, "Filename: %s\n", filename);
    //fprintf(stderr, "N = %d M = %d\n", N, M);

    /*ctrl c handler*/
    signal(SIGINT, handler);

    /*For getting random numbers*/
    srand( (unsigned) time(NULL) * getpid());

    switch(childPid = fork()){
        /*in case fork fails*/
        case -1:
            fprintf(stderr, "Fork failed\n");
        case 0:
            /*Child solves sequences*/
            while(1){
                solver(filename, N);
                
            }
            _exit(EXIT_SUCCESS);
        default:
            /*Parent generates sequences*/
            while(1){
                generateSequence(N, M, filename);
                
            }
            wait(NULL);
            break;
    }
    fprintf(stderr, "\n***Program terminates***\n");
    return 0;
}

int getNumberOfLines(const char *filename){
    int fd = 0;
    int numberOfLines = 0;
    char tmp = ' ';

    if((fd = open(filename, O_RDONLY, 0777)) < 0){
        fprintf(stderr, "File could not be opened for counting the generated sequence\n");
        return 0;
    }
    /*Counts backslash ns*/
    while((read(fd, &tmp, sizeof(char))) == 1){
        if(tmp == '\n'){
            numberOfLines++;
        }
    }
    /*closing the file*/
    close(fd);
    return numberOfLines;
}

void generateSequence(int lengthOfSequence, int maximumLineNumber, const char *filename){
    int numberOfLines = 0;

    /*Allocating memory for the sequence*/
    genSequence = (double*)malloc(sizeof(double) * lengthOfSequence);
    if(genSequence == NULL){
        fprintf(stderr, "Memory not allocated\n");
        exit(1);
    }

    /*Filling the array with random numbers*/
    for(int i = 0; i < lengthOfSequence; ++i){
        genSequence[i] = rand() % 10 + 1;
    }

    /*If it exceeds the maximum number of lines, it will not write it to the file*/
    numberOfLines = getNumberOfLines(filename);
    if(numberOfLines < maximumLineNumber){
        writeToFile(genSequence, lengthOfSequence, filename);
    }else{
        if(genSequence != NULL){
            free(genSequence);
        }
    }
}

void writeToFile(double *sequence, int lengthOfSequence, const char *filename){
    int fd = 0;
    char str[2048] = "";
    str[0] = '\0';

    /**/
    if((fd = open(filename, O_APPEND | O_CREAT | O_WRONLY, 0777)) < 0){
        //fprintf(stderr, "File could not be opened for writing the generated sequence\n");
    }

    /*Writing the sequence in the file*/
    for(int i = 0; i < lengthOfSequence; ++i){
        if(i == 0){
            sprintf(str, "%.1lf", sequence[i]);
        }else{
            sprintf(str, "%s %.1lf", str,  sequence[i]);
        }
        if(i == lengthOfSequence - 1){
            sprintf(str, "%s\n", str);
        }
    }
    if(write(fd, str, strlen(str)) != (unsigned)strlen(str)){
        //fprintf(stderr, "Sequence could not be written in the file\n");
    }
    fprintf(stderr, "Sequence generated = %s", str);
    logParent(str);

    /*Closing the file*/
    close(fd);
    /*Freeing the memory*/
    if(sequence != NULL)
        free(sequence);
}

void solver(const char *filename, int lengthOfSequence){
    int numberOfLines = 0;
    numberOfLines = getNumberOfLines(filename);
    if(numberOfLines != 0){
        /*Read the last sequence from the file*/
        solverSequence = getLastSequence(filename, lengthOfSequence);

        fprintf(stderr, "Sequence read = ");
        print(solverSequence, lengthOfSequence);

        /*Solve it*/
        solverResult = dft(solverSequence, lengthOfSequence);
        fprintf(stderr, "Answer = %s", solverResult);

        /*logging*/
        logChild(solverResult, solverSequence, lengthOfSequence);

        /*Freeing allocated memory*/
        if(solverSequence != NULL)
            free(solverSequence);
        if(solverResult != NULL)
            free(solverResult);
    }
}

double* getLastSequence(const char *filename, int lengthOfSequence){
    char realNumberAsString[65000] = "";
    int fd = 0;
    int fd2 = 0;
    int numberOfLines = 0;
    int counter = 0;
    char tmp = ' ';

	realNumberAsString[0] = '\0';
    /*Allocating memory*/
	lastSequence = NULL;
    lastSequence = (double*)malloc(sizeof(double) * lengthOfSequence);
    numberOfLines = getNumberOfLines(filename);

    if((fd = open(filename, O_RDONLY, 0777)) < 0){
        fprintf(stderr, "File could not be opened for counting the generated sequence\n");
        return lastSequence;
    }

    if((fd2 = open("tmp.dat", O_CREAT | O_WRONLY | O_TRUNC, 0777)) < 0){
        fprintf(stderr, "File could not be opened for writing tmp file\n");
        return lastSequence;
    }

    /*Saves everything except the last sequence which will be deleted soon*/
    while(counter < numberOfLines - 1){
        read(fd, &tmp, sizeof(char));
        write(fd2, &tmp, sizeof(char));
        if(tmp == '\n'){
            counter++;
        }
    }

    counter = 0;
	realNumberAsString[0] = '\0';
    while((read(fd, &tmp, sizeof(char))) == 1){
		sprintf(realNumberAsString, "%s%c", realNumberAsString, tmp);
        //realNumberAsString[counter++] = tmp;
    }
    //realNumberAsString[counter] = '\0';

    stringToDouble(realNumberAsString, lastSequence, lengthOfSequence);

    /*Closing files*/
    close(fd);
    close(fd2);

    /*Deletes the last sequence*/
    replaceText(filename);

    return lastSequence;
}

void replaceText(const char *filename){
    int fd = 0;
    int fd2 = 0;
    char tmp = ' ';

    if((fd = open("tmp.dat", O_RDONLY, 0777)) < 0){
        fprintf(stderr, "File could not be opened for counting the generated sequence\n");
    }

    if((fd2 = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0777)) < 0){
        fprintf(stderr, "File could not be opened for writing tmp file\n");
    }

    while((read(fd, &tmp, sizeof(char))) == 1){
        write(fd2, &tmp, sizeof(char));
    }
    close(fd);
    close(fd2);
}

void stringToDouble( char* realNumberAsString, double *sequence, int lengthOfSequence){
    int counter = 0;
    const char delim[2] = " \0";
    char *token = NULL;

    token = strtok(realNumberAsString, delim);
    while(token != NULL && counter < lengthOfSequence){
        sscanf(token, "%lf", &sequence[counter++]);
        token = strtok(NULL, delim);
    }
}

char* dft(const double *sequence, int lengthOfSequence) {
    double power = 0.0;
    double degree = 0.0;
    double real = 0.0;
    double imaginary = 0.0;

    dftResult = (char*)malloc(sizeof(char) * 1024);
    dftResult[0] = '\0';

    /*Solves dft*/
    for (int k = 0; k < lengthOfSequence; ++k) {
        for (int x = 0; x < lengthOfSequence; ++x) {
            power = 2 * k * x;
            degree = power / lengthOfSequence;
            real += cos(degree * PI) * sequence[x];
            imaginary += -sin(degree * PI) * sequence[x];
        }
        if (imaginary >= 0) {
            if (k != 0 && real > 0) {
                sprintf(dftResult, "%s+", dftResult);
                sprintf(dftResult, "%s%.2f+%.2fi ", dftResult, real, imaginary);
            }
            else {
                sprintf(dftResult, "%s%.2f+%.2fi ", dftResult, real, imaginary);
            }
        }
        else {
            sprintf(dftResult, "%s%.2f%.2fi ", dftResult, real, imaginary);
        }
        real = 0.0;
        imaginary = 0.0;
    }
    sprintf(dftResult, "%s\n", dftResult);
    return dftResult;
}

void print(double* sequence, int lengthOfSequence){
    for(int i = 0; i < lengthOfSequence; ++i){
        fprintf(stderr, "%.2lf ", sequence[i]);
    }
    fprintf(stderr, "\n");
}

void logParent(const char *generated){
    int fd = 0;

    /*Openning a log file for generated sequences*/
    if((fd = open("logParent.dat", O_APPEND | O_CREAT | O_WRONLY, 0777)) < 0){
        //fprintf(stderr, "File could not be opened for logging parent\n");
    }

    /*Writing them into the file*/
    write(fd, generated, strlen(generated));

    /*Closing the file*/
    close(fd);
}

void logChild(const char *result, double *sequence, int lengthOfSequence){
    int fd = 0;
    char str[1024] = "";

    /*Openning a log file for the child process*/
    if((fd = open("logChild.dat", O_APPEND | O_CREAT | O_WRONLY, 0777)) < 0){
        //fprintf(stderr, "File could not be opened for logging child\n");
    }
    /*Writing the sequence in the file*/
    for(int i = 0; i < lengthOfSequence; ++i){
        if(i == 0){
            sprintf(str, "%.1lf", sequence[i]);
        }else{
            sprintf(str, "%s %.1lf", str,  sequence[i]);
        }
        if(i == lengthOfSequence - 1){
            sprintf(str, "%s\n", str);
        }
    }
    if(write(fd, str, strlen(str)) != (unsigned)strlen(str)){
        //fprintf(stderr, "Sequence could not be written in the file\n");
    }

    /*Writing the results*/
    if(write(fd, result, strlen(result)) != (unsigned)strlen(result)){
        //fprintf(stderr, "LogChild could not be written in the file\n");
    }

    /*Closing the file*/
    close(fd);
}

void handler(int nix){
    fprintf(stderr, "Log files have been created\n");
	kill(childPid, SIGINT);
    /*Freeing potential leaks*/
	if(dftResult != NULL){
		free(dftResult);
	}
	if(lastSequence != NULL){
		free(lastSequence);
	}
	if(solverSequence != NULL){
		free(solverSequence);
	}
	if(genSequence != NULL){
		free(genSequence);
	}
	if(solverResult != NULL){
		free(solverResult);
	}
    
    exit(0);
}
