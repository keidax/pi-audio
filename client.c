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
#include <fcntl.h>

#include "pi_audio.h"
#include "client.h"

ourData our_data;

int main() {

    common_setup();

    const PaVersionInfo * version_info = Pa_GetVersionInfo();
    printf("Using %s\n", version_info->versionText);

    struct addrinfo * res;

    getaddrinfo(NULL, stream_port_str, &hints, &res);

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
    pipe(audio_pipe_fd);
    fcntl(audio_pipe_fd[0], F_SETPIPE_SZ, 1048576);

    start_streaming_thread();

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
                                pa_client_callback,
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
