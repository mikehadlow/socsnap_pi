#include <stdio.h>
#include <stdlib.h>
#include <oauth.h>
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
#include "../../twitter/twitter.h"

#define KEYVALUE_LENGTH 7 
#define TWITTER_HANDLE "socsnap"

static int s_interrupted = 0;
static void s_signal_handler(int signal_value)
{
    printf("Shuting down.\n");
    s_interrupted = 1;
}

static void s_catch_signals(void)
{
    printf("Running s_catch_signals.\n");
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
}

// Receive 0MQ string from socket and convert into C string
// Caller must free returned string. Returns NULL if the context
// is being terminated.
static char * s_recv (void *socket) 
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
static int s_send (void *socket, char *string) 
{
    int size = zmq_send (socket, string, strlen (string), 0);
    return size;
}

typedef struct bKeyValue {
    bstring key;
    bstring value;
} bKeyValue;

typedef struct bKeyValues {
    int qty;
    bKeyValue *entry;
} bKeyValues;

void destroy_keyvalues(bKeyValues *keyvalues)
{
    int i = 0;
    for(i = 0; i < keyvalues->qty; i++) {
        bdestroy(keyvalues->entry[i].key);
        bdestroy(keyvalues->entry[i].value);
    }
    free(keyvalues->entry);
    free(keyvalues);
}

bstring get_oauth_args(char *url, char *method)
{
    char *consumer_key = CONSUMER_KEY;
    char *consumer_secret = CONSUMER_SECRET;
    char *access_token = ACCESS_TOKEN;
    char *access_token_secret = ACCESS_TOKEN_SECRET;

    char *signed_url = NULL;
    char *postarg = NULL;

    signed_url = oauth_sign_url2(
        url, 
        &postarg, 
        OA_HMAC, 
        method, 
        consumer_key, 
        consumer_secret, 
        access_token, 
        access_token_secret);

    free(signed_url);

    bstring bpostarg = bfromcstr(postarg);
    free(postarg);
    return bpostarg; 
}

bKeyValues *get_key_values(const_bstring input)
{
    int i = 0;
    struct bstrList *values;
    struct bstrList *keyvalue;
    bKeyValues *keyvalues;

    values = bsplit(input, '&');
    keyvalues = malloc(sizeof(bKeyValues));
    keyvalues->entry = malloc(values->qty * sizeof(bKeyValue));
    keyvalues->qty = values->qty;

    for(i = 0; i < values->qty; i++) {
        keyvalue = bsplit(values->entry[i], '=');
        if(keyvalue->qty == 2) {
            keyvalues->entry[i].key = bstrcpy(keyvalue->entry[0]);
            keyvalues->entry[i].value = bstrcpy(keyvalue->entry[1]);
        } else {
            printf("Invalid keyvalue: %s", values->entry[i]->data);
            return NULL;
        }
        bstrListDestroy(keyvalue);
    }

    bstrListDestroy(values);

    return keyvalues;
}

bstring build_oauth_header(bKeyValues *keyvalues)
{
    bstring buffer = bfromcstr("Authorization: OAuth ");
    int i = 0;
    int first = 1;
    
    for(i = 0; i < keyvalues->qty; i++) {
        if(first) {
            first = 0;
        } else {
            bconchar(buffer, ',');
        }
        bconcat(buffer, keyvalues->entry[i].key);
        bcatcstr(buffer, "=\"");
        bconcat(buffer, keyvalues->entry[i].value);
        bconchar(buffer, '\"');
    }
    return buffer;
}

bstring get_oauth_header(char *url, char *method)
{
    bstring args = get_oauth_args(url, method);
    bKeyValues *keyvalues;
    keyvalues = get_key_values(args);
    bstring oauth_header = build_oauth_header(keyvalues);

    bdestroy(args);
    destroy_keyvalues(keyvalues);

    return oauth_header;
}

void send_status(void *zcontext, char *twitterhandle)
{
    void *xmitter = zmq_socket(zcontext, ZMQ_PAIR);
    zmq_connect(xmitter, "inproc://take_picture");

    printf("[send status] sending to '%s' message\n", twitterhandle);
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

    printf("[Monitor status callback] Got data: size: %zu, nmemb: %zu\n", size, nmemb);

    if(nmemb < 2) {
        printf("[Monitor status callback] Unexpected short buffer.\n");
        return realsize;
    }

    if(data[0] == '\r') {
        printf("[Monitor status callback] Keep alive.\n");
        return realsize;
    }

    if(size == 1) {
        for(i = 0; i < nmemb; i++) {
            if(data[i] == '\r') {
                data[i] = '\0';
                root = cJSON_Parse(data);
                check(root, "root element does not exist");
               
                //char *rendered;
                //rendered = cJSON_Print(root);
                //printf("\nData:\n%s\n\n", rendered);
                //free(rendered);

                text = cJSON_GetObjectItem(root,"text");
                if(text != NULL) {
                    printf("[monitor status] Text: %s\n", text->valuestring);

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
                                printf("[monitor status] User: %s\n", screen_name->valuestring);
                                send_status(zcontext, screen_name->valuestring);
                            }
                        }
                    }

                } else {
                    printf("[Monitor status callback] text element not found\n");
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
        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, zcontext);

        while(!s_interrupted) {
            res = curl_easy_perform(curl);
            if(res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            } 
            printf("[Monitor Status] lost twitter stream, reconnecting in 60 seconds.");
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
        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, post_picture_callback);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform failed: %s\n", curl_easy_strerror(res));
        } 
        printf("[Post Picture] upload succeeded\n");
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

    while(!s_interrupted) {
        printf("[Take picture control] waiting for message.\n");
        char *message = s_recv(receiver);
        if(s_interrupted) {
            printf("[Take picture control] interrpt received, killing server.\n");
            break;
        }
        printf("[Take picture control] got message %s, taking picture...\n", message);

        system("cp ../graphics/splash_ready.raw /dev/fb0");
        sleep(5);
        system("raspistill -h 300 -w 300 -o picture.jpg");

        printf("[Take picture control] picture taken\n");
        
        // put the splash screen back up
        system("cp ../graphics/splash_main.raw /dev/fb0");

        s_send(xmitter, message);
    }

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
        printf("[post picture control] waiting for message.\n");
        char *message = s_recv(receiver);
        if(s_interrupted) {
            printf("[post picture control] interrupt received, killing server.\n");
            break;
        }
        printf("[post picture control] got message %s\n", message);

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

    printf("[Main] Running socsnap ...\n");
    system("cp ../graphics/splash_main.raw /dev/fb0");
    curl_global_init(CURL_GLOBAL_ALL);
    void *zcontext = zmq_ctx_new();

    pthread_t listen_thread;
    pthread_create( &listen_thread, NULL, post_picture_control, zcontext);

    pthread_join( listen_thread, NULL );

    curl_global_cleanup();
    zmq_ctx_destroy(zcontext);
    return 0;
}
