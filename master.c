#include <portaudio.h>
#include <sndfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

const int MAX_CHANNELS = 2;

static SF_INFO info;
static int num_out_channels, sample_rate;
static SNDFILE * sample_file;

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
    frames_read = sf_readf_float(sample_file,
            (float *) outputBuffer,
            (sf_count_t) framesPerBuffer);

    /* If we've read all the frames, then we can finish. */
    if(frames_read < 0 || (unsigned long)frames_read < framesPerBuffer) {
        return paComplete;
    }
    return paContinue;
}

int main() {
    const PaVersionInfo * version_info = Pa_GetVersionInfo();
    printf("Using %s\n", version_info->versionText);

    /* Open a sound file. */
    sample_file = sf_open("samples/deadmau5.wav", SFM_READ, &info);
    if(sample_file == NULL) {
        printf("Error opening sample file:\n");
        printf("%s\n", sf_strerror(sample_file));
        sf_close(sample_file);
        return 1;
    } else {
        printf("Sample file opened successfully!\n");
    }

    if(info.channels <= MAX_CHANNELS) {
        num_out_channels = info.channels;
    } else {
        printf("File has %i channels but we can only handle up to %i channels!"
                "\n", info.channels, MAX_CHANNELS);
        sf_close(sample_file);
        return 1;
    }
    sample_rate = info.samplerate;

    // Get address info for our Pi
    struct addrinfo hints;
    struct addrinfo * pi_ai;
    bzero((char *) &hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = 0;

    getaddrinfo("192.168.1.147", "10144", &hints, &pi_ai);

    int sockfd = socket(pi_ai->ai_family, pi_ai->ai_socktype, pi_ai->ai_protocol);
    if(connect(sockfd, pi_ai->ai_addr, pi_ai->ai_addrlen)) {
        printf("Error connecting to socket: %s\n", strerror(errno));
    }

    printf("Connected successfully!\n");

    float buf[1024];
    sf_count_t frames_read;
    int bytes_per_frame = MAX_CHANNELS * sizeof(float);
    int bytes_to_send, bytes_sent;

    while(1) {
        frames_read = sf_readf_float(sample_file, buf, 256);
        bytes_to_send = frames_read * bytes_per_frame;
        bytes_sent = send(sockfd, (void *) buf, bytes_to_send, 0);
        void * start = buf;
        while(bytes_sent != bytes_to_send) {
            start += bytes_sent;
            bytes_to_send -= bytes_sent;
            bytes_sent = send(sockfd, (void *) start, bytes_to_send, 0);
        }
    }




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
        sf_close(sample_file);
        return 1;
    }

    sf_close(sample_file);
    return 0;
}
