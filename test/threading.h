#ifndef CHANNEL_TEST_THREADING_H
#define CHANNEL_TEST_THREADING_H

#ifdef _WIN32
#  include <Windows.h>
#else
#  include <pthread.h>
#endif

#define CHANNEL_TEST_THREADING_SUCCESS 0
#define CHANNEL_TEST_THREADING_FAILURE 1

// A join handle for a thread.
typedef struct JoinHandle_ {
#ifdef _WIN32
    HANDLE handle;
#else
    pthread_t handle;
#endif
} JoinHandle;

// Spawns a new thread and returns a join handle to it.
JoinHandle* thread_spawn(void (*f)(void*), void* arg);

// Joins the thread and cleans up its memory.
int thread_join(JoinHandle* handle);

// Detaches the thread and drops the join handle.
void thread_detach(JoinHandle* handle);

#endif // CHANNEL_TEST_THREADING_H
