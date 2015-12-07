//#include <sys/socket.h>
//#include <sys/types.h>
#include <pthread.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "master.h"
#include "pi_audio.h"

static pthread_t framesync_thread;
static int sync_sock_fds[MAX_CLIENTS];
static struct addrinfo * client_ais[MAX_CLIENTS];

static const struct timespec wait_time = {
    .tv_nsec = 1000 * 1000 * 10, // 10 milliseconds
    .tv_sec = 0
};

static struct timespec rem_time;

/* How many frames of audio the master has played. */
unsigned long master_frames_played = 0;

void * framesync_thread_init(void * userdata) {
    (void) userdata;
    // Connect to sync sockets here
    int i, status;
    for(i = 0; i < num_clients; i++) {
        if((status = getaddrinfo(clients[i].ip_addr, sync_port_str, &hints, &client_ais[i]))) {
            printf("Error in getaddrinfo(): %s\n", gai_strerror(status));
            exit(1);
        }

        struct addrinfo * cur_ai = client_ais[i];
        if((sync_sock_fds[i] = socket(cur_ai->ai_family, cur_ai->ai_socktype, cur_ai->ai_protocol)) < 0) {
            printf("Error getting socket: %s\n", strerror(errno));
            exit(1);
        }

        if(connect(sync_sock_fds[i], cur_ai->ai_addr, cur_ai->ai_addrlen)) {
            printf("Error connecting to %s:%s -- %s\n", clients[i].ip_addr,
                    sync_port_str, strerror(errno));
            exit(1);
        }
    }

    printf("Framesync thread connected to %i clients\n", num_clients);

    int bytes_sent;
    while(1) {
        for(i = 0; i < num_clients; i++) {
            bytes_sent = send(sync_sock_fds[i], (void *) &master_frames_played, 
                    sizeof(master_frames_played), 0);
            if(bytes_sent < 0) {
                printf("Error sending on port %s -- %s\n", sync_port_str, strerror(errno));
                exit(1);
            }
        }
        //printf("Sent MFP = %i\n", master_frames_played);
        nanosleep(&wait_time, &rem_time);
    }
    return NULL;
}

void start_framesync_thread() {
    int err;
    err = pthread_create(&framesync_thread, NULL, framesync_thread_init, NULL);
    if(err) {
        printf("Error code returned from pthread_create(): %d\n", err);
        exit(1);
    }
}
