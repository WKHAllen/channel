#include "../src/channel.h"
#include "threading.h"
#include <stdio.h>

#ifdef _WIN32
#  include <Windows.h>
#else
#  include <time.h>
#endif

#define STR_SIZE(s) ((strlen(s) + 1) * sizeof(char))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define NEW(T) ((T*)malloc(sizeof(T)))

#define TEST_ASSERT(condition) { \
    if (!(condition)) { \
        fprintf( \
            stderr, \
            "test assertion failed (%s:%d): %s\n", \
            __FILE__, __LINE__, #condition); \
        exit(1); \
    } \
}

#define TEST_ASSERT_EQ(a, b) { \
    if (a != b) { \
        fprintf( \
            stderr, \
            "test equality assertion failed (%s:%d): %s != %s, %" PRI_SIZE_T " != %" PRI_SIZE_T "\n", \
            __FILE__, __LINE__, #a, #b, a, b); \
        exit(1); \
    } \
}

#define TEST_ASSERT_NE(a, b) { \
    if (a == b) { \
        fprintf( \
            stderr, \
            "test inequality assertion failed (%s:%d): %s == %s, %" PRI_SIZE_T " == %" PRI_SIZE_T "\n", \
            __FILE__, __LINE__, #a, #b, a, b); \
        exit(1); \
    } \
}

#define TEST_ASSERT_INT_EQ(a, b) { \
    if (a != b) { \
        fprintf( \
            stderr, \
            "test equality assertion failed (%s:%d): %s != %s, %d != %d\n", \
            __FILE__, __LINE__, #a, #b, a, b); \
        exit(1); \
    } \
}

#define TEST_ASSERT_INT_NE(a, b) { \
    if (a == b) { \
        fprintf( \
            stderr, \
            "test inequality assertion failed (%s:%d): %s == %s, %d == %d\n", \
            __FILE__, __LINE__, #a, #b, a, b); \
        exit(1); \
    } \
}

#define TEST_ASSERT_STR_EQ(a, b) { \
    if (strcmp(a, b) != 0) { \
        fprintf( \
            stderr, \
            "test string equality assertion failed (%s:%d): %s != %s, %s != %s\n", \
            __FILE__, __LINE__, #a, #b, a, b); \
        exit(1); \
    } \
}

#define TEST_ASSERT_STR_NE(a, b) { \
    if (strcmp(a, b) == 0) { \
        fprintf( \
            stderr, \
            "test string inequality assertion failed (%s:%d): %s == %s, %s == %s\n", \
            __FILE__, __LINE__, #a, #b, a, b); \
        exit(1); \
    } \
}

#define TEST_ASSERT_ARRAY_EQ(a, b, size) { \
    for (size_t i = 0; i < size; i++) { \
        if (a[i] != b[i]) { \
            fprintf( \
                stderr, \
                "test array equality assertion failed (%s:%d): %s != %s at index %" PRI_SIZE_T ", with size = %" PRI_SIZE_T "\n", \
                __FILE__, __LINE__, #a, #b, i, size); \
            exit(1); \
        } \
    } \
}

#define TEST_ASSERT_MEM_EQ(a, b, size) { \
    if (memcmp(a, b, size) != 0) { \
        fprintf( \
            stderr, \
            "test memory equality assertion failed (%s:%d): %s != %s, with size = %" PRI_SIZE_T "\n", \
            __FILE__, __LINE__, #a, #b, size); \
        exit(1); \
    } \
}

#define TEST_ASSERT_MEM_NE(a, b, size) { \
    if (memcmp(a, b, size) == 0) { \
        fprintf( \
            stderr, \
            "test memory inequality assertion failed (%s:%d): %s == %s, with size = %" PRI_SIZE_T "\n", \
            __FILE__, __LINE__, #a, #b, size); \
        exit(1); \
    } \
}


#define TEST_ASSERT_ARRAY_MEM_EQ(a, b, size, item_size) { \
    for (size_t i = 0; i < size; i++) { \
        if (memcmp((void *) (a[i]), (void *) (b[i]), item_size) != 0) { \
            fprintf( \
                stderr, \
                "test array memory equality assertion failed (%s:%d): %s != %s at index %" PRI_SIZE_T ", with size = %" PRI_SIZE_T " and item size = %" PRI_SIZE_T "\n", \
                __FILE__, __LINE__, #a, #b, i, size, item_size); \
            exit(1); \
        } \
    } \
}

// Sleep for the provided number of seconds.
void test_sleep(double seconds)
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

