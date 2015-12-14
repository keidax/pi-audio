#define _GNU_SOURCE // Needed for fcntl(2) F_SETPIPE_SZ

#include <portaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#include "pi_audio.h"
#include "client.h"

//static int num_out_channels, sample_rate;

typedef struct ourData {
    int sock_fd; // The socket file descriptor to read from
    long frame_length;
} ourData;

static ourData our_data;

static int pipe_fd[2];
static pthread_t buffer_thread;

uint32_t client_bytes_recvd = 0;
uint32_t client_frames_played = 0;

/* This routine will be called by the PortAudio engine when audio is needed.
 * It may be called at interrupt level on some machines so don't do anything
 * that could mess up the system like calling malloc() or free().
 */
static int paTestCallback(const void * inputBuffer, void * outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo * timeInfo,
        PaStreamCallbackFlags statusFlags,
        void * userData) {
    /* Prevent unused variable warnings. */
    (void) inputBuffer;
    (void) timeInfo;
    (void) statusFlags;
    ourData * data = (ourData *) userData;

    int bytes_read, bytes_to_read;
    static int i = 0;
    /* Read audio data from file. libsndfile can convert between formats
     * on-the-fly, so we should always use floats.
     */
    bytes_to_read = framesPerBuffer * data->frame_length;
    bytes_read = read(pipe_fd[0], outputBuffer, bytes_to_read);
    //printf("Received %i/%i bytes from pipe\n", bytes_read, bytes_to_read);
    /* If we've read all the frames, then we can finish. */
    while(bytes_read != bytes_to_read) {
        if(bytes_read == 0 && bytes_to_read != 0) {
            return paComplete;
        } else if(bytes_read < bytes_to_read) {
            bytes_to_read -= bytes_read;
            bytes_read = read(pipe_fd[0], outputBuffer, bytes_to_read);
            //printf("Received %i/%i bytes from pipe\n", bytes_read, bytes_to_read);
        }
    }

    client_frames_played += framesPerBuffer;
    i++;

    if(i%50 == 0) {
        //printf("MFP: %" PRIu32 "\n", master_frames_played);
        //printf("CFR: %" PRIu32 "\n", client_bytes_recvd / our_data.frame_length);
        //printf("CFP: %" PRIu32 "\n", client_frames_played);
        printf("Client/Master: %i%%\n", client_frames_played * 100 / master_frames_played);
    }

    return paContinue;
}

void * buffer_thread_init(void * userdata) {
    (void) userdata;
    int buf_len = 1024 * our_data.frame_length;
    char buf[buf_len];

    int bytes_to_read, bytes_read, bytes_read_total, bytes_to_write, bytes_written;

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
        bytes_written = write(pipe_fd[1], buf, bytes_to_write);
        while(bytes_written != bytes_to_write) {
            bytes_to_write -= bytes_written;
            bytes_written = write(pipe_fd[1], buf, bytes_to_write);
        }
    }

    return NULL;
}

int main() {

    common_setup();

    const PaVersionInfo * version_info = Pa_GetVersionInfo();
    printf("Using %s\n", version_info->versionText);

    struct addrinfo * res;

    getaddrinfo(NULL, "10144", &hints, &res);

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    if(bind(sockfd, res->ai_addr, res->ai_addrlen)) {
        printf("Error binding to socket: %s\n", strerror(errno));
        return 1;
    }

    if(listen(sockfd, BACKLOG)) {
        printf("Error listening on socket: %s\n", strerror(errno));
        return 1;
    }


    printf("Listening on port %i\n", ntohs(((struct sockaddr_in *)res->ai_addr)->sin_port));

    start_framesync_thread();

    struct sockaddr_storage master_addr;
    socklen_t addr_size;
    addr_size = sizeof(master_addr);
    int new_fd;
    while((new_fd = accept(sockfd, (struct sockaddr *) &master_addr, &addr_size)) < 0) {
        if(errno ==EAGAIN || errno == EWOULDBLOCK) {
            sleep(1);
        } else {
            printf("Error accepting connection: %s\n", strerror(errno));
            return 1;
        }
    }

    our_data.sock_fd = new_fd;
    our_data.frame_length = MAX_CHANNELS * sizeof(short);
    printf("Frame length is %li bytes\n", our_data.frame_length);

    // Create a pipe to hold audio data
    pipe(pipe_fd);
    fcntl(pipe_fd[0], F_SETPIPE_SZ, 1048576);

    int pthread_err;
    pthread_err = pthread_create(&buffer_thread, NULL, buffer_thread_init, NULL);
    if(pthread_err) {
        printf("Error code returned from pthread_create(): %d\n", pthread_err);
        exit(1);
    }

    /* Initialize PortAudio */
    PaError err;
    err = Pa_Initialize();
    if(err != paNoError) goto error;

    PaStream * stream;
    /* Open an audio stream. */
    err = Pa_OpenDefaultStream( &stream,
                                0,
                                MAX_CHANNELS,
                                paInt16,
                                44100,
                                paFramesPerBufferUnspecified,
                                paTestCallback,
                                &our_data);
    if(err != paNoError) goto error;

    /* Start the audio stream. */
    err = Pa_StartStream(stream);
    if(err != paNoError) goto error;

    /* Sleep until stream finishes (when the callback outputs paComplete and all
     * buffers finish playing).
     */
    while(Pa_IsStreamActive(stream)) {
        Pa_Sleep(500);
    }

    /* Stop the audio stream. */
    err = Pa_StopStream(stream);
    if(err != paNoError) goto error;

    /* Close the audio stream. */
    err = Pa_CloseStream(stream);
    if(err != paNoError) goto error;

    /* Terminate PortAudio */
    err = Pa_Terminate();
    if(err != paNoError) {
error:
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    return 0;
}
