/* 
 * File:   server.c
 * Author: ibrhmyzc
 *
 * Created on May 29, 2018, 1:38 AM
 */

// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>


// macros
#define BUFFER_SIZE 1024
#define TRUE 1

// cheaters' details from data.dat
typedef struct Provider{
	char name[BUFFER_SIZE];
	int performance;
	int price;
	int duration;
	int total_served_clients;
	double total_served_time;
}Provider;

typedef struct JobQueue{
	int client_socket_fd;
	int hw;
	struct JobQueue *next;
}JobQueue;

typedef struct ThreadPool{
	pthread_t provider_thread;
	JobQueue *head;
	int queue_size;
	pthread_cond_t cond_empty;
    pthread_mutex_t cond_mutex;
    pthread_mutex_t func_mutex;
}ThreadPool;

typedef struct Helper{
	char server_receives[BUFFER_SIZE];
	int number_of_clients;
}Helper;

// Function prototypes
void signalHandler(int);
void read_providers_from_file(const char *);
float solve_taylor(int);
int calculate_factorial(int);
void log_history();
void init_threads(ThreadPool *);
void push_queue(char [], int);
void *tmp_th(void*);
void *do_hw(void *);
int choose_provider(char, int, char []);
void grafecully_exit();
void statistics();

//Global variables
pthread_mutex_t q_lock = PTHREAD_MUTEX_INITIALIZER;
ThreadPool *pool = NULL;
pthread_t myThreads[BUFFER_SIZE];
pid_t client_pids[BUFFER_SIZE] = {0};
int clientListen = 0;
int clientConnection[BUFFER_SIZE] = {0};
int number_of_clients = 0;
int fd_data = 0;
int fd = 0;
char log_str[BUFFER_SIZE];
int number_of_providers = -1;
Provider *providers = NULL;
int number_of_running_providers;

int main(int argc, char **argv){
// START OF MAIN
	struct sockaddr_in server; 
	char server_sends[BUFFER_SIZE];
	char server_receives[BUFFER_SIZE];
	time_t clock;
	char *log_file_name = NULL;
	char *data_file_name = NULL;
	int port = 0;

    memset(server_sends, '0', sizeof(server_sends));
    memset(server_receives, '0', sizeof(server_receives));

    // for generating random numbers
    srand(time(NULL));

	// Usage
	if(argc != 4){
		fprintf(stderr, "Usage: ./provider portNumber data.dat log.data");
		return 1;
	}

	// getting filenames
	port = atoi(argv[1]);
	data_file_name = argv[2];
	log_file_name = argv[3];

	// log file for server
	if((fd = open(log_file_name, O_APPEND | O_CREAT | O_WRONLY, 0777)) < 0){
        fprintf(stderr, "File could not be opened for logging server\n");
    }

	// signal handler in case of ctrl c
	signal(SIGINT, signalHandler);

	// read data
	read_providers_from_file(data_file_name);

	// createa a socket
	clientListen = socket(AF_INET, SOCK_STREAM, 0);
	if(clientListen < 0){
		perror("socket:");
		return 1;
	}
	memset(&server, '0', sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port);

	bind(clientListen, (struct sockaddr*)&server, sizeof(server));
	listen(clientListen, 100);


	// allocating memory for threadpool
	pool = (ThreadPool*)malloc(sizeof(ThreadPool) * number_of_providers);
	if(pool == NULL){
		fprintf(stderr, "memory allocation for threadpol is failed\n");
		snprintf(log_str, sizeof(log_str), "memory allocation for threadpol is failed\n");
		write(fd, log_str, strlen(log_str));
		exit(EXIT_FAILURE);
	}
	init_threads(pool);

	int n_read = 0;

	printf("\n\n\nserver is waiting for clients at port %d\n\n\n", port);
	while(TRUE){

		clientConnection[number_of_clients] = accept(clientListen, (struct sockaddr*)NULL, NULL);

		// reading the clients info
		n_read = read(clientConnection[number_of_clients], server_receives, sizeof(server_receives) -1);
		server_receives[n_read] = 0;

		// if the data is read
		if(n_read > 0){
			//direct clientsjob to the most appropriate provider
			// clients handles first as a new thread 
			
			Helper *arg = (Helper*)malloc(sizeof(Helper));
			strcpy(arg->server_receives, server_receives);
			arg->number_of_clients = number_of_clients;
			if(pthread_create(&(myThreads[number_of_clients]), NULL, tmp_th, arg))
	        {
	            fprintf(stderr, "failed to create thread:\n");
	        }
			push_queue(server_receives, number_of_clients);
		}

		
		// waits for the next client
		number_of_clients++;
	}	

	// close socket
	close(clientListen);
	return (EXIT_SUCCESS);
//END OF MAIN
}



