#include <portaudio.h>
#include <sndfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#include "pi_audio.h"
#include "master.h"

int num_out_channels, sample_rate;
SNDFILE * playback_file, * streaming_file;

short buf[FRAMES_TO_SEND * MAX_CHANNELS];

static pthread_t streaming_thread;
int sockfd;
uint32_t total_bytes_sent;

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
    (void) userData;

    sf_count_t frames_read;
    /* Read audio data from file. libsndfile can convert between formats
     * on-the-fly, so we should always use floats.
     */
    frames_read = sf_readf_float(playback_file,
            (float *) outputBuffer,
            (sf_count_t) framesPerBuffer);

    master_frames_played += frames_read;

    //printf("MFP: %li\n", master_frames_played);

    /* If we've read all the frames, then we can finish. */
    if(frames_read < 0 || (unsigned long)frames_read < framesPerBuffer) {
        return paComplete;
    }
    return paContinue;
}

void master_setup() {
    num_clients = 1;
    clients[0].ip_addr = "192.168.1.148";
}

void * streaming_thread_init(void * userdata) {
    (void) userdata;

    int opt_on = 0;

    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &opt_on, sizeof(int));

    /* Set up streaming to client. */
    sf_count_t frames_read;
    int bytes_per_frame = MAX_CHANNELS * sizeof(short);
    int bytes_to_send, bytes_sent;
    int i = 0;

    /* Send audio over network to client. */
    while(1) {
        frames_read = sf_readf_short(streaming_file, buf, FRAMES_TO_SEND);
        if(frames_read == 0)
            break;
        bytes_to_send = frames_read * bytes_per_frame;
        bytes_sent = send(sockfd, (void *) buf, bytes_to_send, 0);
        void * start = buf;
        while(bytes_sent != 0) {
            if(bytes_sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                //printf("%%");
                int q;
                for(q = 0; q < 100; q++) {
                    // Burn cpu time
                }
            } else {
                //printf("Sent %i/%i bytes to client\n", bytes_sent, bytes_to_send);
                start += bytes_sent;
                bytes_to_send -= bytes_sent;
                total_bytes_sent += bytes_sent;
            }
            bytes_sent = send(sockfd, (void *) start, bytes_to_send, 0);
        }
        i++;

        if(i%20 == 0) {
            printf("Total frames sent to client: %u\n", total_bytes_sent / bytes_per_frame);
        }
    }
    sf_close(streaming_file);
    return NULL;
}

int main() {

    common_setup();
    master_setup();

    char * file_path = "samples/deadmau5.ogg";
    const PaVersionInfo * version_info = Pa_GetVersionInfo();
    printf("Using %s\n", version_info->versionText);
    SF_INFO playback_info, streaming_info;

    /* Open a sound file. */
    playback_file = sf_open(file_path, SFM_READ, &playback_info);
    if(playback_file == NULL) {
        printf("Error opening file:\n");
        printf("%s\n", sf_strerror(playback_file));
        sf_close(playback_file);
        return 1;
    } else {
        printf("Playback file opened successfully!\n");
    }

    if(playback_info.channels <= MAX_CHANNELS) {
        num_out_channels = playback_info.channels;
    } else {
        printf("File has %i channels but we can only handle up to %i channels!"
                "\n", playback_info.channels, MAX_CHANNELS);
        sf_close(playback_file);
        return 1;
    }
    sample_rate = playback_info.samplerate;

    streaming_file = sf_open(file_path, SFM_READ, &streaming_info);
    if(streaming_file == NULL) {
        printf("Error opening file:\n");
        printf("%s\n", sf_strerror(streaming_file));
        sf_close(streaming_file);
        sf_close(playback_file);
        return 1;
    } else {
        printf("Streaming file opened successfully!\n");
    }


    // Get address info for our Pi
    struct addrinfo * pi_ai;

    getaddrinfo(clients[0].ip_addr, stream_port_str, &hints, &pi_ai);

    sockfd = socket(pi_ai->ai_family, pi_ai->ai_socktype, pi_ai->ai_protocol);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    if(connect(sockfd, pi_ai->ai_addr, pi_ai->ai_addrlen)) {
        printf("Error connecting to socket: %s\n", strerror(errno));
    }

    printf("Connected successfully!\n");

    start_framesync_thread();
    pthread_create(&streaming_thread, NULL, streaming_thread_init, NULL);

    /* Initialize PortAudio */
    PaError err;
    err = Pa_Initialize();
    if(err != paNoError) goto error;

    PaStream * stream;
    /* Open an audio stream. */
    err = Pa_OpenDefaultStream( &stream,
                                0,
                                num_out_channels,
                                paFloat32,
                                sample_rate,
                                paFramesPerBufferUnspecified,
                                paTestCallback,
                                NULL);
    if(err != paNoError) goto error;

    /* Start the audio stream for playback on master. */
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
        sf_close(playback_file);
        return 1;
    }

    sf_close(playback_file);
    return 0;
}
