/*
    - Title: WatchDog
	- Developer: Yehonatan Peleg
	- Reviewer: Alon Brull
	- Date: oct 4 2024
*/

#define _GNU_SOURCE
#include <pthread.h>          /* For thread functionality     */
#include <unistd.h>           /* For fork, execv, getpid      */
#include <semaphore.h>        /* For Semaphore functions      */
#include <stdio.h>            /* For perror                   */
#include <errno.h>            /* For system calls fail errors */   
#include <signal.h>           /* For sigaction, kill          */
#include <stdlib.h>           /* For getenv, setenv           */
#include <fcntl.h>            /* O_CREAT (used in sem_open)   */
#include <string.h>           /* For strcmp                   */
#include <sys/ipc.h>          /* For ftok()                   */
#include <sys/shm.h>          /* for shmget, shmat and shmctl */

#include "heap_scheduler.h"   /* For sched API  */
#include "wd.h"               /* For WD API     */
#include "utils.h"

#define WD_PID "WD_PID"       /* Env variable for Watchdog PID */
#define APP_EXE_NAME_LENGTH 256
#define ERROR 1
#define SUCCESS 0

#define UNUSED(x) (void)(x)

/* ~~~~ Shared Memory Struct ~~~~ */
typedef struct 
{
    time_t interval;
    size_t threshold;
    char app_exe[APP_EXE_NAME_LENGTH]; 
} shared_mem_data_t;
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~ static global variables ~~~~ */
static sched_t* g_sched = NULL;
static pthread_t g_thread = 0; 
static sig_atomic_t g_missed_signals = 0;
static pthread_mutex_t g_mtx = PTHREAD_MUTEX_INITIALIZER;
static sem_t *g_first_sem = NULL;  
static sem_t *g_second_sem = NULL; 
static sem_t g_local_sem = {0}; 
static char* g_wd_exe = "exe/wd_exe";
static pid_t g_partner_pid = 0;
static shared_mem_data_t* g_shared_memory = NULL;
static const char* g_first_process_sem = "cur_process_sem";
static const char* g_second_process_sem = "partner_process_sem";
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void CleanUp(void *param) /* For CleanUp in SchedAddTask */
{
	UNUSED(param);
}

static int ExecPartner(char* argv[], char* partner_exe)
{
    int status = SUCCESS;
    
    g_partner_pid = fork();
    if (-1 == g_partner_pid)
    {
        perror(RED"fork failed"RESET);
        status += ERROR;
    }
    else
    {
        setenv(WD_PID, "alive", 1);
    }

    if(0 == g_partner_pid)
    {
        printf(GREY"New Process's PID: %d\n"RESET, getpid());
        
        argv[0] = partner_exe;

        if (execv(argv[0], argv) == -1)
        {
            perror(RED"execv failed\n"RESET);
            status += ERROR;
        }       
    }

    return status;
}

static wd_status_t Revive(char* argv[])
{
    wd_status_t status = WD_SUCCESS;
    char* partner_exe = NULL;
    
    if (strcmp(argv[0], g_wd_exe) == 0)
    {
        partner_exe = g_shared_memory->app_exe;    
    }
    else
    {
        partner_exe = g_wd_exe;
    }

    if (SUCCESS != ExecPartner(argv, partner_exe))
    {
        status = WD_INIT_FAIL;
    }

    return status;
}

void ResetCounter(int signum)
{
    pthread_mutex_lock(&g_mtx);
    g_missed_signals = 0;
    printf(BOLD_GREEN"Recived SIGUSR1, in PID = %d\n"RESET, getpid());
    pthread_mutex_unlock(&g_mtx);
    UNUSED(signum);
}

void StopSchedRun(int signum)
{
    printf(BOLD_GREEN"Recived SIGUSR2, in PID = %d\n"RESET, getpid());
    SchedStop(g_sched);
    UNUSED(signum);
}

static void InitSignals(void)
{
    struct sigaction sa_usr1 = {0};
    struct sigaction sa_usr2 = {0};

    /* Using memset. Init with {0} in c89 is not guaranteed */
    memset(&sa_usr1, 0, sizeof(struct sigaction));
    memset(&sa_usr2, 0, sizeof(struct sigaction));

    sa_usr1.sa_handler = ResetCounter;
    sa_usr2.sa_handler = StopSchedRun;

    sigaction(SIGUSR1, &sa_usr1, NULL);
    sigaction(SIGUSR2, &sa_usr2, NULL);
}