void *tmp_th(void *arg){
	Helper *tmp = (Helper*)arg; 
	int index = 0;
	char str[BUFFER_SIZE];

	strcpy(str, tmp->server_receives);
	index = tmp->number_of_clients;
	//fprintf(stderr, "new thread handles %s with clientid %d\n", str, index);

	//index = choose_provider(clients_priority, clients_hw, task_info);
	

	free(arg);
}






void push_queue(char task_info[], int client_index){
	char info[BUFFER_SIZE];
	int client_pid = 0;
	char clients_name[BUFFER_SIZE];
	char clients_priority;
	int clients_hw = 0;
	int index = 0;
	char sorry_message[BUFFER_SIZE];

	sleep(0.2);
	pthread_mutex_lock(&q_lock);
	//fprintf(stderr, "%s is being processed", task_info);

	// here parsing the information happens
	sscanf(task_info, "%d %s %c %d", &client_pid, clients_name, &clients_priority, &clients_hw);



	// we need to keep all of clients pids in case of sigint
	client_pids[number_of_clients++] = client_pid;



	// now we need to forward this job to one of the providers
	index = choose_provider(clients_priority, clients_hw, task_info);
	if(index == -1){
		fprintf(stderr, "All providers are busy unfortunately\n");
		snprintf(sorry_message, sizeof(sorry_message), "No provider is availabel for you :/\n");
		write(fd, sorry_message, strlen(sorry_message));
		write(clientConnection[client_index], sorry_message, strlen(sorry_message));
		close(clientConnection[client_index]);
		//fprintf(stderr, "Connections are closed. Index is %d\n", index);
		sleep(2);
	}


	//fprintf(stderr, "Provider %s s chosen\n", providers[index]);


	if(index >= 0 && index < number_of_providers){
		// here we add the job to selected providers task queue
		if(pool[index].head ==  NULL){
			//allocate memory for head
			pool[index].head = (JobQueue*)malloc(sizeof(JobQueue));
			// check if it failed or not
			if(pool[index].head == NULL){
				fprintf(stderr, "memory allocation error for head\n");
			}else{
				// here we add the job to the queue
				pool[index].head->hw = clients_hw;
				pool[index].head->client_socket_fd = clientConnection[client_index];
				pool[index].head->next = NULL;
				pool[index].queue_size++;
			}
		}else{
			// if head is not null, now we need to check if there are 1 or 2 jobs
			// since task queue cannot exceed the lenght 2
			// we will need to forward  the client to another provider
			if(pool[index].queue_size == 1){
				// add one more job
				JobQueue *next = (JobQueue*)malloc(sizeof(JobQueue));
				if(next == NULL){
					fprintf(stderr, "memory allocation error for next\n");
				}else{
					pool[index].head->next = next;
					next->hw = clients_hw;
					next->client_socket_fd = clientConnection[client_index];
					next->next = NULL;
					pool[index].head->next = next;
					pool[index].queue_size++;
				}
			}else if(pool[index].queue_size == 2){
				fprintf(stderr, "Provider %s is busy. Next best choice will be chosen\n", providers[index].name);
				snprintf(sorry_message, sizeof(sorry_message), "No provider is availabel for you :/\n");
				write(clientConnection[client_index], sorry_message, strlen(sorry_message));
				close(clientConnection[client_index]);

			}else{
				fprintf(stderr, "queue of %s has illegal size!! size:%d\n", providers[index].name, pool[index].queue_size);
				exit(1);
			}
		}
	}
	
	pthread_mutex_unlock(&q_lock);

}

