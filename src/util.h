#ifndef CHANNEL_UTIL_H
#define CHANNEL_UTIL_H

#include <stdlib.h>

#define NEW(T) ((T*)malloc(sizeof(T)))
#define NEW_N(T, n) ((T*)malloc((n) * sizeof(T)))

#define CHANNEL_SLEEP_TIME 0.001

// Sleeps for the provided number of seconds.
void channel_sleep(double seconds);

// Sleeps for the number of seconds specified by `CHANNEL_SLEEP_TIME`.
void channel_wait(void);

#endif // CHANNEL_UTIL_H
