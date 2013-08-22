#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <time.h>
#include <pthread.h>
#include <zmq.h>
#include <signal.h>
#include <unistd.h>

#include "dbg.h"
#include "bstrlib.h"
#include "cJSON.h"
#include "auth.h"
#include "logger.h"
#include "zmqhelp.h"
#include "status.h"

#define TWITTER_HANDLE "socsnap"

#define SPLASH_MAIN "../graphics/splash_main.raw"
#define SPLASH_READY "../graphics/splash_ready.raw"

static int s_interrupted = 0;

/*
static void s_signal_handler(int signal_value)
{
    writeline("Shuting down.\n");
    s_interrupted = 1;
}

static void s_catch_signals(void)
{
    writeline("Running s_catch_signals.\n");
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
}
*/

void put_framebuffer(char *bitmap)
{
    char str[1024];
    sprintf(str, "cp %s /dev/fb0", bitmap);
    system(str);
}

void send_status(void *zcontext, char *twitterhandle)
{
    void *xmitter = zmq_socket(zcontext, ZMQ_PAIR);
    zmq_connect(xmitter, "inproc://take_picture");

    writeline("[send status] sending to '%s' message\n", twitterhandle);
    s_send(xmitter, twitterhandle);

    zmq_close(xmitter);
}

// libcurl writer callback
size_t write_data(void *buffer, size_t size, size_t nmemb, void *zcontext)
{
    int i = 0;
    char *data;
    data = (char *) buffer;
    size_t realsize = size * nmemb;
    cJSON *root, *text, *user, *screen_name, *entities, *user_mentions, *user_mention_screen_name;

    writeline("[Monitor status callback] Got data: size: %zu, nmemb: %zu\n", size, nmemb);

    if(nmemb < 2) {
        writeline("[Monitor status callback] Unexpected short buffer.\n");
        return realsize;
    }

    if(data[0] == '\r') {
        writeline("[Monitor status callback] Keep alive.\n");
        return realsize;
    }

    if(size == 1) {
        for(i = 0; i < nmemb; i++) {
            if(data[i] == '\r') {
                data[i] = '\0';
                root = cJSON_Parse(data);
                check(root, "root element does not exist");
               
#ifdef DEBUG
                char *rendered;
                rendered = cJSON_Print(root);
                writeline("\nData:\n%s\n\n", rendered);
                free(rendered);
#endif

                text = cJSON_GetObjectItem(root,"text");
                if(text != NULL) {
                    writeline("[monitor status] Text: %s\n", text->valuestring);

                    user = cJSON_GetObjectItem(root,"user");
                    check(user, "user element does not exist.");

                    screen_name = cJSON_GetObjectItem(user,"screen_name");
                    check(screen_name, "screen_name eleemnt does not exist"); 

                    entities = cJSON_GetObjectItem(root, "entities");
                    check(entities, "entities element does not exist");

                    user_mentions = cJSON_GetObjectItem(entities, "user_mentions");
                    if(user_mentions != NULL) {
                        if(user_mentions->child != NULL) {
                            user_mention_screen_name = cJSON_GetObjectItem(user_mentions->child, "screen_name");
                            check(user_mention_screen_name, "user_mentions->screen_name does not exist.");

                            if(strncmp(user_mention_screen_name->valuestring, TWITTER_HANDLE, 140) == 0) {
                                writeline("[monitor status] User: %s\n", screen_name->valuestring);
                                send_status(zcontext, screen_name->valuestring);
                            }
                        }
                    }

                } else {
                    writeline("[Monitor status callback] text element not found\n");
                }

                cJSON_Delete(root);
            }
        }
    }

    return realsize;

error:
    return realsize;
}

void *monitor_status(void *zcontext) {
    char *url = "https://userstream.twitter.com/1.1/user.json?with=user";
    char *method = "GET";

    bstring oauth_header = get_oauth_header(url, method);
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, (const char *)oauth_header->data);

    curl_global_init(CURL_GLOBAL_SSL);

    curl = curl_easy_init();
    if(curl) {
        // only doing this because I can't work out how to set correct certificates
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
#ifdef DEBUG
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, zcontext);

        while(!s_interrupted) {
            res = curl_easy_perform(curl);
            if(res != CURLE_OK) {
                writeline("[ERROR] curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            } 
            writeline("[Monitor Status] lost twitter stream, reconnecting in 60 seconds.");
            sleep(60);
        }
       
        curl_easy_cleanup(curl);
    }
    curl_slist_free_all(headers);
    
    bdestroy(oauth_header);

    return NULL;
}

