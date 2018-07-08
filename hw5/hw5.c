#include "hw5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unistd.h"
#include "fcntl.h"
#include <pthread.h>
#include <math.h>

pthread_mutex_t lock;
pthread_cond_t  cond_task_queue = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cond_var_lock = PTHREAD_MUTEX_INITIALIZER; 


int fd_data = 0;
int number_of_clients = 0;
int waiting_jobs[MAX_SIZE] = {0};
int job_list[NUMBER_OF_FLORISTS][MAX_SIZE] = {0};
double total_time[NUMBER_OF_FLORISTS] = {0.0};
int total_delivered_flower;

void *deliver_flower(void *arg){
    double delivery_time = 0.0;
	void *result = NULL;
    double distance = 0;
    int preparation = 0;
    Task *head = NULL;
    int client_no = 0;
    char flower[MAX_SIZE];
    int florist_index = *((int*)arg);

    head = florist_thread_pool[florist_index].queue.head;

    // do the task and go on until jobs on the queue are finished
    while(head != NULL){

        // getting client info
        client_no = head->arg - 1;
		if(client_no < 0 || client_no > 23){
			client_no = 0;
		}
        strcpy(flower, clients[client_no].desired_flower);

        // calculate delivery time
        preparation = rand() % 40 + 10;
        distance = clients[client_no].distance;
        delivery_time = florists[florist_index].click_speed * distance + preparation;

        fprintf(stderr, ">Florist %s has delivered a %s to the client%d in %.0lfms\n",
                florists[florist_index].name, flower, client_no+1, delivery_time);

        // pop it from queue adn go to next if there is any
        //pthread_mutex_lock(&lock);
		
        total_time[florist_index] += delivery_time;
        head = head->next;
        clients[client_no].served = 1;
        clients[client_no].delivery_time = delivery_time;
        total_delivered_flower++;
	
        pthread_cond_signal( &cond_task_queue); 
        //thread_mutex_unlock(&lock);
    }
    free(arg);
	return result;
}

int find_closest_florist(int client_no){
    int client_x = 0;
    int client_y = 0;
    int florist_x = 0;
    int florist_y = 0;
    double tmp_distance = 0;
    double distance = 999999999;
    int i = 0;
    int nearest_florist_index = 0;

    // where is the client located
    client_x = clients[client_no].client_x;
    client_y = clients[client_no].client_y;

    // finds the closest florist
    for(i = 0; i < NUMBER_OF_FLORISTS; ++i){
        florist_x = florists[i].florist_x;
        florist_y = florists[i].florist_y;
        //fprintf(stderr, "%d,%d,%d,%d\n", client_x, client_y, florist_x, florist_y);
        tmp_distance = sqrt(pow(client_x - florist_x, 2) + pow(client_y - florist_y, 2));
        //fprintf(stderr, "client%d to florist%d is %lf \n",client_no, i, tmp_distance);
        if( (distance > tmp_distance)
            && (strstr(florists[i].flowers_for_sale, clients[client_no].desired_flower) != NULL) ){
            distance = tmp_distance;
            clients[client_no].distance = distance;
            nearest_florist_index = i;
        }
    }
    // fprintf(stderr, ">>client%d to florist%d is %lf \n",client_no, nearest_florist_index, distance);
    // return the index of nearest florist to the customer
    return nearest_florist_index;
}

void read_data(const char *filename, ThreadPool *pool){
    fprintf(stderr, "Processing requests\n-------------------\n");
    // first read all the data from the fle and store them in struct arrays
    fill_florists_from_file(filename);
    fill_client_from_file(filename);
    assign_customers();
    helper_print_clients();
    //helper_print_florists();
    create_queue(pool);
}

void assign_customers(){
    int i = 0;
    int val = 0;
    for(i = 0; i < number_of_clients; ++i){
        val = find_closest_florist(i);
        clients[i].assigned_to = val;
        job_list[val][waiting_jobs[val]] = clients[i].client_no;
        waiting_jobs[val] += 1;
    }
    for(i = 0; i < NUMBER_OF_FLORISTS; ++i){
        for(int j = 0; j < waiting_jobs[i]; ++j){
            //fprintf(stderr, "%d florist's jobs = %d\n", i, job_list[i][j]);
        }
    }
}