// Helper for `test_rendezvous_channel`.
void test_rendezvous_channel_helper(void* channel_vp)
{
    RendezvousChannel* channel = (RendezvousChannel*)channel_vp;

    int* msg1 = NEW(int);
    *msg1 = 5;
    int* msg2 = NEW(int);
    *msg2 = 6;
    int* msg3 = NEW(int);
    *msg3 = 7;

    TEST_ASSERT_INT_EQ(rendezvous_send_c(channel, msg1), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(rendezvous_send_c(channel, msg2), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(rendezvous_send_c(channel, msg3), CHANNEL_SUCCESS);
}

// Test general rendezvous channel operations.
void test_rendezvous_channel(void)
{
    RendezvousChannel* channel = rendezvous_channel();

    JoinHandle* handle = thread_spawn(test_rendezvous_channel_helper, channel);

    void* recv1 = rendezvous_recv_c(channel);
    void* recv2 = rendezvous_recv_c(channel);
    void* recv3 = rendezvous_recv_c(channel);

    TEST_ASSERT(recv1 != NULL);
    TEST_ASSERT(recv2 != NULL);
    TEST_ASSERT(recv3 != NULL);
    TEST_ASSERT_INT_EQ(*((int*)(recv1)), 5);
    TEST_ASSERT_INT_EQ(*((int*)(recv2)), 6);
    TEST_ASSERT_INT_EQ(*((int*)(recv3)), 7);

    thread_join(handle);

    free(recv1);
    free(recv2);
    free(recv3);

    free_rendezvous_channel(channel);
}

// Helper for `test_rendezvous_sender_receiver`.
void test_rendezvous_sender_receiver_helper(void* sender_vp)
{
    RendezvousSender* sender = (RendezvousSender*)sender_vp;

    int* msg1 = NEW(int);
    *msg1 = 5;
    int* msg2 = NEW(int);
    *msg2 = 6;
    int* msg3 = NEW(int);
    *msg3 = 7;

    TEST_ASSERT_INT_EQ(rendezvous_send(sender, msg1), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(rendezvous_send(sender, msg2), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(rendezvous_send(sender, msg3), CHANNEL_SUCCESS);
}

// Test general rendezvous channel operations, using sender and receiver.
void test_rendezvous_sender_receiver(void)
{
    RendezvousChannel* channel = rendezvous_channel();
    RendezvousSender* sender = channel->sender;
    RendezvousReceiver* receiver = channel->receiver;
    free_rendezvous_channel_wrapper(channel);

    JoinHandle* handle = thread_spawn(test_rendezvous_sender_receiver_helper, sender);

    void* recv1 = rendezvous_recv(receiver);
    void* recv2 = rendezvous_recv(receiver);
    void* recv3 = rendezvous_recv(receiver);

    TEST_ASSERT(recv1 != NULL);
    TEST_ASSERT(recv2 != NULL);
    TEST_ASSERT(recv3 != NULL);
    TEST_ASSERT_INT_EQ(*((int*)(recv1)), 5);
    TEST_ASSERT_INT_EQ(*((int*)(recv2)), 6);
    TEST_ASSERT_INT_EQ(*((int*)(recv3)), 7);

    thread_join(handle);

    free(recv1);
    free(recv2);
    free(recv3);

    free_rendezvous_sender(sender);
    free_rendezvous_receiver(receiver);
}

// Helper for `test_rendezvous_sender_closed`.
void test_rendezvous_sender_closed_helper(void* sender_vp)
{
    RendezvousSender* sender = (RendezvousSender*)sender_vp;

    int* msg1 = NEW(int);
    *msg1 = 5;
    int* msg2 = NEW(int);
    *msg2 = 6;
    int* msg3 = NEW(int);
    *msg3 = 7;

    TEST_ASSERT_INT_EQ(rendezvous_send(sender, msg1), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(rendezvous_send(sender, msg2), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(rendezvous_send(sender, msg3), CHANNEL_SUCCESS);

    free_rendezvous_sender(sender);
}

// Test rendezvous channel sender closing detection.
void test_rendezvous_sender_closed(void)
{
    RendezvousChannel* channel = rendezvous_channel();
    RendezvousSender* sender = channel->sender;
    RendezvousReceiver* receiver = channel->receiver;
    free_rendezvous_channel_wrapper(channel);

    JoinHandle* handle = thread_spawn(test_rendezvous_sender_closed_helper, sender);

    void* recv1 = rendezvous_recv(receiver);
    void* recv2 = rendezvous_recv(receiver);
    void* recv3 = rendezvous_recv(receiver);

    TEST_ASSERT(recv1 != NULL);
    TEST_ASSERT(recv2 != NULL);
    TEST_ASSERT(recv3 != NULL);
    TEST_ASSERT_INT_EQ(*((int*)(recv1)), 5);
    TEST_ASSERT_INT_EQ(*((int*)(recv2)), 6);
    TEST_ASSERT_INT_EQ(*((int*)(recv3)), 7);

    void* recv4 = rendezvous_recv(receiver);

    TEST_ASSERT(recv4 == NULL);

    thread_join(handle);

    free(recv1);
    free(recv2);
    free(recv3);

    free_rendezvous_receiver(receiver);
}

// Helper for `test_rendezvous_receiver_closed`.
void test_rendezvous_receiver_closed_helper(void* receiver_vp)
{
    RendezvousReceiver* receiver = (RendezvousReceiver*)receiver_vp;

    void* recv1 = rendezvous_recv(receiver);
    void* recv2 = rendezvous_recv(receiver);
    void* recv3 = rendezvous_recv(receiver);

    TEST_ASSERT(recv1 != NULL);
    TEST_ASSERT(recv2 != NULL);
    TEST_ASSERT(recv3 != NULL);
    TEST_ASSERT_INT_EQ(*((int*)(recv1)), 5);
    TEST_ASSERT_INT_EQ(*((int*)(recv2)), 6);
    TEST_ASSERT_INT_EQ(*((int*)(recv3)), 7);

    free_rendezvous_receiver(receiver);
}

// Test rendezvous channel receiver closing detection.
void test_rendezvous_receiver_closed(void)
{
    RendezvousChannel* channel = rendezvous_channel();
    RendezvousSender* sender = channel->sender;
    RendezvousReceiver* receiver = channel->receiver;
    free_rendezvous_channel_wrapper(channel);

    JoinHandle* handle = thread_spawn(test_rendezvous_receiver_closed_helper, receiver);

    int msg1 = 5;
    int msg2 = 6;
    int msg3 = 7;

    TEST_ASSERT_INT_EQ(rendezvous_send(sender, &msg1), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(rendezvous_send(sender, &msg2), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(rendezvous_send(sender, &msg3), CHANNEL_SUCCESS);

    thread_join(handle);

    int msg4 = 8;

    TEST_ASSERT_INT_EQ(rendezvous_send(sender, &msg4), CHANNEL_CLOSED);

    free_rendezvous_sender(sender);
}

// Helper for `test_rendezvous_multiple_senders`.
void test_rendezvous_multiple_senders_helper1(void* sender_vp)
{
    RendezvousSender* sender = (RendezvousSender*)sender_vp;

    test_sleep(0.3);

    int* msg = NEW(int);
    *msg = 5;
    TEST_ASSERT_INT_EQ(rendezvous_send(sender, msg), CHANNEL_SUCCESS);
}

// Helper for `test_rendezvous_multiple_senders`.
void test_rendezvous_multiple_senders_helper2(void* sender_vp)
{
    RendezvousSender* sender = (RendezvousSender*)sender_vp;

    test_sleep(0.1);

    int* msg = NEW(int);
    *msg = 6;
    TEST_ASSERT_INT_EQ(rendezvous_send(sender, msg), CHANNEL_SUCCESS);
}

// Helper for `test_rendezvous_multiple_senders`.
void test_rendezvous_multiple_senders_helper3(void* sender_vp)
{
    RendezvousSender* sender = (RendezvousSender*)sender_vp;

    test_sleep(0.2);

    int* msg = NEW(int);
    *msg = 7;
    TEST_ASSERT_INT_EQ(rendezvous_send(sender, msg), CHANNEL_SUCCESS);
}

// Test rendezvous channel with multiple senders.
void test_rendezvous_multiple_senders(void)
{
    RendezvousChannel* channel = rendezvous_channel();
    RendezvousSender* sender = channel->sender;
    RendezvousReceiver* receiver = channel->receiver;
    free_rendezvous_channel_wrapper(channel);

    JoinHandle* handle1 = thread_spawn(test_rendezvous_multiple_senders_helper1, sender);
    JoinHandle* handle2 = thread_spawn(test_rendezvous_multiple_senders_helper2, sender);
    JoinHandle* handle3 = thread_spawn(test_rendezvous_multiple_senders_helper3, sender);

    void* recv1 = rendezvous_recv(receiver);
    void* recv2 = rendezvous_recv(receiver);
    void* recv3 = rendezvous_recv(receiver);

    TEST_ASSERT(recv1 != NULL);
    TEST_ASSERT(recv2 != NULL);
    TEST_ASSERT(recv3 != NULL);
    TEST_ASSERT_INT_EQ(*((int*)(recv1)), 6);
    TEST_ASSERT_INT_EQ(*((int*)(recv2)), 7);
    TEST_ASSERT_INT_EQ(*((int*)(recv3)), 5);

    thread_join(handle1);
    thread_join(handle2);
    thread_join(handle3);

    free(recv1);
    free(recv2);
    free(recv3);

    free_rendezvous_sender(sender);
    free_rendezvous_receiver(receiver);
}

// Test general bounded channel operations.
void test_bounded_channel(void)
{
    BoundedChannel* channel = bounded_channel(3);

    int msg1 = 5;
    int msg2 = 6;
    int msg3 = 7;

    TEST_ASSERT_INT_EQ(bounded_send_c(channel, &msg1), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(bounded_send_c(channel, &msg2), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(bounded_send_c(channel, &msg3), CHANNEL_SUCCESS);

    void* recv1 = bounded_recv_c(channel);
    void* recv2 = bounded_recv_c(channel);
    void* recv3 = bounded_recv_c(channel);

    TEST_ASSERT(recv1 != NULL);
    TEST_ASSERT(recv2 != NULL);
    TEST_ASSERT(recv3 != NULL);
    TEST_ASSERT_INT_EQ(*((int*)(recv1)), msg1);
    TEST_ASSERT_INT_EQ(*((int*)(recv2)), msg2);
    TEST_ASSERT_INT_EQ(*((int*)(recv3)), msg3);

    free_bounded_channel(channel);
}

// Test general bounded channel operations, using sender and receiver.
void test_bounded_sender_receiver(void)
{
    BoundedChannel* channel = bounded_channel(3);
    BoundedSender* sender = channel->sender;
    BoundedReceiver* receiver = channel->receiver;
    free_bounded_channel_wrapper(channel);

    int msg1 = 5;
    int msg2 = 6;
    int msg3 = 7;

    TEST_ASSERT_INT_EQ(bounded_send(sender, &msg1), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(bounded_send(sender, &msg2), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(bounded_send(sender, &msg3), CHANNEL_SUCCESS);

    void* recv1 = bounded_recv(receiver);
    void* recv2 = bounded_recv(receiver);
    void* recv3 = bounded_recv(receiver);

    TEST_ASSERT(recv1 != NULL);
    TEST_ASSERT(recv2 != NULL);
    TEST_ASSERT(recv3 != NULL);
    TEST_ASSERT_INT_EQ(*((int*)(recv1)), msg1);
    TEST_ASSERT_INT_EQ(*((int*)(recv2)), msg2);
    TEST_ASSERT_INT_EQ(*((int*)(recv3)), msg3);

    free_bounded_sender(sender);
    free_bounded_receiver(receiver);
}

// Test bounded channel sender closing detection.
void test_bounded_sender_closed(void)
{
    BoundedChannel* channel = bounded_channel(3);
    BoundedSender* sender = channel->sender;
    BoundedReceiver* receiver = channel->receiver;
    free_bounded_channel_wrapper(channel);

    int msg1 = 5;
    int msg2 = 6;
    int msg3 = 7;

    TEST_ASSERT_INT_EQ(bounded_send(sender, &msg1), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(bounded_send(sender, &msg2), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(bounded_send(sender, &msg3), CHANNEL_SUCCESS);

    free_bounded_sender(sender);

    void* recv1 = bounded_recv(receiver);
    void* recv2 = bounded_recv(receiver);
    void* recv3 = bounded_recv(receiver);

    TEST_ASSERT(recv1 != NULL);
    TEST_ASSERT(recv2 != NULL);
    TEST_ASSERT(recv3 != NULL);
    TEST_ASSERT_INT_EQ(*((int*)(recv1)), msg1);
    TEST_ASSERT_INT_EQ(*((int*)(recv2)), msg2);
    TEST_ASSERT_INT_EQ(*((int*)(recv3)), msg3);

    void* recv4 = bounded_recv(receiver);

    TEST_ASSERT(recv4 == NULL);

    free_bounded_receiver(receiver);
}

// Test bounded channel receiver closing detection.
void test_bounded_receiver_closed(void)
{
    BoundedChannel* channel = bounded_channel(3);
    BoundedSender* sender = channel->sender;
    BoundedReceiver* receiver = channel->receiver;
    free_bounded_channel_wrapper(channel);

    int msg1 = 5;
    int msg2 = 6;
    int msg3 = 7;

    TEST_ASSERT_INT_EQ(bounded_send(sender, &msg1), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(bounded_send(sender, &msg2), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(bounded_send(sender, &msg3), CHANNEL_SUCCESS);

    void* recv1 = bounded_recv(receiver);
    void* recv2 = bounded_recv(receiver);
    void* recv3 = bounded_recv(receiver);

    TEST_ASSERT(recv1 != NULL);
    TEST_ASSERT(recv2 != NULL);
    TEST_ASSERT(recv3 != NULL);
    TEST_ASSERT_INT_EQ(*((int*)(recv1)), msg1);
    TEST_ASSERT_INT_EQ(*((int*)(recv2)), msg2);
    TEST_ASSERT_INT_EQ(*((int*)(recv3)), msg3);

    free_bounded_receiver(receiver);

    int msg4 = 8;

    TEST_ASSERT_INT_EQ(bounded_send(sender, &msg4), CHANNEL_CLOSED);

    free_bounded_sender(sender);
}

// Helper for `test_bounded_threaded`.
void test_bounded_threaded_helper(void* receiver_vp)
{
    BoundedReceiver* receiver = (BoundedReceiver*)receiver_vp;

    void* recv1 = bounded_recv(receiver);
    void* recv2 = bounded_recv(receiver);
    void* recv3 = bounded_recv(receiver);
    void* recv4 = bounded_recv(receiver);
    void* recv5 = bounded_recv(receiver);
    void* recv6 = bounded_recv(receiver);
    void* recv7 = bounded_recv(receiver);

    TEST_ASSERT(recv1 != NULL);
    TEST_ASSERT(recv2 != NULL);
    TEST_ASSERT(recv3 != NULL);
    TEST_ASSERT(recv4 != NULL);
    TEST_ASSERT(recv5 != NULL);
    TEST_ASSERT(recv6 != NULL);
    TEST_ASSERT(recv7 != NULL);
    TEST_ASSERT_INT_EQ(*((int*)(recv1)), 3);
    TEST_ASSERT_INT_EQ(*((int*)(recv2)), 4);
    TEST_ASSERT_INT_EQ(*((int*)(recv3)), 5);
    TEST_ASSERT_INT_EQ(*((int*)(recv4)), 6);
    TEST_ASSERT_INT_EQ(*((int*)(recv5)), 7);
    TEST_ASSERT_INT_EQ(*((int*)(recv6)), 8);
    TEST_ASSERT_INT_EQ(*((int*)(recv7)), 9);
}

// Test bounded sending and receiving from different threads.
void test_bounded_threaded(void)
{
    BoundedChannel* channel = bounded_channel(3);
    BoundedSender* sender = channel->sender;
    BoundedReceiver* receiver = channel->receiver;
    free_bounded_channel_wrapper(channel);

    int msg1 = 3;
    int msg2 = 4;
    int msg3 = 5;
    int msg4 = 6;
    int msg5 = 7;
    int msg6 = 8;
    int msg7 = 9;

    JoinHandle* handle = thread_spawn(test_bounded_threaded_helper, receiver);

    test_sleep(0.1);

    TEST_ASSERT_INT_EQ(bounded_send(sender, &msg1), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(bounded_send(sender, &msg2), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(bounded_send(sender, &msg3), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(bounded_send(sender, &msg4), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(bounded_send(sender, &msg5), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(bounded_send(sender, &msg6), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(bounded_send(sender, &msg7), CHANNEL_SUCCESS);

    thread_join(handle);

    free_bounded_sender(sender);
    free_bounded_receiver(receiver);
}

// Helper for `test_bounded_multiple_senders`.
void test_bounded_multiple_senders_helper1(void* sender_vp)
{
    BoundedSender* sender = (BoundedSender*)sender_vp;

    test_sleep(0.3);

    int* msg = NEW(int);
    *msg = 5;
    TEST_ASSERT_INT_EQ(bounded_send(sender, msg), CHANNEL_SUCCESS);
}

// Helper for `test_bounded_multiple_senders`.
void test_bounded_multiple_senders_helper2(void* sender_vp)
{
    BoundedSender* sender = (BoundedSender*)sender_vp;

    test_sleep(0.1);

    int* msg = NEW(int);
    *msg = 6;
    TEST_ASSERT_INT_EQ(bounded_send(sender, msg), CHANNEL_SUCCESS);
}

// Helper for `test_bounded_multiple_senders`.
void test_bounded_multiple_senders_helper3(void* sender_vp)
{
    BoundedSender* sender = (BoundedSender*)sender_vp;

    test_sleep(0.2);

    int* msg = NEW(int);
    *msg = 7;
    TEST_ASSERT_INT_EQ(bounded_send(sender, msg), CHANNEL_SUCCESS);
}

// Test bounded channel with multiple senders.
void test_bounded_multiple_senders(void)
{
    BoundedChannel* channel = bounded_channel(3);
    BoundedSender* sender = channel->sender;
    BoundedReceiver* receiver = channel->receiver;
    free_bounded_channel_wrapper(channel);

    JoinHandle* handle1 = thread_spawn(test_bounded_multiple_senders_helper1, sender);
    JoinHandle* handle2 = thread_spawn(test_bounded_multiple_senders_helper2, sender);
    JoinHandle* handle3 = thread_spawn(test_bounded_multiple_senders_helper3, sender);

    void* recv1 = bounded_recv(receiver);
    void* recv2 = bounded_recv(receiver);
    void* recv3 = bounded_recv(receiver);

    TEST_ASSERT(recv1 != NULL);
    TEST_ASSERT(recv2 != NULL);
    TEST_ASSERT(recv3 != NULL);
    TEST_ASSERT_INT_EQ(*((int*)(recv1)), 6);
    TEST_ASSERT_INT_EQ(*((int*)(recv2)), 7);
    TEST_ASSERT_INT_EQ(*((int*)(recv3)), 5);

    thread_join(handle1);
    thread_join(handle2);
    thread_join(handle3);

    free(recv1);
    free(recv2);
    free(recv3);

    free_bounded_sender(sender);
    free_bounded_receiver(receiver);
}

// Test general unbounded channel operations.
void test_unbounded_channel(void)
{
    UnboundedChannel* channel = unbounded_channel();

    int msg1 = 5;
    int msg2 = 6;
    int msg3 = 7;
    int msg4 = 8;
    int msg5 = 9;

    TEST_ASSERT_INT_EQ(unbounded_send_c(channel, &msg1), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send_c(channel, &msg2), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send_c(channel, &msg3), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send_c(channel, &msg4), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send_c(channel, &msg5), CHANNEL_SUCCESS);

    void* recv1 = unbounded_recv_c(channel);
    void* recv2 = unbounded_recv_c(channel);
    void* recv3 = unbounded_recv_c(channel);
    void* recv4 = unbounded_recv_c(channel);
    void* recv5 = unbounded_recv_c(channel);

    TEST_ASSERT(recv1 != NULL);
    TEST_ASSERT(recv2 != NULL);
    TEST_ASSERT(recv3 != NULL);
    TEST_ASSERT(recv4 != NULL);
    TEST_ASSERT(recv5 != NULL);
    TEST_ASSERT_INT_EQ(*((int*)(recv1)), msg1);
    TEST_ASSERT_INT_EQ(*((int*)(recv2)), msg2);
    TEST_ASSERT_INT_EQ(*((int*)(recv3)), msg3);
    TEST_ASSERT_INT_EQ(*((int*)(recv4)), msg4);
    TEST_ASSERT_INT_EQ(*((int*)(recv5)), msg5);

    free_unbounded_channel(channel);
}

// Test general unbounded channel operations, using sender and receiver.
void test_unbounded_sender_receiver(void)
{
    UnboundedChannel* channel = unbounded_channel();
    UnboundedSender* sender = channel->sender;
    UnboundedReceiver* receiver = channel->receiver;
    free_unbounded_channel_wrapper(channel);

    int msg1 = 5;
    int msg2 = 6;
    int msg3 = 7;
    int msg4 = 8;
    int msg5 = 9;

    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg1), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg2), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg3), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg4), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg5), CHANNEL_SUCCESS);

    void* recv1 = unbounded_recv(receiver);
    void* recv2 = unbounded_recv(receiver);
    void* recv3 = unbounded_recv(receiver);
    void* recv4 = unbounded_recv(receiver);
    void* recv5 = unbounded_recv(receiver);

    TEST_ASSERT(recv1 != NULL);
    TEST_ASSERT(recv2 != NULL);
    TEST_ASSERT(recv3 != NULL);
    TEST_ASSERT(recv4 != NULL);
    TEST_ASSERT(recv5 != NULL);
    TEST_ASSERT_INT_EQ(*((int*)(recv1)), msg1);
    TEST_ASSERT_INT_EQ(*((int*)(recv2)), msg2);
    TEST_ASSERT_INT_EQ(*((int*)(recv3)), msg3);
    TEST_ASSERT_INT_EQ(*((int*)(recv4)), msg4);
    TEST_ASSERT_INT_EQ(*((int*)(recv5)), msg5);

    free_unbounded_sender(sender);
    free_unbounded_receiver(receiver);
}

// Test unbounded channel sender closing detection.
void test_unbounded_sender_closed(void)
{
    UnboundedChannel* channel = unbounded_channel();
    UnboundedSender* sender = channel->sender;
    UnboundedReceiver* receiver = channel->receiver;
    free_unbounded_channel_wrapper(channel);

    int msg1 = 5;
    int msg2 = 6;
    int msg3 = 7;

    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg1), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg2), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg3), CHANNEL_SUCCESS);

    free_unbounded_sender(sender);

    void* recv1 = unbounded_recv(receiver);
    void* recv2 = unbounded_recv(receiver);
    void* recv3 = unbounded_recv(receiver);

    TEST_ASSERT(recv1 != NULL);
    TEST_ASSERT(recv2 != NULL);
    TEST_ASSERT(recv3 != NULL);
    TEST_ASSERT_INT_EQ(*((int*)(recv1)), msg1);
    TEST_ASSERT_INT_EQ(*((int*)(recv2)), msg2);
    TEST_ASSERT_INT_EQ(*((int*)(recv3)), msg3);

    void* recv4 = unbounded_recv(receiver);

    TEST_ASSERT(recv4 == NULL);

    free_unbounded_receiver(receiver);
}

// Test unbounded channel receiver closing detection.
void test_unbounded_receiver_closed(void)
{
    UnboundedChannel* channel = unbounded_channel();
    UnboundedSender* sender = channel->sender;
    UnboundedReceiver* receiver = channel->receiver;
    free_unbounded_channel_wrapper(channel);

    int msg1 = 5;
    int msg2 = 6;
    int msg3 = 7;

    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg1), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg2), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg3), CHANNEL_SUCCESS);

    void* recv1 = unbounded_recv(receiver);
    void* recv2 = unbounded_recv(receiver);
    void* recv3 = unbounded_recv(receiver);

    TEST_ASSERT(recv1 != NULL);
    TEST_ASSERT(recv2 != NULL);
    TEST_ASSERT(recv3 != NULL);
    TEST_ASSERT_INT_EQ(*((int*)(recv1)), msg1);
    TEST_ASSERT_INT_EQ(*((int*)(recv2)), msg2);
    TEST_ASSERT_INT_EQ(*((int*)(recv3)), msg3);

    free_unbounded_receiver(receiver);

    int msg4 = 8;

    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg4), CHANNEL_CLOSED);

    free_unbounded_sender(sender);
}

// Helper for `test_unbounded_threaded`.
void test_unbounded_threaded_helper(void* receiver_vp)
{
    UnboundedReceiver* receiver = (UnboundedReceiver*)receiver_vp;

    void* recv1 = unbounded_recv(receiver);
    void* recv2 = unbounded_recv(receiver);
    void* recv3 = unbounded_recv(receiver);
    void* recv4 = unbounded_recv(receiver);
    void* recv5 = unbounded_recv(receiver);
    void* recv6 = unbounded_recv(receiver);
    void* recv7 = unbounded_recv(receiver);

    TEST_ASSERT(recv1 != NULL);
    TEST_ASSERT(recv2 != NULL);
    TEST_ASSERT(recv3 != NULL);
    TEST_ASSERT(recv4 != NULL);
    TEST_ASSERT(recv5 != NULL);
    TEST_ASSERT(recv6 != NULL);
    TEST_ASSERT(recv7 != NULL);
    TEST_ASSERT_INT_EQ(*((int*)(recv1)), 3);
    TEST_ASSERT_INT_EQ(*((int*)(recv2)), 4);
    TEST_ASSERT_INT_EQ(*((int*)(recv3)), 5);
    TEST_ASSERT_INT_EQ(*((int*)(recv4)), 6);
    TEST_ASSERT_INT_EQ(*((int*)(recv5)), 7);
    TEST_ASSERT_INT_EQ(*((int*)(recv6)), 8);
    TEST_ASSERT_INT_EQ(*((int*)(recv7)), 9);
}

// Test unbounded sending and receiving from different threads.
void test_unbounded_threaded(void)
{
    UnboundedChannel* channel = unbounded_channel();
    UnboundedSender* sender = channel->sender;
    UnboundedReceiver* receiver = channel->receiver;
    free_unbounded_channel_wrapper(channel);

    int msg1 = 3;
    int msg2 = 4;
    int msg3 = 5;
    int msg4 = 6;
    int msg5 = 7;
    int msg6 = 8;
    int msg7 = 9;

    JoinHandle* handle = thread_spawn(test_unbounded_threaded_helper, receiver);

    test_sleep(0.1);

    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg1), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg2), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg3), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg4), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg5), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg6), CHANNEL_SUCCESS);
    TEST_ASSERT_INT_EQ(unbounded_send(sender, &msg7), CHANNEL_SUCCESS);

    thread_join(handle);

    free_unbounded_sender(sender);
    free_unbounded_receiver(receiver);
}

