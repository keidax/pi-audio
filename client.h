#ifndef _PI_AUDIO_CLIENT_H
#define _PI_AUDIO_CLIENT_H
#include <netdb.h>
#include <portaudio.h>
#include <sndfile.h>

// Type definitions
typedef struct ourData {
    int sock_fd; // The socket file descriptor to read from
    long frame_length;
} ourData;


// Variable declarations/definitions
uint32_t client_bytes_recvd;
uint32_t client_frames_played;

extern ourData our_data;

// File descriptors for pipe containing audio data
int audio_pipe_fd[2];
SNDFILE * decoded_file;
SF_INFO decoded_info;

// Function declarations
void start_framesync_thread();
void start_streaming_thread();

// PortAudio callback function
int pa_client_callback(const void *, void *, unsigned long,
        const PaStreamCallbackTimeInfo *, PaStreamCallbackFlags, void *);

#endif // _PI_AUDIO_CLIENT_H
