#include <portaudio.h>
#include <stdio.h>

typedef struct {
    float left_phase;
    float right_phase;
} paTestData;

static paTestData data;

/* This routine will be called by the PortAudio engine when audio is needed. It
 * may be called at interrupt level on some machines so don't do anything that
 * could mess up the system like calling malloc() or free).
 */
static int paTestCallback(const void * inputBuffer, void * outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo * timeInfo,
        PaStreamCallbackFlags statusFlags,
        void * userData) {
    /* Cast data passed through stream to our structure. */
    paTestData * data = (paTestData*) userData;
    float *out = (float*) outputBuffer;
    unsigned int i;
    (void) inputBuffer; /* Prevent unused variable warning. */
    for(i = 0; i < framesPerBuffer; i++) {
        *out++ = data->left_phase;   /* left */
        *out++ = data->right_phase;  /* right */
        /* Generate simple sawtooth phaser that ranges between -1.0 and 1.0. */
        data->left_phase += 0.01f;
        /* When signal reaches top, drop back down. */
        if(data->left_phase >= 1.0f )
            data->left_phase -= 2.0f;
        /* Higher pitch so we can distinguish left and right. */
        data->right_phase += 0.03f;
        if(data->right_phase >= 1.0f)
            data->right_phase -= 2.0f;
    }
    return 0;
}

int main() {
    const PaVersionInfo * version_info = Pa_GetVersionInfo();
    printf("Using %s\n", version_info->versionText);

    /* Initialize PortAudio */
    PaError err;
    err = Pa_Initialize();
    if(err != paNoError) goto error;

    PaStream * stream;
    /* Open an audio stream. */
    err = Pa_OpenDefaultStream( &stream,
                                0,
                                2,
                                paFloat32,
                                44100,
                                256,
                                paTestCallback,
                                &data);
    if(err != paNoError) goto error;

    /* Start the audio stream. */
    err = Pa_StartStream(stream);
    if(err != paNoError) goto error;

    /* Sleep for several seconds. */
    Pa_Sleep(3 * 1000);

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
