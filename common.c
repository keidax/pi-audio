#include <stdio.h>
#include <netdb.h>

#include "pi_audio.h"

/* Const hints struct to use for all connections */
const struct addrinfo hints = {
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
    .ai_protocol = 0,
    .ai_flags = 0,
    // The rest of these fields don't matter, should just be 0
    .ai_addrlen = 0,
    .ai_canonname = NULL,
    .ai_addr = NULL,
    .ai_next = NULL
};

void common_setup() {
    /* Set up string representations of port values. */
    sprintf(stream_port_str, "%d", STREAM_PORT);
    sprintf(sync_port_str, "%d", FRAMESYNC_PORT);
}
