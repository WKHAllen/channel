#include "threading.h"
#include <stdlib.h>

#define NEW(T) ((T*)malloc(sizeof(T)))

typedef struct ThreadSpawnInfo_ {
    void (*f)(void*);
    void* arg;
} ThreadSpawnInfo;

#ifdef _WIN32
DWORD WINAPI do_thread_spawn(LPVOID info)
{
    ThreadSpawnInfo* spawn_info = (ThreadSpawnInfo*)info;
    void (*f)(void*) = spawn_info->f;
    void* arg = spawn_info->arg;
    free(spawn_info);
    (*f)(arg);
    return 0;
}
#else
void* do_thread_spawn(void* info)
{
    ThreadSpawnInfo* spawn_info = (ThreadSpawnInfo*)info;
    void (*f)(void*) = spawn_info->f;
    void* arg = spawn_info->arg;
    free(spawn_info);
    (*f)(arg);
    return NULL;
}
#endif

JoinHandle* thread_spawn(void (*f)(void*), void* arg)
{
    ThreadSpawnInfo* info = NEW(ThreadSpawnInfo);
    info->f = f;
    info->arg = arg;

#ifdef _WIN32
    HANDLE thread = CreateThread(NULL, 0, do_thread_spawn, info, 0, NULL);

    if (thread == NULL) {
        return NULL;
    }
#else
    pthread_t thread;
    int return_code = pthread_create(&thread, NULL, do_thread_spawn, info);

    if (return_code != 0) {
        return NULL;
    }
#endif

    JoinHandle* handle = NEW(JoinHandle);
    handle->handle = thread;
    return handle;
}

int thread_join(JoinHandle* handle)
{
#ifdef _WIN32
    DWORD ret = WaitForSingleObject(handle->handle, INFINITE);
    CloseHandle(handle->handle);
    free(handle);

    if (ret == WAIT_OBJECT_0) {
        return CHANNEL_TEST_THREADING_SUCCESS;
    }
    else {
        return CHANNEL_TEST_THREADING_FAILURE;
    }
#else
    int ret = pthread_join(handle->handle, NULL);
    free(handle);

    if (ret == 0) {
        return CHANNEL_TEST_THREADING_SUCCESS;
    }
    else {
        return CHANNEL_TEST_THREADING_FAILURE;
    }
#endif
}

void thread_detach(JoinHandle* handle)
{
#ifdef _WIN32
    CloseHandle(handle->handle);
#endif
    free(handle);
}
