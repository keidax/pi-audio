#ifndef _PI_AUDIO_MASTER_H
#define _PI_AUDIO_MASTER_H
#include <netdb.h>

#define BYTES_TO_SEND    4096
#define FRAMES_TO_ENCODE 1024

typedef struct client {
    char * ip_addr;
} client;

#define MAX_CLIENTS 10

int num_clients;
client clients[MAX_CLIENTS];

void start_framesync_thread();

#endif // _PI_AUDIO_MASTER_H
