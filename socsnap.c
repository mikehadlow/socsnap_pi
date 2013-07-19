#include <stdio.h>
#include <stdlib.h>
#include <oauth.h>
#include <string.h>

#include "dbg.h"
#include "bstrlib.h"
#include "../twitter/twitter.h"

#define KEYVALUE_LENGTH 7 

typedef struct KeyValue { 
    char *key;
    char *value;
} KeyValue;

void destroy_keyvalues(KeyValue *keyvalues)
{
    int i = 0;
    for(i = 0; i < KEYVALUE_LENGTH; i++) {
        if(keyvalues[i].key != NULL) {
            free(keyvalues[i].key);
            free(keyvalues[i].value);
        }
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

char **get_amp_separated_strings(char *input) 
{
    char **values;
    values = malloc(KEYVALUE_LENGTH * sizeof(char*));

    char *remainder;
    int length;
    char amp = '&';

    int i = 0;
    int complete = 0;
    for (i = 0; i < KEYVALUE_LENGTH; i++) {
        remainder = strchr(input, amp);

        if(remainder != NULL) {
            length = remainder - input;

            values[i] = malloc(sizeof(char) * (length + 1));
            values[i] = strncpy(values[i], input, length); 
            values[i][length] = '\0';

            input = remainder + 1;
        } else if (!complete) {
            length = strlen(input);
            values[i] = calloc(sizeof(char), length + 1);
            strcpy(values[i], input);
            complete = 1;
        } else {
            values[i] = NULL;
        }
    }

    return values;
}

KeyValue *get_key_values_from_keyvalue_strings(char **values)
{
    KeyValue *keyvalues;
    keyvalues = malloc(KEYVALUE_LENGTH * sizeof(KeyValue));
    char eq = '=';
    char* value;
    size_t keylength = 0;
    size_t valuelength = 0;

    int i = 0;
    for(i = 0; i < KEYVALUE_LENGTH; i++) {

        if(values[i] != NULL) {
            value = strchr(values[i], eq);
            if(value != NULL) {
                keylength = (value - values[i]);
                valuelength = strlen(value + 1);

                keyvalues[i].key = malloc((keylength + 1) * sizeof(char*));
                keyvalues[i].value = malloc((valuelength + 1) * sizeof(char*));
                
                keyvalues[i].key = strncpy(keyvalues[i].key, values[i], keylength);
                keyvalues[i].key[keylength + 1] = '\0';
                keyvalues[i].value = strcpy(keyvalues[i].value, value + 1);
            } else {
                printf("Invalid key value string: %s\n", values[i]);
            } 
        } else {
            keyvalues[i].key = NULL;
            keyvalues[i].value = NULL;
        }
    }

    return keyvalues;
}

char *build_oauth_header(KeyValue *keyvalues)
{
    size_t extra_chars = 3; // =""
    size_t length = 0;
    int i = 0;
    char *oauth_header;

    for (i = 0; i < KEYVALUE_LENGTH; i++) {
        if(keyvalues[i].key != NULL) {
            length = length + strlen(keyvalues[i].key) + strlen(keyvalues[i].value) + extra_chars;
        }
    }
    length = length + 1;
    oauth_header = malloc(length * sizeof(char));


    return oauth_header;
}

int main(int argc, char *argv[])
{
    printf("Running socsnap ...\n");
//    test_oauth();

    char *test = "first_key=first_value_and_some&second_key=second_value_x&third_key=third_value_and_something_quite_long";
    char **values;
    KeyValue *keyvalues;
    char *oauth_header;

    values = get_amp_separated_strings(test);
    keyvalues = get_key_values_from_keyvalue_strings(values);

    int i = 0;

    for (i = 0; i < KEYVALUE_LENGTH; i++) {
        if(values[i] != NULL) {
            printf("%s\n", values[i]);
            printf("%s => %s\n", keyvalues[i].key, keyvalues[i].value);
        }
    }

    for (i = 0; i < KEYVALUE_LENGTH; i++) {
        if(values[i] != NULL) {
            free(values[i]);
        }
    }

    oauth_header = build_oauth_header(keyvalues);
    printf("OAuth header:\n%s\n", oauth_header);
    
    free(oauth_header);
    free(values);
    destroy_keyvalues(keyvalues);

    return 0;
}
