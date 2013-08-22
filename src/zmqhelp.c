#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>

// Receive 0MQ string from socket and convert into C string
// Caller must free returned string. Returns NULL if the context
// is being terminated.
char * s_recv (void *socket) 
{
    char buffer [256];
    int size = zmq_recv (socket, buffer, 255, 0);
    if (size == -1)
        return NULL;
    if (size > 255)
        size = 255;
    buffer [size] = 0;
    return strdup (buffer);
}

// Convert C string to 0MQ string and send to socket
int s_send (void *socket, char *string) 
{
    int size = zmq_send (socket, string, strlen (string), 0);
    return size;
}
