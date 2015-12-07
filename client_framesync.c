//#include <sys/socket.h>
//#include <sys/types.h>
#include <pthread.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "pi_audio.h"
#include "client.h"

static pthread_t framesync_thread;
static struct addrinfo * res;
static int sync_sock_fd;

static struct sockaddr_storage master_addr;
static socklen_t addr_size;

static const struct timespec wait_time = {
    .tv_nsec = 1000 * 1000 * 10, // 10 milliseconds
    .tv_sec = 0
};

static struct timespec rem_time;

/* How many frames of audio the master has played. */
unsigned long master_frames_played = 0;

void * framesync_thread_init(void * userdata) {
    (void) userdata;
    // Connect to master sync socket
    getaddrinfo(NULL, sync_port_str, &hints, &res);
    sync_sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if(listen(sync_sock_fd, BACKLOG)) {
        printf("Error listening on port %s -- %s\n", sync_port_str,
                strerror(errno));
    }

    addr_size = sizeof(master_addr);
    int new_fd = accept(sync_sock_fd, (struct sockaddr *) &master_addr,
            &addr_size);

    int bytes_read = 0;
    while(1) {
        bytes_read = recv(new_fd, (void *) &master_frames_played,
                sizeof(master_frames_played), 0);
        if(bytes_read == 0){
            break;
        }
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
