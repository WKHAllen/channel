#include "mutex.h"
#include "util.h"

Mutex* new_mutex(void)
{
    Mutex* mutex = NEW(Mutex);
#ifdef _WIN32
    mutex->lock = CreateMutex(NULL, FALSE, NULL);
#else
    pthread_mutex_init(&mutex->lock, NULL);
#endif

    return mutex;
}

int mutex_lock(Mutex* mutex)
{
#ifdef _WIN32
    if (WaitForSingleObject(mutex->lock, INFINITE) == WAIT_OBJECT_0) {
        return CHANNEL_MUTEX_SUCCESS;
    }
    else {
        return CHANNEL_MUTEX_FAILURE;
    }
#else
    if (pthread_mutex_lock(&mutex->lock) == 0) {
        return CHANNEL_MUTEX_SUCCESS;
    }
    else {
        return CHANNEL_MUTEX_FAILURE;
    }
#endif
}

int mutex_release(Mutex* mutex)
{
#ifdef _WIN32
    if (ReleaseMutex(mutex->lock)) {
        return CHANNEL_MUTEX_SUCCESS;
    }
    else {
        return CHANNEL_MUTEX_FAILURE;
    }
#else
    if (pthread_mutex_unlock(&mutex->lock) == 0) {
        return CHANNEL_MUTEX_SUCCESS;
    }
    else {
        return CHANNEL_MUTEX_FAILURE;
    }
#endif
}

void free_mutex(Mutex* mutex)
{
#ifdef _WIN32
    CloseHandle(mutex->lock);
#else
    pthread_mutex_destroy(&mutex->lock);
#endif

    free(mutex);
}
