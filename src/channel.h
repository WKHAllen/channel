#ifndef CHANNEL_H
#define CHANNEL_H

#include "mutex.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>

#define CHANNEL_SUCCESS     0
#define CHANNEL_CLOSED      1
#define CHANNEL_MUTEX_ERROR 2

// A message in a rendezvous channel.
typedef struct RendezvousMessage_ {
    void* message;
} RendezvousMessage;

// The internal message buffer in a rendezvous channel.
typedef struct RendezvousChannelBuffer_ {
    RendezvousMessage* message;
    bool sender_alive;
    bool receiver_alive;
    Mutex* mutex;
    atomic_flag send_blocked;
} RendezvousChannelBuffer;

// The sending half of a rendezvous channel.
typedef struct RendezvousSender_ {
    RendezvousChannelBuffer* buffer;
} RendezvousSender;

// The receiving half of a rendezvous channel.
typedef struct RendezvousReceiver_ {
    RendezvousChannelBuffer* buffer;
} RendezvousReceiver;

// Both halves of a rendezvous channel.
typedef struct RendezvousChannel_ {
    RendezvousSender* sender;
    RendezvousReceiver* receiver;
} RendezvousChannel;

// Creates a rendezvous channel. A rendezvous channel is a channel without an
// internal buffer. Each time a message is sent, the send operation will block
// until a corresponding receive operation occurs. If you need a channel with
// a buffer, use a bounded channel.
//
// The channel is intended to be separated into its sending and receiving
// halves. To separate the channel, use this function, extract the `sender` and
// `receiver` fields, and call `free_rendezvous_channel_wrapper` to clean up the
// memory used by the channel wrapper. Once finished using the sender and
// receiver, call `free_rendezvous_sender` and `free_rendezvous_receiver` to
// clean up their memory as well. If for some reason you do not need to separate
// the channel into its sending and receiving halves, `free_rendezvous_channel`
// can be called when finished with the channel.
//
// The channel created is multi-producer, single-consumer. This means that for
// each channel multiple senders may safely send messages from different threads
// simultaneously, but there can only ever be one receiver. Attempting to
// receive from a single channel via two or more threads at the same time will
// introduce a race condition.
RendezvousChannel* rendezvous_channel(void);

// Sends a message through the channel via the sender. The message must be
// kept alive at at least long enough to be received. The returned value is an
// error code.
int rendezvous_send(RendezvousSender* sender, void* message);

// Sends a message through the channel via the channel wrapper. The message
// must be kept alive at at least long enough to be received. The returned
// value is an error code.
int rendezvous_send_c(RendezvousChannel* channel, void* message);

// Receives a message from the channel via the receiver. If `NULL` is
// returned, the sender was destroyed.
void* rendezvous_recv(RendezvousReceiver* receiver);

// Receives a message from the channel via the channel wrapper. If `NULL` is
// returned, the sender was destroyed.
void* rendezvous_recv_c(RendezvousChannel* channel);

// Frees all memory within the channel, including the sender, receiver, and
// internal buffer.
void free_rendezvous_channel(RendezvousChannel* channel);

// Frees only the memory used by the channel wrapper. The sender, receiver,
// and internal buffer will remain allocated.
void free_rendezvous_channel_wrapper(RendezvousChannel* channel);

// Frees the memory used by the sending half of the channel. If the receiver
// is still alive, the internal buffer will remain allocated.
void free_rendezvous_sender(RendezvousSender* sender);

// Frees the memory used by the receiving half of the channel. If the sender
// is still alive, the internal buffer will remain allocated.
void free_rendezvous_receiver(RendezvousReceiver* receiver);

// A message in a bounded channel. This only contains a pointer to the message
// itself.
typedef struct BoundedMessage_ {
    void* message;
} BoundedMessage;

// The internal message buffer of a bounded channel.
typedef struct BoundedChannelBuffer_ {
    size_t capacity;
    size_t size;
    size_t head_offset;
    BoundedMessage** messages;
    bool sender_alive;
    bool receiver_alive;
    Mutex* mutex;
    atomic_flag send_blocked;
} BoundedChannelBuffer;

// The sending half of a bounded channel.
typedef struct BoundedSender_ {
    BoundedChannelBuffer* buffer;
} BoundedSender;

// The receiving half of a bounded channel.
typedef struct BoundedReceiver_ {
    BoundedChannelBuffer* buffer;
} BoundedReceiver;

// Both halves of a bounded channel.
typedef struct BoundedChannel_ {
    BoundedSender* sender;
    BoundedReceiver* receiver;
} BoundedChannel;

// Creates a bounded channel with the given internal buffer capacity. Once the 
// buffer is full, send operations will block until receive operations occur and
// clear up space within the buffer. The capacity cannot be zero, or NULL will
// be returned. If you need a channel with a zero-sized buffer, use a rendezvous
// channel. If you need a channel with no upper bound on memory usage, use an
// unbounded channel.
//
// The channel is intended to be separated into its sending and receiving
// halves. To separate the channel, use this function, extract the `sender` and
// `receiver` fields, and call `free_bounded_channel_wrapper` to clean up the
// memory used by the channel wrapper. Once finished using the sender and
// receiver, call `free_bounded_sender` and `free_bounded_receiver` to clean up
// their memory as well. If for some reason you do not need to separate the
// channel into its sending and receiving halves, `free_bounded_channel` can be
// called when finished with the channel.
//
// The channel created is multi-producer, single-consumer. This means that for
// each channel multiple senders may safely send messages from different threads
// simultaneously, but there can only ever be one receiver. Attempting to
// receive from a single channel via two or more threads at the same time will
// introduce a race condition.
BoundedChannel* bounded_channel(size_t capacity);

