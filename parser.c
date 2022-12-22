
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#include "parser.h"

char* getIP(char* hostName){
    struct hostent *h;
    h = gethostbyname(hostName);
    return inet_ntoa(*((struct in_addr *) h->h_addr));
}

int parseURL(URLelements *elems, char *url){

     // verify FTP protocol
    char *segment = strtok(url, ":");
    if ((segment == NULL) || (strcmp(segment, "ftp") != 0)){
        printf("ERROR while parsing input\n");
        return -1;
    } 
    segment = strtok(NULL, "\0");
    char remainingURL[MAX_URL_SIZE];
    strcpy(remainingURL, segment);


    // parse username
    char aux[MAX_URL_SIZE];
    strcpy(aux, remainingURL);
    segment = strtok(aux, ":");

        // if after the protocol, there is not "//:"
    if (segment == NULL || (strlen(segment) < 3) || (segment[0] != '/') || (segment[1] != '/')){
        printf("ERROR while parsing input\n");
        return -1;
    } 

    else if (strcmp(segment, remainingURL) == 0) {
        memset(elems->username, 0, sizeof(elems->username));
        strcpy(elems->username, "anonymous");

        memset(elems->password, 0, sizeof(elems->password));
        strcpy(elems->password, "");

        char aux2[MAX_URL_SIZE];
        strcpy(aux2, &remainingURL[2]);
        strcpy(remainingURL, aux2);
    }
    else {
        memset(elems->username, 0, sizeof(elems->username)); 
        strcpy(elems->username, &segment[2]);

        // parse password
        segment = strtok(NULL, "@");
        if (segment == NULL || (strlen(segment) == 0))  return -1;
        
        memset(elems->password, 0, sizeof(elems->password));
        strcpy(elems->password, segment);
        segment = strtok(NULL, "\0");
        strcpy(remainingURL, segment);
    }

    // parsing hostname
    segment = strtok(remainingURL, "/");    
    if (segment == NULL) {
          printf("ERROR while parsing input\n");
        return -1;
    }
    memset(elems->hostname, 0, sizeof(elems->hostname));
    strcpy(elems->hostname, segment);

    
    // parse url path and url 
    segment = strtok(NULL, "\0");
    if (segment == NULL) {
        printf("ERROR while parsing input\n");
        return -1;
    }

    char* end_url = strrchr(segment, '/');
    memset(elems->url_path, 0, sizeof(elems->url_path));
    memset(elems->filename, 0, sizeof(elems->filename));

    if (end_url != NULL) {
        strncpy(elems->url_path, segment, end_url - segment);
        strcpy(elems->filename, end_url + 1);

    }
    else{
        strcpy(elems->url_path, "");
        strcpy(elems->filename, segment);
    }

    return 0;
}
