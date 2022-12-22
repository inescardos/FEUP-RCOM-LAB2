/**      (C)2000-2021 FEUP
 *       tidy up some includes and parameters
 * */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#include "connector.h"


int createAndConnectSocket(char* server_ip_addr,uint16_t server_port){
    /*server address handling*/
    int fd;
    struct sockaddr_in server_addr;
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);        /*server TCP port must be network byte ordered */
    server_addr.sin_addr.s_addr = inet_addr(server_ip_addr);    /*32 bit Internet address network byte ordered*/
    
    /*open a TCP socket*/
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }
    /*connect to the server*/
    if (connect(fd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }

    return fd;
}


int verifyEndofMessage(char* resp){

    char* aux = resp;
    do{
        if(strncmp(aux, "220 ", 4) == 0){
            return 1;
        }

        // to go for the next line
        while(*aux != '\n' && *aux != '\0') aux++;
        if(*aux == '\n') aux++;

    }while(*aux != '\0');

    return 0;
}

int openControlSocket(char* IPAddress, uint16_t server_port, char cmd_buf[], char resp[]){

    int control_socket;

    if ((control_socket = createAndConnectSocket(IPAddress, SERVER_PORT)) < 0) {
        printf("ERROR while creating and connecting socket1\n");
        return -1;
    }

    do{
        size_t bytes_read = read(control_socket, resp, MAX_FILE_SIZE);
        resp[bytes_read] = 0;
    }while(verifyEndofMessage(resp) != 1);

    return control_socket;
}


int readServerMessage(int sockfd,char receive_buf[]){
    size_t bytes_rcv = 0;
    int code = 0;
    while((bytes_rcv = read(sockfd,receive_buf,MAX_FILE_SIZE))){
        receive_buf[bytes_rcv]='\0';
        char *bg_line = receive_buf;

        for(int i = 0; i < bytes_rcv; i=strchr(bg_line,'\n')-receive_buf){
            char number[4];
            strncpy(number, bg_line, 4);
            code = atoi(number); 
        
            if(bg_line[3] == ' '){
                return code;
            }
            bg_line = &receive_buf[i+1];
        }
    };

    return 0;
}


int login( char* username, char* password, int control_socket, char cmd[], char resp[]){

    // username
    sprintf(cmd, "USER %s\n", username);
    if( write(control_socket, cmd, strlen(cmd)) == 0){
        printf("ERROR during login");
        return -1;
    }
    int server_code = readServerMessage(control_socket, resp);
    if(server_code == 331){
        // password
        sprintf(cmd, "PASS %s\n", password);
        if( write(control_socket, cmd, strlen(cmd)) == 0){
            printf("ERROR during login");
            return -1;
        }
        server_code = readServerMessage(control_socket, resp);
    }
    if(server_code != 230){
        printf("Error during login\n");
        printf("Server response code%d\n", server_code);
        return -1;
    }
    printf("Successfully logged in\n");

    return 0;

}

int enterPassiveMode(int control_socket, char cmd[], char resp[]){

    strcpy(cmd, "PASV\n");
    if( write(control_socket, cmd, strlen(cmd)) == 0){
        printf("ERROR while sending username command");
        return -1;
    }
    int server_code = readServerMessage(control_socket, resp);
    if(server_code != 227){
        printf("ERROR reading passive message\n");
        printf("Server response code%d\n", server_code);
        return -1;
    }
    return 0;
}


int getDoor(int control_socket, char resp[]){

    size_t comma_counter=0, door1 =0, door2 = 0;
    for (size_t i = 0; i < strlen(resp) && resp[i] != ')'; i++) {
        if (resp[i] == ',') {
            comma_counter++;
            continue;
        }
        if (comma_counter == 4) {
            door1 *= 10;
            door1 += (resp[i] - '0');

        } else if (comma_counter == 5) {
            door2 *= 10;
            door2 += (resp[i] - '0');
        }
    }

    return door1 * 256 + door2;
}

int openDataSocket(char* IPAddress, int control_socket, char cmd[], char resp[]){

    if( enterPassiveMode(control_socket, cmd, resp)<0 ) return -1;
    printf("Entering passive mode\n");

    int door;
    if((door = getDoor(control_socket,resp)) <0){
        printf("ERROR entering passive mode\n");
        return -1;
    };

    int data_socket;
    if ((data_socket = createAndConnectSocket(IPAddress, door)) < 0) {
        printf("ERROR while creating and connecting data socket\n");
        return -1;
    }

    return data_socket;
}    


int requestFile( char* URLpath, char* filename ,int control_socket, char cmd[], char resp[]){

    sprintf(cmd, "RETR %s/%s\n", URLpath, filename);
    if( write(control_socket, cmd, strlen(cmd)) == 0){
        printf("ERROR while sending RETR command");
        return -1;
    }

    int server_code = readServerMessage(control_socket, resp);
    if(server_code != 150){
        printf("ERROR sending RETR\n");
        printf("Server response code%d\n", server_code);
        return -1;
    }
    printf("Sending RETR command\n");
    return 0;
}

int downloadFile(char* filename , int data_socket, char resp[]){

    int bytes_read;
    FILE* file = fopen(filename, "wb");
    while( (bytes_read = read(data_socket, resp, MAX_FILE_SIZE)) != 0){
        fwrite(resp, 1, bytes_read, file);
    }
    fclose(file);

    return 0;
}

int quit(int control_socket, char cmd[]){

    sprintf(cmd, "QUIT\n");
    if( write(control_socket, cmd, strlen(cmd)) == 0){
        printf("ERROR while sending RETR command");
        return -1;
    }
    printf("Quiting\n");
    return 0;
}
