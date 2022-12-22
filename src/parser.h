#ifndef __PARSER__
#define __PARSER__

#include "constants.h"

typedef struct{
    char username[MAX_URL_SIZE]; 
    char password[MAX_URL_SIZE]; 
    char hostname[MAX_URL_SIZE]; 
    char url_path[MAX_URL_SIZE]; 
    char filename[MAX_URL_SIZE]; 
} URLelements;

char* getIP(char* hostName);
int parseURL(URLelements *elems, char *url);

#endif