// Helper for `test_unbounded_multiple_senders`.
void test_unbounded_multiple_senders_helper1(void* sender_vp)
{
    UnboundedSender* sender = (UnboundedSender*)sender_vp;

    test_sleep(0.3);

    int* msg = NEW(int);
    *msg = 5;
    TEST_ASSERT_INT_EQ(unbounded_send(sender, msg), CHANNEL_SUCCESS);
}

// Helper for `test_unbounded_multiple_senders`.
void test_unbounded_multiple_senders_helper2(void* sender_vp)
{
    UnboundedSender* sender = (UnboundedSender*)sender_vp;

    test_sleep(0.1);

    int* msg = NEW(int);
    *msg = 6;
    TEST_ASSERT_INT_EQ(unbounded_send(sender, msg), CHANNEL_SUCCESS);
}

// Helper for `test_unbounded_multiple_senders`.
void test_unbounded_multiple_senders_helper3(void* sender_vp)
{
    UnboundedSender* sender = (UnboundedSender*)sender_vp;

    test_sleep(0.2);

    int* msg = NEW(int);
    *msg = 7;
    TEST_ASSERT_INT_EQ(unbounded_send(sender, msg), CHANNEL_SUCCESS);
}

// Test unbounded channel with multiple senders.
void test_unbounded_multiple_senders(void)
{
    UnboundedChannel* channel = unbounded_channel();
    UnboundedSender* sender = channel->sender;
    UnboundedReceiver* receiver = channel->receiver;
    free_unbounded_channel_wrapper(channel);

    JoinHandle* handle1 = thread_spawn(test_unbounded_multiple_senders_helper1, sender);
    JoinHandle* handle2 = thread_spawn(test_unbounded_multiple_senders_helper2, sender);
    JoinHandle* handle3 = thread_spawn(test_unbounded_multiple_senders_helper3, sender);

    void* recv1 = unbounded_recv(receiver);
    void* recv2 = unbounded_recv(receiver);
    void* recv3 = unbounded_recv(receiver);

    TEST_ASSERT(recv1 != NULL);
    TEST_ASSERT(recv2 != NULL);
    TEST_ASSERT(recv3 != NULL);
    TEST_ASSERT_INT_EQ(*((int*)(recv1)), 6);
    TEST_ASSERT_INT_EQ(*((int*)(recv2)), 7);
    TEST_ASSERT_INT_EQ(*((int*)(recv3)), 5);

    thread_join(handle1);
    thread_join(handle2);
    thread_join(handle3);

    free(recv1);
    free(recv2);
    free(recv3);

    free_unbounded_sender(sender);
    free_unbounded_receiver(receiver);
}

