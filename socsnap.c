#include <stdio.h>
#include <stdlib.h>
#include <oauth.h>
#include <string.h>

#include "dbg.h"
#include "../twitter/twitter.h"

#define KEYVALUE_LENGTH 7 

typedef struct KeyValue { 
    char *key;
    char *value;
} KeyValue;

KeyValue *get_keyvalues(char *oauth_args)
{
    KeyValue *keyvalues = calloc(sizeof(KeyValue), KEYVALUE_LENGTH);

    return keyvalues;
}

void destroy_keyvalues(KeyValue *keyvalues)
{
    int i = 0;
    for(i = 0; i < KEYVALUE_LENGTH; i++) {
        free(keyvalues[i].key);
        free(keyvalues[i].value);
    }
    free(keyvalues);
}

char *get_oauth_args(char *url)
{
    char *consumer_key = CONSUMER_KEY;
    char *consumer_secret = CONSUMER_SECRET;
    char *access_token = ACCESS_TOKEN;
    char *access_token_secret = ACCESS_TOKEN_SECRET;

    char *method = "POST";
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

    return postarg; 
}

void test_oauth()
{
    char *url = "http://www.example.com/";
    char *postarg = NULL;

    postarg = get_oauth_args(url);

    printf("oauth args: %s\n", postarg);
    printf("%s\n\n", postarg);

    free(postarg);
}

int main(int argc, char *argv[])
{
    printf("Running socsnap ...\n");
//    test_oauth();

    char *test = "first_key=first_value&second_key=second_value&third_key=third_value";
    int amp = '&';
    char *pos = strchr(test, amp);
    printf("%s\n", pos);
    size_t len = pos - test;
    printf("%zu\n", len);
    
    char *first = calloc(sizeof(char), len+1);
    first = strncpy(first, test, len);
    first[len+1] = '\0';

    printf("First: %s\n", first);

    free(first);

    return 0;
}
