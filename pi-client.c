#include <portaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

const int MAX_CHANNELS = 2;
const int BACKLOG = 5; // Do we need this?

static int num_out_channels, sample_rate;

typedef struct ourData {
    int sock_fd; // The socket file descriptor to read from
    long frame_length;
} ourData;

static ourData our_data;

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
    /* Read audio data from file. libsndfile can convert between formats
     * on-the-fly, so we should always use floats.
     */
    bytes_to_read = framesPerBuffer * data->frame_length;
    bytes_read = recv(data->sock_fd, outputBuffer, bytes_to_read, 0);
    printf("Received %i/%i bytes from socket\n", bytes_read, bytes_to_read);
    /* If we've read all the frames, then we can finish. */
    while(bytes_read != bytes_to_read) {
        if(bytes_read == 0 && bytes_to_read != 0) {
            return paComplete;
        } else if(bytes_read < bytes_to_read) {
            bytes_to_read -= bytes_read;
            bytes_read = recv(data->sock_fd, outputBuffer, bytes_to_read, 0);
            printf("Received %i/%i bytes from socket\n", bytes_read, bytes_to_read);
        }
    }

    return paContinue;
}

int main() {
    const PaVersionInfo * version_info = Pa_GetVersionInfo();
    printf("Using %s\n", version_info->versionText);

    struct addrinfo hints;
    struct addrinfo * res;
    bzero((char *) &hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, "10144", &hints, &res);

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(bind(sockfd, res->ai_addr, res->ai_addrlen)) {
        printf("Error binding to socket: %s\n", strerror(errno));
        return 1;
    }

    if(listen(sockfd, BACKLOG)) {
        printf("Error listening on socket: %s\n", strerror(errno));
        return 1;
    }


    printf("Listening on port %i\n", ntohs(((struct sockaddr_in *)res->ai_addr)->sin_port));

    struct sockaddr_storage master_addr;
    socklen_t addr_size;
    addr_size = sizeof(master_addr);
    int new_fd = accept(sockfd, (struct sockaddr *) &master_addr, &addr_size);
    
    our_data.sock_fd = new_fd;
    our_data.frame_length = MAX_CHANNELS * sizeof(float);
    printf("Frame length is %i bytes\n", our_data.frame_length);


    /* Initialize PortAudio */
    PaError err;
    err = Pa_Initialize();
    if(err != paNoError) goto error;

    PaStream * stream;
    /* Open an audio stream. */
    err = Pa_OpenDefaultStream( &stream,
                                0,
                                MAX_CHANNELS,
                                paFloat32,
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