int main(void)
{
    // Begin
    printf("Beginning tests\n");

    // Run tests
    printf("\nTesting rendezvous channel...\n");
    test_rendezvous_channel();
    printf("\nTesting rendezvous sender and receiver...\n");
    test_rendezvous_sender_receiver();
    printf("\nTesting rendezvous channel sender closing detection...\n");
    test_rendezvous_sender_closed();
    printf("\nTesting rendezvous channel receiver closing detection...\n");
    test_rendezvous_receiver_closed();
    printf("\nTesting rendezvous channel with multiple senders...\n");
    test_rendezvous_multiple_senders();
    printf("\nTesting bounded channel...\n");
    test_bounded_channel();
    printf("\nTesting bounded sender and receiver...\n");
    test_bounded_sender_receiver();
    printf("\nTesting bounded channel sender closing detection...\n");
    test_bounded_sender_closed();
    printf("\nTesting bounded channel receiver closing detection...\n");
    test_bounded_receiver_closed();
    printf("\nTesting bounded sending and receiving from different threads...\n");
    test_bounded_threaded();
    printf("\nTesting bounded channel with multiple senders...\n");
    test_bounded_multiple_senders();
    printf("\nTesting unbounded channel...\n");
    test_unbounded_channel();
    printf("\nTesting unbounded sender and receiver...\n");
    test_unbounded_sender_receiver();
    printf("\nTesting unbounded channel sender closing detection...\n");
    test_unbounded_sender_closed();
    printf("\nTesting unbounded channel receiver closing detection...\n");
    test_unbounded_receiver_closed();
    printf("\nTesting unbounded sending and receiving from different threads...\n");
    test_unbounded_threaded();
    printf("\nTesting unbounded channel with multiple senders...\n");
    test_unbounded_multiple_senders();

    // Done
    printf("\nCompleted tests\n");
}