int choose_provider(char client_priority, int clients_hw, char server_receives[]){
	int index = 0;
	int val = 0;
	int best = 0;
	int i = 0;
	int j = 0;
	
	if(number_of_running_providers == number_of_providers){
		fprintf(stderr, "All providers are offline\n");
		snprintf(log_str, sizeof(log_str), "All providers are offline\n");
		write(fd, log_str, strlen(log_str));
		grafecully_exit();
	}

	for(i = 0; i < number_of_providers; ++i){
		if(pool[i].queue_size != 2){
			break;
		}
	}
	if(i == number_of_providers){
		return -1;
	}

	// initialize
	if(client_priority == 'c' || client_priority == 'C'){
		best = providers[0].price;
	}else if(client_priority == 'Q' || client_priority == 'q'){
		best = providers[0].performance;
	}else if(client_priority == 't' || client_priority == 'T'){
		best = 2;
	}

	// finds the best provider
	for(i = 0; i < number_of_providers; ++i){
		// here we choose the best availabe provider
		if(client_priority == 'c' || client_priority == 'C'){
			// low cost
			val = providers[i].price;
			if(val < best && pool[i].queue_size != 2){
				best = val;
				index = i;
			}
		}else if(client_priority == 'Q' || client_priority == 'q'){
			// high quality
			val = providers[i].performance;
			if(val > best && pool[i].queue_size != 2){
				best = val;
				index = i;
			}
		}else if(client_priority == 't' || client_priority == 'T'){
			// high performance
			val = pool[i].queue_size;
			if(val < best && pool[i].queue_size != 2){
				best = val;
				index = i;
			}
		}
	}


	index = rand() % number_of_providers;
	fprintf(stderr, ">>>Client with pid %s is connected and forwarded to provider %s\n", server_receives, providers[index].name);
	snprintf(log_str, sizeof(log_str), ">>>Client with pid %s is connected and forwarded to provider %s\n",  server_receives, providers[index].name);
	write(fd, log_str, strlen(log_str));
	return index;
}


void init_threads(ThreadPool *pool){
	int i = 0;
	int err_check = 0;

	//fprintf(stderr, "start of init\n");

	for(i = 0; i < number_of_providers; ++i){
		// creating thread
		int *arg = (int*)malloc(sizeof(*arg)); // will be freed in thread function
        *arg = i;
        
        //fprintf(stderr, "%d. thread is being created\n", *arg);

        // initial queue size is 0
        pool[i].queue_size = 0;

        err_check = pthread_create(&(pool[i].provider_thread), NULL, (void*)do_hw, arg);
        if(err_check != 0){
            fprintf(stderr, "pthread creation error\n");
        }

        if(pthread_mutex_init(&pool[i].func_mutex, NULL) != 0) {
	        perror("mutex_init:");
	    }

	    if(pthread_mutex_init(&pool[i].cond_mutex, NULL) != 0) {
	        perror("mutex_init:");
	    }
	    if(pthread_cond_init(&pool[i].cond_empty, NULL) != 0) {
	        perror("cond_init:Empty Queue");
	    }


	    pool[i].head = NULL;
	}

	//fprintf(stderr, "Initialization of threads are completed\n");
}




void *do_hw(void *arg){
	void *result = NULL;
	int index = *((int*)arg);
	int flag = 1;
	int client_fd = 0;
	int isShowing = 1;
	int cost = 0;
	double provider_time = 0.0;
	int r_time = 0;

	pthread_mutex_lock(&pool[index].func_mutex);
	while(flag == 1){
 		

		// find client index
		if(pool[index].queue_size != 0){

			//fprintf(stderr, " >>> Queue size of %s is %d\n", providers[index].name, pool[index].queue_size);


			// processing info
			client_fd = pool[index].head->client_socket_fd;
			char solution[BUFFER_SIZE];
	        int degree = pool[index].head->hw;
	    
			fprintf(stderr, "Provider %s is processing task of finding cos(%d)\n", providers[index].name, degree);
			snprintf(log_str, sizeof(log_str), "Provider %s is processing task of finding cos(%d)\n", providers[index].name, degree);
			write(fd, log_str, strlen(log_str));
			// start timer
			clock_t p_time;
			p_time = clock();

			// simulate heavy work
			r_time = rand() % 10 + 5;
			sleep(r_time);

			// preparing and sending the data to the client

			solution[0] = '\0';
			char tmp = '>';
			float answer = solve_taylor(degree);
			cost = providers[index].price;
			char name[32];
			strcpy(name, providers[index].name);

			//end timer
			p_time = clock() - p_time;
			provider_time = (float)p_time / CLOCKS_PER_SEC;
			if(provider_time < 5)
				provider_time = r_time;
			if(r_time < 5)
				provider_time = 5 + r_time * 0.1;
			//Provider Ayse completed task number 2: cos(55)=0.573 in 9.71 seconds.
			fprintf(stderr, "Provider %s has completed the task in %.2lf seconds. cos(%d) = %.2f\n", name, provider_time, degree, answer);
			snprintf(log_str, sizeof(log_str), "Provider %s has completed the task in %.2lf seconds. cos(%d) = %.2f\n",
								 name, provider_time, degree, answer);
			write(fd, log_str, strlen(log_str));

			snprintf(solution, sizeof(solution), "%c %.2f %d %lf %s", tmp, answer, cost, provider_time, name);
	        write(client_fd, solution, strlen(solution));
	   

	        // now we have to pop this from queue
	        pool[index].head = pool[index].head->next;
	        pool[index].queue_size -= 1;

	        //fprintf(stderr, " <<< Queue size of %s is %d\n", providers[index].name, pool[index].queue_size);
	        isShowing = 1;
	        providers[index].total_served_clients++;
	        providers[index].total_served_time = provider_time;
	        if(providers[index].total_served_time >= providers[index].duration){
	        	fprintf(stderr, "%s's time is up!!!\n", providers[index].name)
;	        	flag = 0;
	        }
	        close(client_fd);

		}else if(isShowing == 1 && pool[index].queue_size == 0){

			fprintf(stderr, "Provider %s is waiting for a task\n", providers[index].name);
			snprintf(log_str, sizeof(log_str), "Provider %s is waiting for a task\n", providers[index].name);
			write(fd, log_str, strlen(log_str));
			isShowing = 0;

		}
             

	}
	pthread_mutex_unlock(&pool[index].func_mutex);
	fprintf(stderr, "Provider %s is now exiting\n", providers[index].name);
	snprintf(log_str, sizeof(log_str), "Provider %s is now exiting\n", providers[index].name);
	write(fd, log_str, strlen(log_str));
	++number_of_running_providers;

	free(arg);
	return result;
}

