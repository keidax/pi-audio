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
    ourData * data = (ourData *) userData;

    int bytes_read, bytes_to_read;
    int frames_read;
    static int i = 0;
    /* Read audio data from file. libsndfile can convert between formats
     * on-the-fly, so we should always use floats.
     */
    frames_read = sf_readf_short(decoded_file, outputBuffer, framesPerBuffer);
    //printf("Received %i/%i bytes from pipe\n", bytes_read, bytes_to_read);
    /* If we've read all the frames, then we can finish. */
    if(frames_read < framesPerBuffer) {
        return paComplete;
    }

    client_frames_played += framesPerBuffer;
    i++;

    if(i%50 == 0) {
        //printf("MFP: %" PRIu32 "\n", master_frames_played);
        //printf("CFR: %" PRIu32 "\n", client_bytes_recvd / our_data.frame_length);
        //printf("CFP: %" PRIu32 "\n", client_frames_played);
        printf("Client/Master: %i%%\t", client_frames_played * 100 / master_frames_played);
        printf("Client buffer: %.3f secs\n", ((client_bytes_recvd / our_data.frame_length) - client_frames_played) / 44100.0);
    }

    return paContinue;
}
