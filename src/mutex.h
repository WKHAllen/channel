#ifndef CHANNEL_MUTEX_H
#define CHANNEL_MUTEX_H

#ifdef _WIN32
#  include <Windows.h>
#else
#  include <pthread.h>
#endif

#define CHANNEL_MUTEX_SUCCESS 0
#define CHANNEL_MUTEX_FAILURE 1

typedef struct Mutex_ {
#ifdef _WIN32
    HANDLE lock;
#else
    pthread_mutex_t lock;
#endif
} Mutex;

// Creates a new mutex object.
Mutex* new_mutex(void);

// Gets a lock on the mutex.
int mutex_lock(Mutex* mutex);

// Releases a lock on the mutex.
int mutex_release(Mutex* mutex);

// Frees the memory used by the mutex.
void free_mutex(Mutex* mutex);

#endif // CHANNEL_MUTEX_H
