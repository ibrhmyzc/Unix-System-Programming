#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hw5.h"

void handler(int nix);

int main(int argc, char **argv){
    int i = 0;
    int err_check = 0;
    char filename[MAX_SIZE];
    // usage
    if(argc != 2){
        fprintf(stderr, "Usage: floristApp data.dat\n");
        exit(EXIT_FAILURE);
    }

	// signal handler
	signal(SIGINT, handler);

    //file name
    strcpy(filename, argv[1]);

    // random number seed
    srand(time(NULL));

    // initialize mutex
    if(pthread_mutex_init(&lock, NULL) != 0){
        fprintf(stderr, "mutex init error\n");
        exit(EXIT_FAILURE);
    }

    // creating a threadpool as many as number of florists
    florist_thread_pool = (ThreadPool*)malloc(sizeof(ThreadPool) * NUMBER_OF_FLORISTS);
    if(florist_thread_pool == NULL){
        fprintf(stderr, "could not allocate memory for threadpool\n");
        exit(EXIT_FAILURE);
    }else{
        for(i = 0; i < NUMBER_OF_FLORISTS; ++i){
            // fill pool
            florist_thread_pool[i].id = i;
            florist_thread_pool[i].queue.head = NULL;
        }

    }

    // read all the data from .dat file
    read_data(filename, florist_thread_pool);

    //inside of threadpool, I create pthread
    for(i = 0; i < NUMBER_OF_FLORISTS; ++i){
        int *arg = (int*)malloc(sizeof(*arg)); // will be freed in delivery function
        *arg = i;

        // create thread
        err_check = pthread_create(&(florist_thread_pool[i].florist_thread), NULL, (void*)deliver_flower, arg);
        if(err_check != 0){
            fprintf(stderr, "pthread creation error\n");
        }
    }
	
	pthread_mutex_lock(&cond_var_lock);
    while(total_delivered_flower < number_of_clients){
    	pthread_cond_wait( & cond_task_queue, & cond_var_lock ); 
    }
	pthread_mutex_unlock(&cond_var_lock);

    // wait all flowers to deliver
    for(i = 0; i < NUMBER_OF_FLORISTS; ++i){
        pthread_join(florist_thread_pool[i].florist_thread, NULL);
    }

    fprintf(stderr, "-------------------------\nAll requests are finished\n");
    show_statistics();

    // destroy mutex
    pthread_mutex_destroy(&lock);
    // free memory
	free_queue(florist_thread_pool);
    free(florist_thread_pool);
    return 0;
}

void handler(int nix){
	// free memory
	free_queue(florist_thread_pool);
    free(florist_thread_pool);
}
