#ifndef __WD_H__
#define __WD_H__

#include <stddef.h>     /* For size_t type */
#include <time.h>       /* For time_t type */

/* 
 * This file defines the interface for the Watchdog service, which provides 
 * failure detection and automatic recovery for applications. It ensures that 
 * if the main application fails, it is restarted automatically.
 */

/* ========================================================================== */

/* 
 * Enumeration of possible statuses returned by Watchdog functions.
 * - WD_SUCCESS: Operation was successful.
 * - WD_INIT_FAIL: Initialization failed. This may occur due to errors in 
 *                 system calls such as fork, exec, or pthread_create.
 * - WD_TERMINATION_FAIL: Cleanup or termination of the Watchdog failed.
 * - WD_MEM_ALLOC_FAIL: Failure due to inability to allocate required memory.
 */
typedef enum wd_status {
    WD_SUCCESS = 0,
    WD_INIT_FAIL = 1,
    WD_TERMINATION_FAIL = 2,
    WD_MEM_ALLOC_FAIL = 3
} wd_status_t;

/* 
 * Starts the watchdog service for the calling application.
 * 
 * Parameters:
 * - interval_in_sec: The interval in seconds between each 
 *   health check by the watchdog.
 * - intervals_per_check: The number of intervals that can pass before the
 *   watchdog takes action to recover the application.
 * - argc: The count of command-line arguments.
 * - argv: The command-line arguments.
 * 
 * Returns:
 * - A status code of type wd_status_t indicating the result of the operation.
 */
wd_status_t WDStart(time_t interval_in_sec, size_t intervals_per_check, 
                                                        int argc, char* argv[]);

/* 
 * Stops the watchdog service.
 * 
 * Returns:
 * - A status code of type wd_status_t indicating the result of the operation.
 */
wd_status_t WDStop(void);

#endif /* __WD_H__ */