static wd_status_t InitNamedSemaphores(void)
{
    g_first_sem = sem_open(g_first_process_sem, O_CREAT, 0644, 1); 
    if(g_first_sem == SEM_FAILED)
    {
        perror(RED"sem_open failed(first_process_sem)\n"RESET);

        return WD_INIT_FAIL;
    }

    g_second_sem = sem_open(g_second_process_sem, O_CREAT, 0644, 0);
    if(g_second_sem == SEM_FAILED)
    {
        perror(RED"sem_open failed(second_process_sem)\n"RESET);

        return WD_INIT_FAIL;
    }

    return WD_SUCCESS;
}

static wd_status_t SyncProcesses(char* argv[])
{
    printf(PURPLE"SyncProcesses in PID:%d argv[0]%s ", getpid(), argv[0]);

    if (strcmp(argv[0], g_wd_exe) == 0)
    {
        /* wd process */
        printf("in %d, post on %d then wait\n"RESET, getpid(), g_partner_pid);
        sem_post(g_first_sem);

        if (sem_wait(g_second_sem) == -1)
        {
            perror(RED"sem_wait failed"RESET);
            return WD_INIT_FAIL;
        }
    }
    else
    {
        printf("in %d, wait till %d will post\n"RESET, getpid(), g_partner_pid);
        if (sem_wait(g_first_sem) == -1)
        {
            perror(RED"sem_wait failed"RESET);
            return WD_INIT_FAIL;
        }

        sem_post(g_second_sem);
    }
    return WD_SUCCESS;
}

static int ImAlive(void *param)
{
    printf(YELLOW"Im Alive: PID: %d Sending signal to %d\n"RESET, getpid(),
                                                                 g_partner_pid);
    pthread_mutex_lock(&g_mtx);                                                             
    ++g_missed_signals;

    if (kill(g_partner_pid, SIGUSR1) == -1)
    {
        perror(RED"Failed to send SIGUSR1 to partner"RESET);
    }
    pthread_mutex_unlock(&g_mtx);
    UNUSED(param);

    return 1; 
}

static int AreYouAlive(void* param)
{
    pthread_mutex_lock(&g_mtx);
    printf(MAG"AreYouAlive - in PID %d | missed_signals: %u\n"RESET, getpid(), g_missed_signals);  
    
    if (g_missed_signals > (int) g_shared_memory->threshold)
    {
        printf(BLUE"parter (%d) is dead\n"RESET, g_partner_pid);

        if(SUCCESS == Revive(param))
        {
            g_missed_signals = 0;
            SyncProcesses(param);
        }
    }

    pthread_mutex_unlock(&g_mtx);

    return 1;
}

static void CleanUpSemaphores(void)
{
    sem_close(g_first_sem);
    sem_close(g_second_sem);

    sem_unlink(g_first_process_sem);
    sem_unlink(g_second_process_sem);
}

static wd_status_t InitScheduler(void* arg)
{
    size_t seconds_to_run = g_shared_memory->interval;

    g_sched = SchedCreate();
    if (NULL == g_sched)
    {
        perror(RED"Scheduler creation failed"RESET);
        return WD_MEM_ALLOC_FAIL;
    } 

    /* Task 1: "Are you alive?" */
    if (UIDIsSame(SchedAddTask(g_sched, AreYouAlive, time(NULL) + 2,
                         seconds_to_run + 1, arg, &CleanUp, NULL), UIDBadUID()))
    {
        perror(RED"Failed to add 'AreYouAlive' task to scheduler"RESET);
        SchedDestroy(g_sched);
        g_sched = NULL;  
        return WD_MEM_ALLOC_FAIL;
    }

    /* Task 2: "I'm alive!" */
    if (UIDIsSame(SchedAddTask(g_sched, ImAlive, time(NULL) + 1, seconds_to_run, 
                                             arg, &CleanUp, NULL), UIDBadUID()))
    {
        perror(RED"Failed to add 'ImAlive' task to scheduler"RESET);
        SchedDestroy(g_sched);
        g_sched = NULL;  
        return WD_MEM_ALLOC_FAIL;
    }

    return WD_SUCCESS;
}

void* HandleThread(void* arg)
{
    wd_status_t status;

    status = InitScheduler(arg);
    if (status != WD_SUCCESS)
    {
        return (void*) status;  
    }

    if (SyncProcesses(arg) == SUCCESS)
    {
        sem_post(&g_local_sem);
        SchedRun(g_sched);
    }    

    SchedDestroy(g_sched);

    return NULL;
}
  