float solve_taylor(int x){
	const int STEP = 5;
	const float PI = 3.14;
	int flag = 0;
	if(x > 180){
		x = x - 180;
		flag = 1;
	}
	float x_val = (x * PI) / 180;
	float result = 0.0;
	int n = 0;
	int sign = 1;
	for(n = 0; n < STEP; ++n){
		result += sign * pow(x_val, 2 * n) / calculate_factorial(2 * n);
		sign *= -1;
	}
	if(flag == 1)
		return -result;
	else
		return result;
}

int calculate_factorial(int n){
	int i = 0;
	int result = n;
	for(i = 1; i < n; ++i){
		result *= i;
	}
	if(result > 0)
		return result;
	else
		return 1;
}

void read_providers_from_file(const char *filename){
	
	char line[BUFFER_SIZE] = {' '};
	char tmp = ' ';

	// open the file for providers' infos in read only mode
    if((fd_data = open(filename, O_RDONLY, 0777)) < 0){
        perror("file could not be opened");
        exit(EXIT_FAILURE);
    }

    while(read(fd_data, &tmp, sizeof(char)) > 0){
    	if(tmp == '\n')
    		number_of_providers++;
    }
    close(fd_data);

    fprintf(stderr, "%d providers with the following infos are read from file\n", number_of_providers);
    snprintf(log_str, sizeof(log_str), "%d providers with the following infos are read from file\n", number_of_providers);
	write(fd, log_str, strlen(log_str));
    // creating a provider array
    providers = (Provider*)malloc(sizeof(Provider) * number_of_providers);
    if(providers == NULL){
    	fprintf(stderr, "could not allocate memory for provider array\n");
    	return 1;
    }



    if((fd_data = open(filename, O_RDONLY, 0777)) < 0){
        perror("file could not be opened");
        exit(EXIT_FAILURE);
    }


    // read line by line 
    memset(line, '0', sizeof(line));
    int index = 0;
    int counter = 0;
    int n_read = 1;
    while(TRUE && n_read != 0){
    	n_read = read(fd_data, &tmp, sizeof(char));
    	line[counter++] = tmp;
    	if(tmp == '\n'){
    		if(index != 0){
    			// parse here
    			sscanf(line, "%s %d %d %d"
		    				 , &providers[index-1].name
		    				 , &providers[index-1].performance
		    				 , &providers[index-1].price
		    				 , &providers[index-1].duration);
    		}
    		
    		counter = 0;
    		index++;
    	}
    }

    // print providers
    for(int i = 0; i < number_of_providers; ++i){
    		fprintf(stderr, "%s %d %d %d\n", providers[i].name
    									   , providers[i].performance
    									   , providers[i].price
    									   , providers[i].duration);
    		snprintf(log_str, sizeof(log_str), "%s %d %d %d\n", providers[i].name
    									   , providers[i].performance
    									   , providers[i].price
    									   , providers[i].duration);
			write(fd, log_str, strlen(log_str));
    }

    // close file
    close(fd_data);
}

