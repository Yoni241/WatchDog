#include <stdio.h>       /* For printf perror        */
#include <unistd.h>      /* For fork, execv, getpid  */
#include <sys/ipc.h>     /* For ftok                 */
#include <sys/shm.h>     /* For shmget, shmat, shmdt */
#include <time.h>        /* For time types           */

#include "wd.h"
#include "utils.h"

#define SHM_SIZE sizeof(shared_mem_data_t)
char* wd_exe = "exe/wd_exe";

typedef struct 
{
    time_t interval_in_sec;
    size_t intervals_per_check;
} shared_mem_data_t;

/* Function to read shared memory */
static void ReadFromSharedMem(time_t* interval_in_sec, 
                                                    size_t* intervals_per_check)
{
    key_t key = 0;
    int block_id = 0;
    shared_mem_data_t* shared_data;
    char* shared_memory = NULL;

    key = ftok(wd_exe, 0);
    if (key == -1) 
    {
        perror(RED"ftok"RESET);
        return;
    }

    block_id = shmget(key, SHM_SIZE, 0644 | IPC_CREAT);
    if (block_id == -1) 
    {
        perror(RED"shmget"RESET);
        return;
    }

    shared_memory = shmat(block_id, NULL, 0);
    if (shared_memory == (char*) -1) 
    {
        perror(RED"shmat"RESET);
        return;
    }

    shared_data = (shared_mem_data_t*)shared_memory;
    *interval_in_sec = shared_data->interval_in_sec;
    *intervals_per_check = shared_data->intervals_per_check;

    if (shmdt(shared_memory) == -1) 
    {
        perror(RED"shmdt"RESET);
        return;
    }
}

int main(int argc, char *argv[])
{
    time_t interval_in_sec = 0;
    size_t intervals_per_check = 0;

    ReadFromSharedMem(&interval_in_sec, &intervals_per_check);

    WDStart(interval_in_sec, intervals_per_check, argc, argv);

    printf(BOLD_GREEN "App is being Watched from WD\n" RESET);

    while (1) 
    {
        sleep(1);
    }
    
    return 0;
}