static int GetAccessToSharedMem(void) 
{
    key_t key = 0;
    int shmid = 0;

    key = ftok(g_wd_exe, 0);
    if (key == -1) 
    {
        perror(RED"ftok failed"RESET);
        return ERROR;
    }

    shmid = shmget(key, sizeof(shared_mem_data_t), 0644); 
    if (shmid == -1) 
    {
        perror(RED"shmget failed"RESET);
        return ERROR;
    }

    g_shared_memory = (shared_mem_data_t*) shmat(shmid, NULL, 0);
    if (g_shared_memory == (void*) -1) 
    {
        perror(RED"shmat failed"RESET);
        return ERROR;
    }

    return SUCCESS;
}

static int WriteToSharedMem(time_t interval_in_sec, size_t intervals_per_check,
                                                                  char *app_exe) 
{
    key_t key;
    int block_id = 0;
    shared_mem_data_t *sm_data = NULL;

    key = ftok(g_wd_exe, 0);
    if (-1 == key) 
    {
        perror(RED"ftok failed"RESET);
        return ERROR;
    }

    block_id = shmget(key, sizeof(shared_mem_data_t), 0644 | IPC_CREAT);
    if (-1 == block_id) 
    {
        perror(RED"shmget failed"RESET);
        return ERROR;
    }

    g_shared_memory = (shared_mem_data_t *)shmat(block_id, NULL, 0);
    if (g_shared_memory == (void *)-1) 
    {
        perror(RED"shmat failed"RESET);
        return ERROR;
    }

    sm_data = g_shared_memory;
    sm_data->interval = interval_in_sec;
    sm_data->threshold = intervals_per_check;
    strncpy(sm_data->app_exe, app_exe, APP_EXE_NAME_LENGTH - 1); 

    return SUCCESS;
}

static wd_status_t InitWDComponents(void) 
{
    wd_status_t status = WD_SUCCESS;

    if (WD_SUCCESS != (status = InitNamedSemaphores())) 
    {
        return status;
    }

    InitSignals();

    return WD_SUCCESS;
}

static wd_status_t InitPartner(time_t interval, size_t threshold, char* argv[]) 
{
    wd_status_t status = WD_SUCCESS;

    if (getenv(WD_PID) == NULL) 
    {
        if (SUCCESS != (status = WriteToSharedMem(interval, threshold, 
                                                                      argv[0]))) 
        {
            CleanUpSemaphores();
            return status;
        }
        if (WD_SUCCESS != (status = Revive(argv))) 
        {
            CleanUpSemaphores();
            shmdt(g_shared_memory);        
            return status;
        }
    } 
    else 
    {
        /* 
            - The g_partner_pid is initialized to 0.
            - It can be assumed that the process running here has been revived,
              so its partner must be its parent.
            - The g_partner_pid of the parent is assigned in the Revive 
              function of the child. This is how the process identifies 
              its partner's PID.
        */
        g_partner_pid = getppid();

        if (SUCCESS != (status = GetAccessToSharedMem())) 
        {
            CleanUpSemaphores();
            shmdt(g_shared_memory);        

            return status;
        }
    }

    return WD_SUCCESS;
}

wd_status_t WDStart(time_t interval_in_sec, size_t intervals_per_check, 
                                                         int argc, char* argv[])
{
    wd_status_t status = WD_SUCCESS;

    if (WD_SUCCESS != 
        (status = InitWDComponents()))
    {
        return status;
    }

    if (WD_SUCCESS != (status = InitPartner(interval_in_sec, 
                                                    intervals_per_check, argv))) 
    {
        return status;
    }

    if (SUCCESS != pthread_create(&g_thread, NULL, &HandleThread, argv))
    {
        perror(RED"pthread_create failed"RESET);
        CleanUpSemaphores();
        shmdt(g_shared_memory);        
        pthread_mutex_destroy(&g_mtx);

        return WD_INIT_FAIL;
    }

    sem_wait(&g_local_sem);
    sem_destroy(&g_local_sem);

    (void)(argc);

    return WD_SUCCESS;
}
    
wd_status_t WDStop(void)
{
    int status = WD_SUCCESS;

    if(0 != kill(g_partner_pid, SIGUSR2))
    {
        status = WD_TERMINATION_FAIL;
    }

    pthread_kill(g_thread, SIGUSR2);
    
    if (pthread_join(g_thread, NULL) != 0)
    {
        perror(RED"pthread_join failed in WDStop"RESET);
        status = WD_TERMINATION_FAIL;
    }
    
    CleanUpSemaphores();
    shmdt(g_shared_memory);
    pthread_mutex_destroy(&g_mtx);

    return status;
}