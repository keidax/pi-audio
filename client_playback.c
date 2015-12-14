#include <portaudio.h>
#include <stdio.h>
#include <unistd.h>

#include "pi_audio.h"
#include "client.h"

/* This routine will be called by the PortAudio engine when audio is needed.
 * It may be called at interrupt level on some machines so don't do anything
 * that could mess up the system like calling malloc() or free().
 */
int pa_client_callback(const void * inputBuffer, void * outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo * timeInfo,
        PaStreamCallbackFlags statusFlags,
        void * userData) {
    /* Prevent unused variable warnings. */
    (void) inputBuffer;
    (void) timeInfo;
    (void) statusFlags;
    //ourData * data = (ourData *) userData;
    (void) userData;

    sf_count_t frames_read;
    static int i = 0;
    /* Read audio data from file. */
    frames_read = sf_readf_short(decoded_file, outputBuffer, framesPerBuffer);

    /* If we've read all the frames, then we can finish. */
    if(frames_read < framesPerBuffer) {
        return paComplete;
    }

    client_frames_played += framesPerBuffer;
    i++;

    if(i%50 == 0) {
        printf("Client lag: %.3f secs\t", (master_frames_played - client_frames_played) / 44100.0);
        printf("Client buffer: %.3f secs\n", ((client_bytes_recvd / BYTES_PER_FRAME) - client_frames_played) / 44100.0);
    }

    return paContinue;
}
