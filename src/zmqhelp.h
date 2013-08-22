#ifndef _zmqhelp_h
#define _zmqhelp_h

// Receive 0MQ string from socket and convert into C string
// Caller must free returned string. Returns NULL if the context
// is being terminated.
char * s_recv (void *socket);

// Convert C string to 0MQ string and send to socket
int s_send (void *socket, char *string);

#endif
