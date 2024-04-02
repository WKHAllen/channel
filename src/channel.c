#include "channel.h"
#include "mutex.h"
#include "util.h"
#include <stdlib.h>

RendezvousChannel* rendezvous_channel(void)
{
    Mutex* mutex = new_mutex();
    atomic_flag send_blocked = ATOMIC_FLAG_INIT;

    RendezvousChannelBuffer* buffer = NEW(RendezvousChannelBuffer);
    buffer->message = NULL;
    buffer->sender_alive = true;
    buffer->receiver_alive = true;
    buffer->mutex = mutex;
    buffer->send_blocked = send_blocked;

    RendezvousSender* sender = NEW(RendezvousSender);
    sender->buffer = buffer;

    RendezvousReceiver* receiver = NEW(RendezvousReceiver);
    receiver->buffer = buffer;

    RendezvousChannel* channel = NEW(RendezvousChannel);
    channel->sender = sender;
    channel->receiver = receiver;

    return channel;
}

int rendezvous_send(RendezvousSender* sender, void* message)
{
    if (!sender->buffer->receiver_alive) {
        return CHANNEL_CLOSED;
    }

    while (atomic_flag_test_and_set(&sender->buffer->send_blocked)) {
        channel_wait();
    }

    if (mutex_lock(sender->buffer->mutex) != CHANNEL_MUTEX_SUCCESS) {
        atomic_flag_clear(&sender->buffer->send_blocked);
        return CHANNEL_MUTEX_ERROR;
    }

    if (!sender->buffer->receiver_alive) {
        if (mutex_release(sender->buffer->mutex) != CHANNEL_MUTEX_SUCCESS) {
            return CHANNEL_MUTEX_ERROR;
        }

        return CHANNEL_CLOSED;
    }

    RendezvousMessage* new_message = NEW(RendezvousMessage);
    new_message->message = message;
    sender->buffer->message = new_message;

    if (mutex_release(sender->buffer->mutex) != CHANNEL_MUTEX_SUCCESS) {
        return CHANNEL_MUTEX_ERROR;
    }

    while (sender->buffer->message != NULL) {
        channel_wait();
    }

    atomic_flag_clear(&sender->buffer->send_blocked);

    return CHANNEL_SUCCESS;
}

int rendezvous_send_c(RendezvousChannel* channel, void* message)
{
    return rendezvous_send(channel->sender, message);
}

void* rendezvous_recv(RendezvousReceiver* receiver)
{
    if (!receiver->buffer->sender_alive && receiver->buffer->message == NULL) {
        return NULL;
    }

    while (receiver->buffer->message == NULL && receiver->buffer->sender_alive) {
        channel_wait();
    }

    if (mutex_lock(receiver->buffer->mutex) != CHANNEL_MUTEX_SUCCESS) {
        return NULL;
    }

    if (!receiver->buffer->sender_alive && receiver->buffer->message == NULL) {
        mutex_release(receiver->buffer->mutex);
        return NULL;
    }

    RendezvousMessage* new_message = receiver->buffer->message;
    void* message = new_message->message;
    free(new_message);
    receiver->buffer->message = NULL;

    if (mutex_release(receiver->buffer->mutex) != CHANNEL_MUTEX_SUCCESS) {
        return NULL;
    }

    return message;
}

void* rendezvous_recv_c(RendezvousChannel* channel)
{
    return rendezvous_recv(channel->receiver);
}

void free_rendezvous_channel(RendezvousChannel* channel)
{
    if (channel->sender->buffer->message != NULL) {
        free(channel->sender->buffer->message);
    }

    free_mutex(channel->sender->buffer->mutex);
    free(channel->sender->buffer);
    free(channel->sender);
    free(channel->receiver);
    free(channel);
}

void free_rendezvous_channel_wrapper(RendezvousChannel* channel)
{
    free(channel);
}

void free_rendezvous_sender(RendezvousSender* sender)
{
    sender->buffer->sender_alive = false;

    if (!sender->buffer->receiver_alive) {
        if (sender->buffer->message != NULL) {
            free(sender->buffer->message);
        }

        free_mutex(sender->buffer->mutex);
        free(sender->buffer);
    }

    free(sender);
}

void free_rendezvous_receiver(RendezvousReceiver* receiver)
{
    receiver->buffer->receiver_alive = false;

    if (!receiver->buffer->sender_alive) {
        if (receiver->buffer->message != NULL) {
            free(receiver->buffer->message);
        }

        free_mutex(receiver->buffer->mutex);
        free(receiver->buffer);
    }

    free(receiver);
}

