#include <stdio.h>
#include <stdlib.h>
#include <oauth.h>
#include <string.h>

#include "dbg.h"
#include "bstrlib.h"
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
    return bfromcstr("test");
}

int main(int argc, char *argv[])
{
    printf("Running socsnap ...\n");
//    test_oauth();

    bstring test = bfromcstr("first_key=first_value_and_some&second_key=second_value_x&third_key=third_value_and_something_quite_long");
    bKeyValues *keyvalues;
    bstring oauth_header;

    keyvalues = get_key_values(test);

    int i = 0;
    for(i = 0; i < keyvalues->qty; i++) {
        printf("%s => %s\n", keyvalues->entry[i].key->data, keyvalues->entry[i].value->data);
    }

    oauth_header = build_oauth_header(keyvalues);
    printf("OAuth header:\n%s\n", oauth_header->data);
    
    bdestroy(test);
    bdestroy(oauth_header);
    destroy_keyvalues(keyvalues);

    return 0;
}
