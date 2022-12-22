#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#include "connector.c"
#include "parser.c"
#include "constants.h"


int main(int argc, char **argv) {

    if (argc !=2){
        printf("ERROR, enough arguments given. Usage: download ftp://anonymous:password@ftp.up.pt/pub/timestamp.txt");
        return -1;
    }

    URLelements elems;
    char cmd[MAX_FILE_SIZE], resp[MAX_FILE_SIZE];
    int control_socket, data_socket;

    // parse URL
    if (parseURL(&elems, argv[1]) != 0) return -1;
   
   // create control socket 
    char* IPAddress = getIP(elems.hostname);
    if ((control_socket = openControlSocket(IPAddress, SERVER_PORT, cmd, resp)) < 0){
        printf("ERROR while creating and connecting control socket.\n");
        return -1;
    }

    // login into server
    if( login(elems.username, elems.password, control_socket, cmd, resp)< 0){
        printf("ERROR while doing login.\n");
        return -1;
    }

    // create data socket and download file
    if ((data_socket = openDataSocket(IPAddress, control_socket, cmd, resp)) < 0){
        printf("ERROR while creating and connecting data socket.\n");
        return -1;
    }

    // request server for the file
    if(requestFile(elems.url_path, elems.filename, control_socket, cmd, resp) < 0){
        printf("Error while requesting the file to the server.\n");
        return -1;
    }

    // download file
    if (downloadFile(elems.filename, data_socket, resp) < 0){
        printf("ERROR while downloading the selected file.\n");
        return -1;
    }

    // quit
    if(quit(control_socket, cmd) < 0){
        printf("ERROR while closing connection.\n");
        return -1;
    }

    return 0;
}
