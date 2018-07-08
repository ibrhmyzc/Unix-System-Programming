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
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>

#define NUMBER_OF_CHEFS 6
#define SHARED_MEMORY_SIZE 3       // stores only 2 char and null
#define TRUE 1                     // to use True
#define SNAME "/ibrhmyzc"

// global variables
int shm_chef_value[6];
sem_t *sem_chefs[NUMBER_OF_CHEFS];	   // semaphore for each ingredient
int shm_wholesaler_value;
sem_t *sem_wholesaler;			   // semaphore for the cake
key_t shared_memory_key = 123;     // key for shared memory
int sem_id = 0;                    // keeps the semaphore id
char *what_wholesaler_brings = NULL;    // shared memory
// stand for eggs, flour, butter and sugar respectively
const char WHAT_CHEFS_NEED[6][3] = {"ef\0", "eb\0", "es\0", "fb\0", "fs\0", "bs\0"};
const char WHAT_CHEFS_NEED_REVERSED[6][3] = {"fe\0", "be\0", "se\0", "bf\0", "sf\0", "sb\0"};  
pid_t pids[7]; 					   // pids of all processes. it will be used for signal handling
pid_t parent_pid;

// functions
void wholesaler();
void fill_random_ingredients();
void generate_two_different_numbers(int*, int*);
void chef(int no);
void handler(int nix);

int main(int argc, char **argv) {
    int i = 0;           // used in the for loop
	char sem_name[32];
	pid_t child_pid = 0;
	pids[i] = getpid();
	// Usage
	if(argc != 1){
		perror("Usage: ./sekerpare");
		exit(1);
	}
	
	// creating a segment
    if ((sem_id = shmget(shared_memory_key, SHARED_MEMORY_SIZE, IPC_CREAT | 0666)) < 0){
        perror("error in creating a segment\n");
        exit(1);
    }

    // attaching the segment to the data space
    if((what_wholesaler_brings = shmat(sem_id, NULL, 0)) == (char *) -1){
        perror("error in attaching/n");
        exit(1);
    }
	
	// semaphore for wholesaler http://pubs.opengroup.org/onlinepubs/009604599/functions/shm_open.html
	if((shm_wholesaler_value = shm_open("/wholesaler", O_RDWR | O_CREAT, S_IRWXU)) < 0){
		perror("error in wholesaler sem_open");
		exit(1);
	}
	
	ftruncate(shm_wholesaler_value, sizeof(sem_t));
	sem_wholesaler = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_wholesaler_value, 0);
	
	if((sem_init(sem_wholesaler, 1, 0)) < 0){
		perror("error in wholesaler sem_init");
		exit(1);
	}
	
    // for generating random numbers
    srand(time(NULL));

	what_wholesaler_brings[0] = '\0';
	
    // Creates as many processes as the number of chefs
    for(i = 0; i < NUMBER_OF_CHEFS; ++i)
        if((child_pid = fork()) == 0)
            break;
	
	// handler
	signal(SIGINT, handler);
	pids[i] = getpid();
	//fprintf(stderr, "pids[%d] = %d\n", i, pids[i]);
    switch(child_pid){
        case -1:
            perror("fork error\n");
            exit(1);
        case 0:         // child process
            //fprintf(stderr, "chef%d with pid=%d is created\n", i, getpid());
           
			// starting chef with his number
            chef(i);
            exit(0);
        default:
            pids[6] = getpid(); 
			//fprintf(stderr, "wholesaler with pid=%d is running\n", getpid());
            wholesaler();
            wait(NULL); // parent waits all of its children processes
            break;
    }


	sem_destroy(sem_wholesaler);
	shmdt(what_wholesaler_brings);
	shmctl(sem_id, IPC_RMID, NULL);
    fprintf(stderr, "Program terminated in pid %d\n", getpid());
    return 0;
}

void handler(int nix){
	int k = 0;
	fprintf(stderr, "control c is caught\n");
	// destroying semaphores
	if(parent_pid == getpid()){
		sem_destroy(sem_wholesaler);
		shmdt(what_wholesaler_brings);
		shmctl(sem_id, IPC_RMID, NULL);
		for(k = 0; k < 6; ++k){
			if(pids[k] != getpid()){
				fprintf(stderr, "%d\n", pids[k]);
				kill(pids[k], SIGINT);
			}	
		}
	}else{
		kill(parent_pid, SIGINT);
	}
	
	
	exit(0);
}

void wholesaler(){
    while(TRUE){
		// randomly brings two ingredients
		fill_random_ingredients();
		fprintf(stderr, "wholesaler produced %c and %c\n", what_wholesaler_brings[0], what_wholesaler_brings[1]);
		fprintf(stderr, "wholesaler waits\n");
		sem_wait(sem_wholesaler);
    }
}

void fill_random_ingredients(){
	int no = 0;

    const char ingredients[4] = {'e', 'f', 'b', 's'}; // stand for eggs, flour, butter and sugar respectively
    int number1 = 0, number2 = 0;

    // generating a random number from 0 to 3
    generate_two_different_numbers(&number1, &number2);
    
    // fprintf(stderr, "generated numbers are %d and %d\n", number1, number2);
    while(what_wholesaler_brings[0] != '\0');
	fprintf(stderr, "wholesaler got the cake and left to sell it\n");

	no = what_wholesaler_brings[1] - '0';
	fprintf(stderr, "chef%d waits for %c and %c\n", no
			, WHAT_CHEFS_NEED[no][0], WHAT_CHEFS_NEED[no][1]);
    // storing two new random ingredients for the next cake
	what_wholesaler_brings[0] = ingredients[number1];
    what_wholesaler_brings[1] = ingredients[number2];
    what_wholesaler_brings[2] = '\0';
}

void generate_two_different_numbers(int *number1, int *number2){
	// number1 gets a random integer 
	*number1 = rand() % 4;
	
	// both numbers are the same
	*number2 = *number1;
	
	// as long as they have the same number, while continues
	while(*number2 == *number1){
		*number2 = rand() % 4;
	}	
}

// {"ef", "eb", "es", "fb", "fs", "bs"} respectively what the chefs needs to prepare the cake
void chef(int no){
	int for_which_chef = 0;
    while(TRUE){   
		// fprintf(stderr, "ingredients that the chef%d sees are %c and %c\n", no, what_wholesaler_brings[0], what_wholesaler_brings[1]);
		
		// if what wholesaler brings is what the chef needs
		for(for_which_chef = 0; for_which_chef < 6; for_which_chef++){
			if(strcmp(what_wholesaler_brings, WHAT_CHEFS_NEED[for_which_chef]) == 0
				|| strcmp(what_wholesaler_brings, WHAT_CHEFS_NEED_REVERSED[for_which_chef]) == 0){
				break;
			}
		}
		// chefs[no] waits for the ingredients of which he lacks
		if(for_which_chef == no){
			fprintf(stderr, "chef%d has taken the %c\n", no, what_wholesaler_brings[0]);
			fprintf(stderr, "chef%d has taken the %c\n", no, what_wholesaler_brings[1]);
			what_wholesaler_brings[0] = '\0';
			what_wholesaler_brings[1] = no + '0';
			fprintf(stderr, "chef%d has delivered the cake to wholesaler\n");
		}

		// delivers to the wholesaler in order to free him to bring more ingredients for the chefs
		sem_post(sem_wholesaler);
    }
}