void fill_client_from_file(const char filename[]){
    int i = 0;
    int counter = 0;
    ssize_t number_of_read_characters = 0;
    int flag_for_while = TRUE;
    char tmp = '\0';
    char line[MAX_SIZE] = {};

    if((fd_data = open(filename, O_RDONLY, 0777)) < 0){
        perror("file could not be opened");
        exit(EXIT_FAILURE);
    }

    for(i = 0; i < MAX_SIZE; ++i){

        // reads a line and stores it in the char array line
        while(flag_for_while == TRUE){
            number_of_read_characters = read(fd_data, &tmp, sizeof(char));
            if(tmp != '\n')
                line[counter++] = tmp;
            else if(number_of_read_characters > 0){
                // getting a null terminated line and breaking the while loop
                line[counter] = '\0';
                counter = 0;
                flag_for_while = FALSE;
            }if(number_of_read_characters == 0){
                line[counter-1] = '\0';
                counter = 0;
                flag_for_while = FALSE;
            }
        }// while ends

        //fprintf(stderr, "%s\n", line);

        // parsing the read line and filling florist structs
        if(i > 3){
            parse_client_line(line, i-4);
            if(number_of_read_characters == 0){
                number_of_clients = i + 1 - 4;
                i = MAX_SIZE;
            }
        }

        // for readind the next line we need to reinitialize them
        flag_for_while = TRUE;
        line[0] = '\0';

    }

    close(fd_data);
}

void parse_client_line(const char *line, int index){
    char filler[MAX_SIZE] = {};
    int client_no = 0;
    int coordinate = 0;
    int counter = 0;
    int i = 0;


    // reading the name
    for(i = 0; i < strlen(line); ++i){
        if(line[i] != '(')
            filler[i] = line[i];
        else{
            filler[i] = '\0';
            break;
        }
    }
    counter = i;
    sscanf(filler, "%*[^0-9]%d", &client_no);
    clients[index].client_no = client_no;


    // reading x coordinate
    filler[0] = '\0';
    for(i = counter; i < strlen(line); ++i){

        if(line[i] != ',')
            filler[i - counter] = line[i];
        else{
            filler[i - counter] = '\0';
            break;
        }
    }
    counter = i;
    sscanf(filler, "%*[^0-9]%d", &coordinate);
    if(filler[1] == '-')
        clients[index].client_x = coordinate * -1;
    else
        clients[index].client_x = coordinate;


    // reading y coordinate
    filler[0] = '\0';
    for(i = counter; i < strlen(line); ++i){

        if(line[i] != ';')
            filler[i - counter] = line[i];
        else{
            filler[i - counter] = '\0';
            break;
        }
    }
    counter = i;
    sscanf(filler, "%*[^0-9]%d", &coordinate);
    if(filler[1] == '-')
        clients[index].client_y = coordinate * -1;
    else
        clients[index].client_y = coordinate;

    counter = 0;
    for(i = 0; i < strlen(line); ++i){
        if(line[i] != ':')
            counter++;
        else
            break;

    }
    counter = i + 1;
    for(i = counter; i < strlen(line); ++i){
        if(line[i] == ' ')
            counter++;
    }

    filler[0] = '\0';
    for(i = counter; i < strlen(line); ++i){
        filler[i-counter] = line[i];
    }
    filler[i - counter] = '\0';
    //fprintf(stderr, "%s\n", filler);
    strcpy(clients[index].desired_flower, filler);

    // set status
    clients[index].served = FALSE;
}

void helper_print_florists(){
    int i = 0;
    for(i = 0; i < NUMBER_OF_FLORISTS; ++i){
        fprintf(stderr, "---------------------------\n");
        fprintf(stderr, "Name = %s\nx = %d\ny = %d\nspeed = %.1lf\nlist of flowers = %s\n",
                florists[i].name,
                florists[i].florist_x,
                florists[i].florist_y,
                florists[i].click_speed,
                florists[i].flowers_for_sale);
    }
}

void helper_print_clients(){
    int i = 0;
    for(i = 0; i < number_of_clients; ++i){
        fprintf(stderr, "---------------------------\n");
        fprintf(stderr, "no = %d\nx = %d\ny = %d\nserved = %d\nassigned_to = %d\ndistance = %lf\ndesired flowers = %s\n",
                clients[i].client_no,
                clients[i].client_x,
                clients[i].client_y,
                clients[i].served,
                clients[i].assigned_to,
                clients[i].distance,
                clients[i].desired_flower);
    }
}

