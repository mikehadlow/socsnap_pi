#include <stdio.h>
#include <stdlib.h>
#include <oauth.h>
#include <string.h>
#include <curl/curl.h>

#include "dbg.h"
#include "bstrlib.h"
#include "cJSON.h"
#include "../twitter/twitter.h"

#define KEYVALUE_LENGTH 7 

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

bstring get_oauth_args(char *url)
{
    char *consumer_key = CONSUMER_KEY;
    char *consumer_secret = CONSUMER_SECRET;
    char *access_token = ACCESS_TOKEN;
    char *access_token_secret = ACCESS_TOKEN_SECRET;

    char *method = "GET";
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

// libcurl writer callback
size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
    int i = 0;
    char *data;
    data = (char *) buffer;
    size_t realsize = size * nmemb;
    cJSON *root, *text, *user, *screen_name;
    char *rendered;

    printf("Got data: size: %zu, nmemb: %zu\n", size, nmemb);

    if(nmemb < 2) {
        printf("Unexpected short buffer.\n");
        return realsize;
    }

    if(data[0] == '\r') {
        printf("Keep alive.\n");
        return realsize;
    }

    if(size == 1) {
        for(i = 0; i < nmemb; i++) {
            if(data[i] == '\r') {
                printf("Got newline\n");
                data[i] = '\0';
                root = cJSON_Parse(data);
                if(root == NULL) {
                    printf("cJSON_Parse returned NULL\n");
                    return realsize;
                }
                
                rendered = cJSON_Print(root);
                printf("\nData:\n%s\n\n", rendered);
                free(rendered);

                text = cJSON_GetObjectItem(root,"text");
                if(text != NULL) {
                    printf("Text: %s\n", text->valuestring);

                    user = cJSON_GetObjectItem(root,"user");
                    check(user, "user element does not exist.");

                    screen_name = cJSON_GetObjectItem(user,"screen_name");
                    check(screen_name, "screen_name eleemnt does not exist"); 

                    printf("User: %s\n", screen_name->valuestring);
                } else {
                    printf("text element not found\n");
                }

                cJSON_Delete(root);
            }
        }
    }

    return realsize;

error:
    return realsize;
}

void curl_test(bstring oauth_header, char *url) {
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, oauth_header->data);

    printf("\ncurl test ...\n");

    curl_global_init(CURL_GLOBAL_SSL);

    curl = curl_easy_init();
    if(curl) {
        // only doing this because I can't work out how to set correct certificates
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            printf("curl succeded\n");
        }
        curl_easy_cleanup(curl);
    }
    curl_slist_free_all(headers);
}

bstring get_oauth_header(char *url)
{
    bstring args = get_oauth_args(url);
    bKeyValues *keyvalues;
    keyvalues = get_key_values(args);
    bstring oauth_header = build_oauth_header(keyvalues);

    bdestroy(args);
    destroy_keyvalues(keyvalues);

    return oauth_header;
}

void monitor_status()
{
    char *url = "https://userstream.twitter.com/1.1/user.json";

    bstring oauth_header = get_oauth_header(url);
    curl_test(oauth_header, url);
    
    bdestroy(oauth_header);
}


void post_picture()
{
    char *url = "https://api.twitter.com/1.1/statuses/update_with_media.json";
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    bstring oauth_header = get_oauth_header(url);
    headers = curl_slist_append(headers, oauth_header->data);

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform failed: %s\n", curl_easy_strerror(res));
        } else {
            printf("curl succeeded\n");
        }
    }

    bdestroy(oauth_header);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
}

int main(int argc, char *argv[])
{
    printf("Running socsnap ...\n");

    // monitor_status();
    post_picture();

    return 0;
}