// Sends a message through the channel via the sender. The message must be
// kept alive at at least long enough to be received. The returned value is an
// error code.
int bounded_send(BoundedSender* sender, void* message);

// Sends a message through the channel via the channel wrapper. The message
// must be kept alive at at least long enough to be received. The returned
// value is an error code.
int bounded_send_c(BoundedChannel* channel, void* message);

// Receives a message from the channel via the receiver. If `NULL` is
// returned, the sender was destroyed.
void* bounded_recv(BoundedReceiver* receiver);

// Receives a message from the channel via the channel wrapper. If `NULL` is
// returned, the sender was destroyed.
void* bounded_recv_c(BoundedChannel* channel);

// Frees all memory within the channel, including the sender, receiver, and
// internal buffer.
void free_bounded_channel(BoundedChannel* channel);

// Frees only the memory used by the channel wrapper. The sender, receiver,
// and internal buffer will remain allocated.
void free_bounded_channel_wrapper(BoundedChannel* channel);

// Frees the memory used by the sending half of the channel. If the receiver
// is still alive, the internal buffer will remain allocated.
void free_bounded_sender(BoundedSender* sender);

// Frees the memory used by the receiving half of the channel. If the sender
// is still alive, the internal buffer will remain allocated.
void free_bounded_receiver(BoundedReceiver* receiver);

// A message in a bounded channel. This contains a pointer to the message and
// a pointer to the next message.
typedef struct UnboundedMessage_ {
    void* message;
    struct UnboundedMessage_* next;
} UnboundedMessage;

// The internal message buffer of an unbounded channel.
typedef struct UnboundedChannelBuffer_ {
    size_t size;
    UnboundedMessage* first_message;
    UnboundedMessage* last_message;
    bool sender_alive;
    bool receiver_alive;
    Mutex* mutex;
} UnboundedChannelBuffer;

// The sending half of an unbounded channel.
typedef struct UnboundedSender_ {
    UnboundedChannelBuffer* buffer;
} UnboundedSender;

// The receiving half of an unbounded channel.
typedef struct UnboundedReceiver_ {
    UnboundedChannelBuffer* buffer;
} UnboundedReceiver;

// Both halves of an unbounded channel.
typedef struct UnboundedChannel_ {
    UnboundedSender* sender;
    UnboundedReceiver* receiver;
} UnboundedChannel;

// Creates an unbounded channel. For unbounded channels, the only bound on
// memory usage is the amount of memory the operating system provides. If you
// need a channel with an upper bound on memory usage, use a bounded channel.
//
// The channel is intended to be separated into its sending and receiving
// halves. To separate the channel, use this function, extract the `sender` and
// `receiver` fields, and call `free_unbounded_channel_wrapper` to clean up the
// memory used by the channel wrapper. Once finished using the sender and
// receiver, call `free_unbounded_sender` and `free_unbounded_receiver` to clean
// up their memory as well. If for some reason you do not need to separate the
// channel into its sending and receiving halves, `free_unbounded_channel` can
// be called when finished with the channel.
//
// The channel created is multi-producer, single-consumer. This means that for
// each channel multiple senders may safely send messages from different threads
// simultaneously, but there can only ever be one receiver. Attempting to
// receive from a single channel via two or more threads at the same time will
// introduce a race condition.
UnboundedChannel* unbounded_channel(void);

// Sends a message through the channel via the sender. The message must be
// kept alive at at least long enough to be received. The returned value is an
// error code.
int unbounded_send(UnboundedSender* sender, void* message);

// Sends a message through the channel via the channel wrapper. The message
// must be kept alive at at least long enough to be received. The returned
// value is an error code.
int unbounded_send_c(UnboundedChannel* channel, void* message);

// Receives a message from the channel via the receiver. If `NULL` is
// returned, the sender was destroyed.
void* unbounded_recv(UnboundedReceiver* receiver);

// Receives a message from the channel via the channel wrapper. If `NULL` is
// returned, the sender was destroyed.
void* unbounded_recv_c(UnboundedChannel* channel);

// Frees all memory within the channel, including the sender, receiver, and
// internal buffer.
void free_unbounded_channel(UnboundedChannel* channel);

// Frees only the memory used by the channel wrapper. The sender, receiver,
// and internal buffer will remain allocated.
void free_unbounded_channel_wrapper(UnboundedChannel* channel);

// Frees the memory used by the sending half of the channel. If the receiver
// is still alive, the internal buffer will remain allocated.
void free_unbounded_sender(UnboundedSender* sender);

// Frees the memory used by the receiving half of the channel. If the sender
// is still alive, the internal buffer will remain allocated.
void free_unbounded_receiver(UnboundedReceiver* receiver);

#endif // CHANNEL_H
