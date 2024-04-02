#include "util.h"

#ifdef _WIN32
#  include <Windows.h>
#else
#  include <time.h>
#endif

void channel_sleep(double seconds)
{
#ifdef _WIN32
    Sleep(seconds * 1000);
#else
    struct timespec ts;
    ts.tv_sec = seconds;
    ts.tv_nsec = ((int)(seconds * 1000) % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}

void channel_wait(void)
{
    channel_sleep(CHANNEL_SLEEP_TIME);
}
