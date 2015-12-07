#ifndef _PI_AUDIO_MASTER_H
#define _PI_AUDIO_MASTER_H
#include <netdb.h>

#define FRAMES_TO_SEND 512


typedef struct client {
    char * ip_addr;
} client;

#define MAX_CLIENTS 10

int num_clients;
client clients[MAX_CLIENTS];

#endif // _PI_AUDIO_MASTER_H
