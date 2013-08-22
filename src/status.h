#ifndef _status_h
#define _status_h

void status_init(void *zcontext);

void *create_status_context(void *zcontext);

void destroy_status_context(void *sender);

void status_update(void *sender, char *status);

#endif
