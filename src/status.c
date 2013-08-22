#include <ncurses.h>
#include <zmq.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "zmqhelp.h"

static char *blank;

WINDOW *status_window_init()
{
    int x, y, width, height;
    WINDOW *status_window;

    initscr();
    cbreak();   // line buffering disabled, pass on everything to me

    height = 1;
    width = COLS;
    y = LINES - height;
    x = 0;

    refresh();

    status_window = newwin(height, width, y, x);
    wrefresh(status_window);
    return status_window;
}

void *status_thread_start(void *zcontext)
{
    WINDOW *status_window;
    status_window = status_window_init();

    void *receiver = zmq_socket(zcontext, ZMQ_PULL);
    zmq_bind (receiver, "tcp://*:5558");

    char *status;
    
    while(1) {
        status = s_recv(receiver);
        mvwprintw(status_window, 0, 0, blank);
        wrefresh(status_window);
        mvwprintw(status_window, 0, 0, status);
        wrefresh(status_window);
        free(status);
    }

    zmq_close(receiver);
    delwin(status_window);
    return NULL;
}

void status_init(void *zcontext)
{
    pthread_t listen_thread;
    pthread_create( &listen_thread, NULL, status_thread_start, zcontext);

    blank = malloc((size_t)COLS);
    memset(blank, ' ', (size_t)COLS);
    blank[COLS-1] = '\0';
}

void *create_status_context(void *zcontext)
{
    void *sender = zmq_socket(zcontext, ZMQ_PUSH);
    zmq_connect (sender, "tcp://localhost:5558");
    return sender;
}

void destroy_status_context(void *sender)
{
    zmq_close(sender);
}

void status_update(void *sender, char *status)
{
    s_send(sender, status);
}


