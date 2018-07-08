/* 
 * File:   client.c
 * Author: ibrhmyzc
 *
 * Created on May 29, 2018, 1:38 AM
 */

// Libraries
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
 #include <sys/time.h> 

// Macros
#define BUFFER_SIZE 1024
#define TRUE 1

typedef struct Client{
	char name[BUFFER_SIZE];
	char priority;
	int hw;
}Client;
// Function prototypes
void signalHandler(int);

// Global variables
int createSocket = 0;
int log_fd= 0;
int answer_flag = 1;
Client client;
char log_str[BUFFER_SIZE];
FILE *outp = NULL;



int main(int argc, char **argv){
// START OF MAIN
	char client_recieves[BUFFER_SIZE];
	char client_sends[BUFFER_SIZE];
	struct sockaddr_in server;
	char tmp;
	float answer = 0.0;
	int cost = 0;
	double provider_time = 0.0;
	char provider_name[BUFFER_SIZE];
	double total_time = 0.0;
	char ip[BUFFER_SIZE];
	int port = 0;

	if(argc != 6){
		fprintf(stderr, "Usage: hileci C 45 127.0.0.1 5555");
		return 1;
	}else{
		strcpy(client.name, argv[1]);
		client.priority = *argv[2];
		client.hw = atoi(argv[3]);
		strcpy(ip, argv[4]);
		port = atoi(argv[5]);
	}

	// signal handler in case of ctrl c
	signal(SIGINT, signalHandler);

	outp = fopen("clientLog.dat", "a");


	memset(client_recieves, '0', sizeof(client_recieves));
	memset(client_sends, '0', sizeof(client_sends));

	createSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(createSocket < 0){
		fprintf(stderr, "socket errror\n");
		return 1;
	}
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr(ip);

	if(connect(createSocket, (struct sockaddr*)&server, sizeof(server)) < 0){
		fprintf(stderr, "connection failed\n");
		return 1;
	}

	int n_read = 0;

	struct timeval t0, t1;

	gettimeofday(&t0, 0);
	
	log_str[0] = '\0';
	snprintf(log_str, sizeof(log_str), "Client %s is requesting %c %d from server %s:%d\n"
					,client.name, client.priority, client.hw, ip, port);
	//fprintf(stderr, "%s\n", log_str);
	//write(log_fd, log_str, sizeof(log_str));
	fputs(log_str, outp);
	while(TRUE){

		snprintf(client_sends, sizeof(client_sends),
				 "%d %s %c %d", getpid(), client.name, client.priority, client.hw);
		fprintf(stderr, "Client %s is requesting %c %d from server %s:%d\n"
			,client.name, client.priority, client.hw, ip, port);


		write(createSocket, client_sends, sizeof(client_sends));

		n_read = read(createSocket, client_recieves, sizeof(client_recieves) - 1);
		client_recieves[n_read] = 0;


		if(n_read > 0){
			//fprintf(stderr, "%s\n", client_recieves);

			sscanf(client_recieves, "%c %f %d %lf %s", &tmp, &answer, &cost, &provider_time, provider_name);
			// timer ends
			gettimeofday(&t1, 0);
			double elapsed = (t1.tv_sec - t0.tv_sec);
			total_time = provider_time + elapsed;


			fprintf(stderr, "%s's task completed by %s in %.2lf seconds, cos(%d) = %.2f, cost = %dTL, total spent time %.2lf seconds.\n"
					,client.name, provider_name, provider_time, client.hw, answer, cost, total_time);
			// rarely it returns 0
			if(total_time > 1){
				snprintf(log_str, sizeof(log_str), "%s's task completed by %s in %.2lf seconds, cos(%d) = %.2f, cost = %dTL, total spent time %.2lf seconds.\n",
				client.name, provider_name, provider_time, client.hw, answer, cost, total_time);
				//write(log_fd, log_str, sizeof(log_str));
				fputs(log_str, outp);
				fclose(outp);
			}

			
			//close(log_fd);
			//fclose(outp);
			close(createSocket);
			exit(0);
		}
	}

	
	close(createSocket);
	return (EXIT_SUCCESS);
// END OF MAIN
}


void signalHandler(int nix){
	fprintf(stderr, "\nSIGINT ist caught in client\n");
	snprintf(log_str, sizeof(log_str), "\nSIGINT ist caught in client\n");
	//write(log_fd, log_str, sizeof(log_str));
	fputs(log_str, outp);
	fclose(outp);
	close(createSocket);
	//close(log_fd);
	exit(nix);
}