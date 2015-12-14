#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pi_audio.h"
#include "client.h"

static pthread_t stream_thread;

void * stream_thread_init(void * userdata) {
    (void) userdata;
    int buf_len = 1024 * our_data.frame_length;
    char buf[buf_len];

    int bytes_to_read, bytes_read, bytes_read_total, bytes_to_write, bytes_written;

    client_bytes_recvd = 0;

    while(1) {
        bytes_read_total = 0;
        // Receive from master over network
        bytes_to_read = buf_len;
        bytes_read = recv(our_data.sock_fd, buf, bytes_to_read, 0);

        void * start = buf;
        while(bytes_read != 0) {
            if(bytes_read < 0) {
                if((errno == EAGAIN || errno == EWOULDBLOCK)) {
                    int q;
                    for(q = 0; q< 100; q++) {
                        //burn cpu time
                    }
                } else {
                    printf("Error receiving on socket: %s\n", strerror(errno));
                }
            } else {
                //printf("Received %i/%i bytes from socket\n", bytes_read, bytes_to_read);
                client_bytes_recvd += bytes_read;
                bytes_read_total += bytes_read;
                bytes_to_read -= bytes_read;
                start += bytes_read;
            }
            bytes_read = recv(our_data.sock_fd, start, bytes_to_read, 0);
        }

        // Write to pipe
        bytes_to_write = bytes_read_total;
        bytes_written = write(audio_pipe_fd[1], buf, bytes_to_write);
        while(bytes_written != bytes_to_write) {
            bytes_to_write -= bytes_written;
            bytes_written = write(audio_pipe_fd[1], buf, bytes_to_write);
        }
    }

    return NULL;
}

void start_streaming_thread() {
    int pthread_err;
    pthread_err = pthread_create(&stream_thread, NULL, stream_thread_init, NULL);
    if(pthread_err) {
        printf("Error code returned from pthread_create(): %d\n", pthread_err);
        exit(1);
    }

    decoded_file = sf_open_fd(audio_pipe_fd[0], SFM_READ, &decoded_info, 0);
}