char *get_time()
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    return asctime(timeinfo);
}

size_t post_picture_callback(void *buffer, size_t size, size_t nmemb, void *zcontext)
{
    // do nothing for now
    size_t realsize = size * nmemb;
    return realsize;
}

size_t header_callback( void *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t realsize = size * nmemb;
    bstring headers = blk2bstr(ptr, (int)realsize);

    printf((char *)headers->data);

    bdestroy(headers);
    return realsize;
}

void post_picture(char *path, char *status)
{
    char *url = "https://api.twitter.com/1.1/statuses/update_with_media.json";
    char *method = "POST";
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;

    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastptr = NULL;

    bstring oauth_header = get_oauth_header(url, method);

    headers = curl_slist_append(headers, (const char *)oauth_header->data);
    headers = curl_slist_append(headers, "Expect:");

    curl_formadd(&formpost, &lastptr,
        CURLFORM_COPYNAME, "status",
        CURLFORM_COPYCONTENTS, status,
        CURLFORM_END); 

    curl_formadd(&formpost, &lastptr,
        CURLFORM_COPYNAME, "media[]",
        CURLFORM_FILE, path,
        CURLFORM_END);

    curl = curl_easy_init();
    
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
#ifdef DEBUG
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, post_picture_callback);
        
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEHEADER, NULL);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            writeline("[ERROR] curl_easy_perform failed: %s\n", curl_easy_strerror(res));
        } 
        writeline("[Post Picture] upload succeeded\n");
    }


    curl_formfree(formpost);
    bdestroy(oauth_header);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
}

void *take_picture_control(void *zcontext)
{
    void *receiver = zmq_socket(zcontext, ZMQ_PAIR);
    zmq_bind(receiver, "inproc://take_picture");

    pthread_t listen_thread;
    pthread_create(&listen_thread, NULL, monitor_status, zcontext);

    void *xmitter = zmq_socket(zcontext, ZMQ_PAIR);
    zmq_connect(xmitter, "inproc://post_picture");

    void *status_context = create_status_context(zcontext);
    char status[80];

    while(!s_interrupted) {
        writeline("[Take picture control] waiting for message.\n");
        char *message = s_recv(receiver);
        if(s_interrupted) {
            writeline("[Take picture control] interrpt received, killing server.\n");
            break;
        }
        writeline("[Take picture control] got message %s, taking picture...\n", message);

        put_framebuffer(SPLASH_READY);
        sprintf(status, "Picture request from @%s", message);
        status_update(status_context, status);
        sleep(5);

        system("raspistill -h 300 -w 300 -o picture.jpg");

        writeline("[Take picture control] picture taken\n");
        
        // put the splash screen back up
        put_framebuffer(SPLASH_MAIN);
        sprintf(status, "Picture taken of @%s", message);
        status_update(status_context, status);

        s_send(xmitter, message);
        free(message);
    }

    destroy_status_context(status_context);
    zmq_close(receiver);
    zmq_close(xmitter);

    return NULL;
}

bstring create_status(char *screen_name)
{
    bstring status = bfromcstr("@");
    bcatcstr(status, screen_name);
    bcatcstr(status, " Your photo taken at ");
    bcatcstr(status, get_time());
    return status;
}

void *post_picture_control(void *zcontext)
{
    void *receiver = zmq_socket(zcontext, ZMQ_PAIR);
    zmq_bind(receiver, "inproc://post_picture");

    pthread_t picture_thread;
    pthread_create(&picture_thread, NULL, take_picture_control, zcontext);

    while(!s_interrupted) {
        writeline("[post picture control] waiting for message.\n");

        char *message = s_recv(receiver);
        if(s_interrupted) {
            writeline("[post picture control] interrupt received, killing server.\n");
            break;
        }
        writeline("[post picture control] got message %s\n", message);

        char *path = "picture.jpg";
        bstring status = create_status(message);
        post_picture(path, (char *)status->data);
        free(message);
    }
    
    zmq_close(receiver);

    return NULL;
}

int main(int argc, char *argv[])
{
    //s_catch_signals();

    writeline("[Main] Running socsnap ...\n");
    
    curl_global_init(CURL_GLOBAL_ALL);
    void *zcontext = zmq_ctx_new();
    status_init(zcontext);

    put_framebuffer(SPLASH_MAIN);

    pthread_t listen_thread;
    pthread_create( &listen_thread, NULL, post_picture_control, zcontext);

    pthread_join( listen_thread, NULL );

    curl_global_cleanup();
    zmq_ctx_destroy(zcontext);
    return 0;
}
