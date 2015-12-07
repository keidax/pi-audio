//#include <sys/socket.h>
//#include <sys/types.h>
#include <pthread.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pi_audio.h"

static pthread_t framesync_thread;
static struct addrinfo * res;
static int sync_sock_fd;

static struct sockaddr_storage master_addr;
static socklen_t addr_size;

/* How many frames of audio the master has played. */
unsigned long master_frames_played = 0;

void * framesync_thread_init(void * userdata) {
    (void) userdata;
    // Connect to sync sockets here
    int i;
    getaddrinfo(NULL, sync_port_str, &hints, &res);
    sync_sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if(listen(sync_sock_fd, BACKLOG)) {
        printf("Error listening on port %s -- %s\n", sync_port_str,
                strerror(errno));
    }

    addr_size = sizeof(master_addr);
    int new_fd = accept(sync_sock_fd, &master_addr, &addr_size);

    while(1) {

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