BoundedChannel* bounded_channel(size_t capacity)
{
    if (capacity == 0) {
        return NULL;
    }

    Mutex* mutex = new_mutex();
    atomic_flag send_blocked = ATOMIC_FLAG_INIT;

    BoundedMessage** messages = NEW_N(BoundedMessage*, capacity);

    for (size_t i = 0; i < capacity; i++) {
        messages[i] = NEW(BoundedMessage);
        messages[i]->message = NULL;
    }

    BoundedChannelBuffer* buffer = NEW(BoundedChannelBuffer);
    buffer->capacity = capacity;
    buffer->size = 0;
    buffer->head_offset = 0;
    buffer->messages = messages;
    buffer->sender_alive = true;
    buffer->receiver_alive = true;
    buffer->mutex = mutex;
    buffer->send_blocked = send_blocked;

    BoundedSender* sender = NEW(BoundedSender);
    sender->buffer = buffer;

    BoundedReceiver* receiver = NEW(BoundedReceiver);
    receiver->buffer = buffer;

    BoundedChannel* channel = NEW(BoundedChannel);
    channel->sender = sender;
    channel->receiver = receiver;

    return channel;
}

int bounded_send(BoundedSender* sender, void* message)
{
    if (!sender->buffer->receiver_alive) {
        return CHANNEL_CLOSED;
    }

    while (atomic_flag_test_and_set(&sender->buffer->send_blocked)) {
        channel_wait();
    }

    if (mutex_lock(sender->buffer->mutex) != CHANNEL_MUTEX_SUCCESS) {
        atomic_flag_clear(&sender->buffer->send_blocked);
        return CHANNEL_MUTEX_ERROR;
    }

    if (!sender->buffer->receiver_alive) {
        if (mutex_release(sender->buffer->mutex) != CHANNEL_MUTEX_SUCCESS) {
            return CHANNEL_MUTEX_ERROR;
        }

        return CHANNEL_CLOSED;
    }

    sender->buffer->messages[(sender->buffer->head_offset + sender->buffer->size) % sender->buffer->capacity]->message = message;
    sender->buffer->size++;

    if (sender->buffer->size < sender->buffer->capacity) {
        atomic_flag_clear(&sender->buffer->send_blocked);
    }

    if (mutex_release(sender->buffer->mutex) != CHANNEL_MUTEX_SUCCESS) {
        return CHANNEL_MUTEX_ERROR;
    }

    return CHANNEL_SUCCESS;
}

int bounded_send_c(BoundedChannel* channel, void* message)
{
    return bounded_send(channel->sender, message);
}

void* bounded_recv(BoundedReceiver* receiver)
{
    if (!receiver->buffer->sender_alive && receiver->buffer->size == 0) {
        return NULL;
    }

    while (receiver->buffer->size == 0 && receiver->buffer->sender_alive) {
        channel_wait();
    }

    if (mutex_lock(receiver->buffer->mutex) != CHANNEL_MUTEX_SUCCESS) {
        return NULL;
    }

    if (!receiver->buffer->sender_alive && receiver->buffer->size == 0) {
        mutex_release(receiver->buffer->mutex);
        return NULL;
    }

    void* message = receiver->buffer->messages[receiver->buffer->head_offset]->message;
    receiver->buffer->head_offset = (receiver->buffer->head_offset + 1) % receiver->buffer->capacity;
    receiver->buffer->size--;
    atomic_flag_clear(&receiver->buffer->send_blocked);

    if (mutex_release(receiver->buffer->mutex) != CHANNEL_MUTEX_SUCCESS) {
        return NULL;
    }

    return message;
}

void* bounded_recv_c(BoundedChannel* channel)
{
    return bounded_recv(channel->receiver);
}

void free_bounded_channel(BoundedChannel* channel)
{
    for (size_t i = 0; i < channel->sender->buffer->capacity; i++) {
        free(channel->sender->buffer->messages[i]);
    }

    free(channel->sender->buffer->messages);
    free_mutex(channel->sender->buffer->mutex);
    free(channel->sender->buffer);
    free(channel->sender);
    free(channel->receiver);
    free(channel);
}

void free_bounded_channel_wrapper(BoundedChannel* channel)
{
    free(channel);
}

void free_bounded_sender(BoundedSender* sender)
{
    sender->buffer->sender_alive = false;

    if (!sender->buffer->receiver_alive) {
        free(sender->buffer->messages);
        free_mutex(sender->buffer->mutex);
        free(sender->buffer);
    }

    free(sender);
}

void free_bounded_receiver(BoundedReceiver* receiver)
{
    receiver->buffer->receiver_alive = false;

    if (!receiver->buffer->sender_alive) {
        free(receiver->buffer->messages);
        free_mutex(receiver->buffer->mutex);
        free(receiver->buffer);
    }

    free(receiver);
}

