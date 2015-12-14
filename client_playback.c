#include <portaudio.h>
#include <stdio.h>
#include <unistd.h>

#include "pi_audio.h"
#include "client.h"

// Allowable latency, in frames
static int master_frame_correction = 3000;
static const int skip_frames = 512;
static short skip_buf[512 * MAX_CHANNELS];

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

    // Skip ahead if we're lagging too far behind the master
    if(client_frames_played + framesPerBuffer < master_frames_played + master_frame_correction) {
        int frames_to_skip = (master_frames_played + master_frame_correction) -
            (client_frames_played + framesPerBuffer);
        int frames_skipped;
        client_frames_played += frames_to_skip;
        printf("Skipping %i frames\n", frames_to_skip);
        while(frames_to_skip > skip_frames) {
            frames_skipped = sf_readf_short(decoded_file, skip_buf, skip_frames);
            frames_to_skip -= frames_skipped;
        }
        sf_readf_short(decoded_file, skip_buf, frames_to_skip);
    }

    /* Read audio data from file. */
    frames_read = sf_readf_short(decoded_file, outputBuffer, framesPerBuffer);

    /* If we've read all the frames, then we can finish. */
    if(frames_read < framesPerBuffer) {
        return paComplete;
    }

    client_frames_played += framesPerBuffer;
    i++;

    if(i%10 == 0) {
        printf("MFP: %u\n", master_frames_played);
        //printf("Client lag: %.3f secs\t", (master_frames_played - client_frames_played) / 44100.0);
        //printf("Client buffer: %.3f secs\n", ((client_bytes_recvd / BYTES_PER_FRAME) - client_frames_played) / 44100.0);
    }

    return paContinue;
}
