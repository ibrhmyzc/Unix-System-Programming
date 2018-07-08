#ifndef CSE344HW5_H
#define CSE344HW5_H
#include <pthread.h>

// constants
#define TRUE 1
#define FALSE 0
#define MAX_SIZE 1024
#define NUMBER_OF_FLORISTS 3
#define NAME_SIZE 32

extern pthread_mutex_t lock;
extern pthread_cond_t  cond_task_queue;
extern pthread_mutex_t cond_var_lock; 

extern int total_delivered_flower;
extern int number_of_clients;


typedef struct Task{
    int arg;
    struct Task *next;
}Task;

typedef struct TaskQueue{
    Task *head;
    int number_of_jobs;
}TaskQueue;

typedef struct ThreadPool{
    TaskQueue queue;
    pthread_t florist_thread;
    int id;
}ThreadPool;

typedef struct Florist{
    char name[NAME_SIZE];
    int florist_x;
    int florist_y;
    double click_speed;
    char flowers_for_sale[MAX_SIZE];
}Florist;

typedef struct Client{
    int client_no;
    int client_x;
    int client_y;
    int served;
    int assigned_to;
    double distance;
    double delivery_time;
    char desired_flower[MAX_SIZE];
}Client;

Florist florists[NUMBER_OF_FLORISTS];
Client clients[MAX_SIZE];
ThreadPool *florist_thread_pool;
// Functions

void fill_florists_from_file(const char filename[]);
void fill_client_from_file(const char filename[]);
void parse_florist_line(const char *line, int index);
void parse_client_line(const char *line, int index);
void helper_print_florists();
void helper_print_clients();
int find_closest_florist(int client_no);
void read_data(const char *filename, ThreadPool *pool);
void *deliver_flower(void *arg);
void assign_customers();
void create_queue(ThreadPool *pool);
void push_queue(ThreadPool *pool, int i);
void free_queue(ThreadPool *pool);
void show_statistics();
#endif