UnboundedChannel* unbounded_channel(void)
{
    Mutex* mutex = new_mutex();

    UnboundedChannelBuffer* buffer = NEW(UnboundedChannelBuffer);
    buffer->size = 0;
    buffer->first_message = NULL;
    buffer->last_message = NULL;
    buffer->sender_alive = true;
    buffer->receiver_alive = true;
    buffer->mutex = mutex;

    UnboundedSender* sender = NEW(UnboundedSender);
    sender->buffer = buffer;

    UnboundedReceiver* receiver = NEW(UnboundedReceiver);
    receiver->buffer = buffer;

    UnboundedChannel* channel = NEW(UnboundedChannel);
    channel->sender = sender;
    channel->receiver = receiver;

    return channel;
}

int unbounded_send(UnboundedSender* sender, void* message)
{
    if (!sender->buffer->receiver_alive) {
        return CHANNEL_CLOSED;
    }

    if (mutex_lock(sender->buffer->mutex) != CHANNEL_MUTEX_SUCCESS) {
        return CHANNEL_MUTEX_ERROR;
    }

    if (!sender->buffer->receiver_alive) {
        if (mutex_release(sender->buffer->mutex) != CHANNEL_MUTEX_SUCCESS) {
            return CHANNEL_MUTEX_ERROR;
        }

        return CHANNEL_CLOSED;
    }

    UnboundedMessage* this_message = NEW(UnboundedMessage);
    this_message->message = message;
    this_message->next = NULL;

    if (sender->buffer->last_message != NULL) {
        sender->buffer->last_message->next = this_message;
    }
    else {
        sender->buffer->first_message = this_message;
    }

    sender->buffer->last_message = this_message;
    sender->buffer->size++;

    if (mutex_release(sender->buffer->mutex) != CHANNEL_MUTEX_SUCCESS) {
        return CHANNEL_MUTEX_ERROR;
    }

    return CHANNEL_SUCCESS;
}

int unbounded_send_c(UnboundedChannel* channel, void* message)
{
    return unbounded_send(channel->sender, message);
}

void* unbounded_recv(UnboundedReceiver* receiver)
{
    if (!receiver->buffer->sender_alive && receiver->buffer->size == 0) {
        return NULL;
    }

    while (receiver->buffer->size == 0 && receiver->buffer->sender_alive) {
        channel_wait();
    }

    if (mutex_lock(receiver->buffer->mutex) != CHANNEL_MUTEX_SUCCESS) {
        return NULL;
    }

    if (!receiver->buffer->sender_alive && receiver->buffer->size == 0) {
        mutex_release(receiver->buffer->mutex);
        return NULL;
    }

    UnboundedMessage* this_message = receiver->buffer->first_message;
    receiver->buffer->first_message = this_message->next;
    void* message = this_message->message;
    free(this_message);

    receiver->buffer->size--;

    if (receiver->buffer->first_message == NULL) {
        receiver->buffer->last_message = NULL;
    }

    if (mutex_release(receiver->buffer->mutex) != CHANNEL_MUTEX_SUCCESS) {
        return NULL;
    }

    return message;
}

void* unbounded_recv_c(UnboundedChannel* channel)
{
    return unbounded_recv(channel->receiver);
}

void free_unbounded_channel(UnboundedChannel* channel)
{
    while (channel->sender->buffer->first_message != NULL) {
        UnboundedMessage* message = channel->sender->buffer->first_message;
        channel->sender->buffer->first_message = message->next;
        free(message);
    }

    free_mutex(channel->sender->buffer->mutex);
    free(channel->sender->buffer);
    free(channel->sender);
    free(channel->receiver);
    free(channel);
}

void free_unbounded_channel_wrapper(UnboundedChannel* channel)
{
    free(channel);
}

void free_unbounded_sender(UnboundedSender* sender)
{
    sender->buffer->sender_alive = false;

    if (!sender->buffer->receiver_alive) {
        while (sender->buffer->first_message != NULL) {
            UnboundedMessage* message = sender->buffer->first_message;
            sender->buffer->first_message = message->next;
            free(message);
        }

        free_mutex(sender->buffer->mutex);
        free(sender->buffer);
    }

    free(sender);
}

void free_unbounded_receiver(UnboundedReceiver* receiver)
{
    receiver->buffer->receiver_alive = false;

    if (!receiver->buffer->sender_alive) {
        while (receiver->buffer->first_message != NULL) {
            UnboundedMessage* message = receiver->buffer->first_message;
            receiver->buffer->first_message = message->next;
            free(message);
        }

        free_mutex(receiver->buffer->mutex);
        free(receiver->buffer);
    }

    free(receiver);
}