void parse_florist_line(const char *line, int index){
    char filler[MAX_SIZE] = {};
    int coordinate = 0;
    double speed = 0.0;
    int counter = 0;
    int i = 0;



    // reading the name
    for(i = 0; i < strlen(line); ++i){
        if(line[i] != '(')
            filler[i] = line[i];
        else{
            filler[i] = '\0';
            break;
        }
    }
    counter = i;
    strcpy(florists[index].name, filler);



    // reading x coordinate
    filler[0] = '\0';
    for(i = counter; i < strlen(line); ++i){

        if(line[i] != ',')
            filler[i - counter] = line[i];
        else{
            filler[i - counter] = '\0';
            break;
        }
    }
    counter = i;
    sscanf(filler, "%*[^0-9]%d", &coordinate);
    if(filler[1] == '-')
        florists[index].florist_x = coordinate * -1;
    else
        florists[index].florist_x = coordinate;


    // reading y coordinate
    filler[0] = '\0';
    for(i = counter; i < strlen(line); ++i){

        if(line[i] != ';')
            filler[i - counter] = line[i];
        else{
            filler[i - counter] = '\0';
            break;
        }
    }
    counter = i;
    sscanf(filler, "%*[^0-9]%d", &coordinate);
    if(filler[1] == '-')
        florists[index].florist_y = coordinate * -1;
    else
        florists[index].florist_y = coordinate;

    // reading speed
    filler[0] = '\0';
    for(i = counter; i < strlen(line); ++i){

        if(line[i] != ')')
            filler[i - counter] = line[i];
        else{
            filler[i - counter] = '\0';
            break;
        }
    }
    counter = i;
    sscanf(filler, "%*[^0-9]%lf", &speed);
    florists[index].click_speed = speed;


    // reading until ':'
    filler[0] = '\0';
    for(i = counter; i < strlen(line); ++i){
        if(line[i] != ':')
            filler[i - counter] = line[i];
        else{
            counter = i+1;
            filler[0] = '\0';
            break;
        }
    }

    //get rid of spaces
    filler[0] = '\0';
    for(i = counter; i < strlen(line); ++i){
        if(line[i] != ' ')
            filler[i - counter] = line[i];
        else{
            counter = i+1;
            filler[0] = '\0';
            break;
        }
    }

    filler[0] = '\0';
    for(i = counter; i < strlen(line); ++i){
        filler[i-counter] = line[i];
    }
    filler[i - counter] = '\0';

    strcpy(florists[index].flowers_for_sale, filler);
}

void fill_florists_from_file(const char filename[]){
    int i = 0;
    int counter = 0;
    int flag_for_while = TRUE;
    char tmp = '\0';
    char line[MAX_SIZE] = {};

    if((fd_data = open(filename, O_RDONLY, 0777)) < 0){
        perror("file could not be opened");
        exit(EXIT_FAILURE);
    }

    for(i = 0; i < NUMBER_OF_FLORISTS; ++i){

        // reads a line and stores it in the char array line
        while(flag_for_while == TRUE){
            read(fd_data, &tmp, sizeof(char));
            if(tmp != '\n')
                line[counter++] = tmp;
            else{
                // getting a null terminated line and breaking the while loop
                line[counter] = '\0';
                counter = 0;
                flag_for_while = FALSE;
            }
        }// while ends

        //fprintf(stderr, "line = %s\n", line);

        // parsing the read line and filling florist structs
        parse_florist_line(line, i);

        // for readind the next line we need to reinitialize them
        flag_for_while = TRUE;
        line[0] = '\0';
    }

    close(fd_data);
}

void create_queue(ThreadPool *pool){
    int i = 0;
    for(i = 0; i < NUMBER_OF_FLORISTS; ++i){
        pool[i].queue.number_of_jobs = waiting_jobs[i];
        pool[i].queue.head = (Task*)malloc(sizeof(Task));
        pool[i].queue.head->next = NULL;
        push_queue(pool, i);
    }
}
void push_queue(ThreadPool *pool, int index){
    int i = 0;
    Task *newTask = NULL;
    Task *head = pool[index].queue.head;

    // with this, we will push elements to the queue
    newTask = head;

    for(i = 0; i < waiting_jobs[index]; ++i){

        // filling the new task
        newTask->arg = job_list[index][i];

        //fprintf(stderr, "%d arg=%d\n",index, *(int*)(newTask->arg));

        // if no job left
        if(i == waiting_jobs[index] - 1) {
            newTask->next = NULL;
            // fprintf(stderr, "%d arg=%d\n",index, *(int*)(newTask->arg));
        }
        else{
            newTask->next = (Task*)malloc(sizeof(Task));
            newTask = newTask->next;
        }
    }

//    newTask = florist_thread_pool[index].queue.head;
//    // printing queue
//    while(newTask != NULL){
//        fprintf(stderr, "%d arg=%d\n",index, newTask->arg);
//        newTask = newTask->next;
//    }
}

void show_statistics(){
    int i = 0;
    for(i = 0; i < NUMBER_OF_FLORISTS; ++i){
        fprintf(stderr, "Florist %s has sold %d flowers in %.0lf ms\n",
                        florists[i].name, waiting_jobs[i], total_time[i]);
    }
}

void free_queue(ThreadPool *pool){
	int i = 0;
	Task *next= NULL;
	Task *tmp = NULL;
	for(i = 0; i < NUMBER_OF_FLORISTS; ++i){
		next = pool[i].queue.head;
		while(next != NULL){
			tmp = next;
			next = tmp->next;
			if(tmp != NULL)
				free(tmp);
		}
	}
	
}




