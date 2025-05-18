#include <stdio.h>   /* For prints     */
#include <unistd.h>  /* For pid, sleep */  

#include "wd.h"
#include "utils.h"

int main(int argc, char *argv[])
{
    wd_status_t status = WD_SUCCESS;
    int count = 1000;

    status = WDStart(2, 3, argc, argv);  

    printf(BOLD_GREEN "App is being Watched from Client\n" RESET);
    while(--count)
    {
        printf(PURPLE "count = %d\n" RESET, count);
        sleep(1);
    }

    status += WDStop();

    printf(RED"App is NOT being Watched (from Client)\n"RESET);

    if (WD_SUCCESS != status)
    {
        printf(RED"FAILED\n"RESET);
    }
    else
    {
        printf(GREEN"SUCCESS\n"RESET);
    }
    return 0;
}



