/*
 * COBGDB GnuCOBOL Debugger:
 * This code is based on the GnuCOBOL Debugger extension available at:
 * https://github.com/OlegKunitsyn/gnucobol-debug
 *
 * License:
 * This code is provided without any warranty, express or implied.
 * You may modify and distribute it at your own risk.
 * 
 * Author: Marcos Martins Duma
 */
#include <stdio.h>
#include <stdlib.h>
#include "cobgdb.h"

#if defined(__linux__)
#if !defined(CLOCK_MONOTONIC)
#define CLOCK_MONOTONIC		1
#endif
#endif


/*
 * COBGDB - Platform Abstraction Layer
 *
 * Provides cross-platform implementations for:
 *   - High-resolution timing
 *   - Sleep functions
 *   - Process termination and cleanup
 *   - Mutex synchronization
 *   - Thread creation and management
 *
 * Supported platforms:
 *   - Windows
 *   - Linux / POSIX
 *
 * This module isolates operating system specific APIs from
 * the rest of the debugger codebase.
 */
double getCurrentTime() {
#ifdef _WIN32
    LARGE_INTEGER frequency, start;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    return (double)start.QuadPart / frequency.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
#endif
}

void exit_with_message(const char *msg, int code) {
    #if defined(__linux__)
    printf("\033[?1002l");
    printf("\033[?1006l");
    #endif
    printf("%s\n", msg);
    fflush(stdout);
    exit(code);
}


void sleep_ms(unsigned int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}

/* ============================================================
   MUTEX CROSS-PLATFORM
   ============================================================ */
void mutex_init(mutex_t *m) {
#ifdef _WIN32
    InitializeCriticalSection(m);
#else
    pthread_mutex_init(m, NULL);
#endif
}

void mutex_destroy(mutex_t *m) {
#ifdef _WIN32
    DeleteCriticalSection(m);
#else
    pthread_mutex_destroy(m);
#endif
}

void mutex_lock(mutex_t *m) {
#ifdef _WIN32
    EnterCriticalSection(m);
#else
    pthread_mutex_lock(m);
#endif
}

void mutex_unlock(mutex_t *m) {
#ifdef _WIN32
    LeaveCriticalSection(m);
#else
    pthread_mutex_unlock(m);
#endif
}

/* ============================================================
   THREAD CROSS-PLATFORM
   ============================================================ */

#ifdef _WIN32
unsigned __stdcall thread_start(void *arg)
#else
void *thread_start(void *arg)
#endif
{
    void (**args)(void*) = (void(**)(void*))arg;
    void (*func)(void*) = args[0];
    void *userdata      = args[1];

    free(arg);
    func(userdata);

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

void thread_create(thread_t *t, void (*func)(void*), void *userdata)
{
    void **args = malloc(2 * sizeof(void*));
    args[0] = (void*)func;
    args[1] = userdata;

#ifdef _WIN32
    *t = (HANDLE)_beginthreadex(NULL, 0, thread_start, args, 0, NULL);
#else
    pthread_create(t, NULL, thread_start, args);
#endif
}

void thread_join(thread_t t)
{
#ifdef _WIN32
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
#else
    pthread_join(t, NULL);
#endif
}