void grafecully_exit(){
	fprintf(stderr, "Sockets are being closed. Clients are being informed and allocated memory is being freed\n");
	snprintf(log_str, sizeof(log_str), "Sockets are being closed. Clients are being informed and allocated memory is being freed\n");
	write(fd, log_str, strlen(log_str));

	// show stats
	statistics();


	// close open sockets
	close(clientListen);
	close(clientConnection);

	// close files
	close(fd_data);
	close(fd);

	// free memory
	if(pool != NULL){
		int i = 0;
		int q_size = 0;
		for(i = 0; i < number_of_providers; ++i){
			q_size = pool[i].queue_size;
			if(q_size > 0){			
				if(pool[i].head->next != NULL){
					fprintf(stderr, "%s is deleted %d\n", providers[i].name, pool[i].head->next->hw);
					free(pool[i].head->next);
					if(pool[i].head != NULL){
						fprintf(stderr, "%s is deleted %d\n", providers[i].name, pool[i].head->hw);
						free(pool[i].head);
					}
				}else if(pool[i].head != NULL){
					fprintf(stderr, "%s is deleted %d\n", providers[i].name, pool[i].head->hw);
					free(pool[i].head);
				}			
			}
			// destroy mutex and cond var
			pthread_mutex_destroy(&pool[i].cond_mutex);
    		pthread_cond_destroy(&pool[i].cond_empty);
    		pthread_mutex_destroy(&pool[i].func_mutex);
		}

		// destroy pool
		free(pool);
		
	}

	// free dynhamically allocated memory
	if(providers != NULL)
		free(providers);

	// kill clients
	for(int i = 0; i < number_of_clients; ++i){
		kill(client_pids[i], SIGINT);
	}
	exit(1);
}

void statistics(){
	int i = 0;
	fprintf(stderr, " *** Statistics ***\n");
	snprintf(log_str, sizeof(log_str), "*** Statistics ***\n");
	write(fd, log_str, strlen(log_str));

	fprintf(stderr, "Name     Number of Clients served\n");
	snprintf(log_str, sizeof(log_str), "Name     Number of Clients served\n");
	write(fd, log_str, strlen(log_str));

	for(i = 0; i < number_of_providers; ++i){
		fprintf(stderr, "%s      %d\n", providers[i].name, providers[i].total_served_clients);
		snprintf(log_str, sizeof(log_str), "%s      %d\n", providers[i].name, providers[i].total_served_clients);
		write(fd, log_str, strlen(log_str));
	}
}

void signalHandler(int nix){
	fprintf(stderr, "\nSIGINT ist caught in server\n");
	snprintf(log_str, sizeof(log_str), "\nSIGINT ist caught in server\n");
	write(fd, log_str, strlen(log_str));

	fprintf(stderr, "Sockets are being closed. Clients are being informed and allocated memory is being freed\n");
	snprintf(log_str, sizeof(log_str), "Sockets are being closed. Clients are being informed and allocated memory is being freed\n");
	write(fd, log_str, strlen(log_str));

	// show stats
	statistics();


	// close open sockets
	close(clientListen);
	close(clientConnection);

	// close files
	close(fd_data);
	close(fd);

	// free memory
	if(pool != NULL){
		int i = 0;
		int q_size = 0;
		for(i = 0; i < number_of_providers; ++i){
			q_size = pool[i].queue_size;
			if(q_size > 0){			
				if(pool[i].head->next != NULL){
					//fprintf(stderr, "%s is deleted %d\n", providers[i].name, pool[i].head->next->hw);
					free(pool[i].head->next);
					if(pool[i].head != NULL){
						//fprintf(stderr, "%s is deleted %d\n", providers[i].name, pool[i].head->hw);
						free(pool[i].head);
					}
				}else if(pool[i].head != NULL){
					//fprintf(stderr, "%s is deleted %d\n", providers[i].name, pool[i].head->hw);
					free(pool[i].head);
				}			
			}
			// destroy mutex and cond var
			pthread_mutex_destroy(&pool[i].cond_mutex);
    		pthread_cond_destroy(&pool[i].cond_empty);
    		pthread_mutex_destroy(&pool[i].func_mutex);
		}

		// destroy pool
		free(pool);
		
	}

	// free dynhamically allocated memory
	if(providers != NULL)
		free(providers);

	// kill clients
	for(int i = 0; i < number_of_clients; ++i){
		kill(client_pids[i], SIGINT);
	}

	exit(nix);
}