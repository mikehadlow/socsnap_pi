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

char ***get_amp_separated_strings(char *input) 
{
    char **values_p[KEYVALUE_LENGTH] = malloc(sizeof(char*[KEYVALUE_LENGTH]));
    char **values = &values_p;
    char *remainder;
    size_t length;
    char amp = '&';

    int i = 0;
    int complete = 0;
    for (i = 0; i < KEYVALUE_LENGTH; i++) {
        remainder = strchr(input, amp);
        if(remainder != NULL) {
            printf("Found. Remainder is %s\n", remainder);

            length = remainder - input;
            printf("Length is %zd\n", length);

            values[i] = calloc(sizeof(char), length + 1);
            values[i] = strncpy(values[i], input, length);
            values[i][length + 1] = '\0';

            input = remainder + 1;
        } else if (!complete) {
            printf("Last %s\n", input);

            length = strlen(input);
            values[i] = calloc(sizeof(char), length);
            strcpy(values[i], input);
            complete = 1;
        } else {
            values[i] = NULL;
        }
    }

    return values_p;
}

int main(int argc, char *argv[])
{
    printf("Running socsnap ...\n");
//    test_oauth();

    char *test = "first_key=first_value&second_key=second_value&third_key=third_value";
    char ***values_p = get_amp_separated_strings(test);
    char **values = &values_p;
    int i = 0;

    for (i = 0; i < KEYVALUE_LENGTH; i++) {
        if(values[i] != NULL) {
            printf("%s\n", values[i]);
        }
    }

    for (i = 0; i < KEYVALUE_LENGTH; i++) {
        if(values[i] != NULL) {
            free(values[i]);
        }
    }

    free(values_p);

    return 0;
}
