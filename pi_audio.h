#ifndef _PI_AUDIO_H
#define _PI_AUDIO_H

#define MAX_CHANNELS 2

#define STREAM_PORT     10144
#define FRAMESYNC_PORT  10145
#define BACKLOG         5

/* How many frames of audio the master has played. */
extern unsigned long master_frames_played;

/* Strings representing port values; max is "65536", so 10 bytes is enough. */
char stream_port_str [10];
char sync_port_str [10];

/* Const hints struct to use for all connections */
extern const struct addrinfo hints;

/* Shared setup function called by master and client. */
void common_setup();

#endif // _PI_AUDIO_H
