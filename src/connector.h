#ifndef __CONNECTOR__
#define __CONNECTOR__

#include <stdio.h>
#include "constants.h"

int createAndConnectSocket(char* server_ip_addr,uint16_t server_port);

int verifyEndofMessage(char* resp);

int openControlSocket(char* IPAddress, uint16_t server_port, char cmd_buf[], char resp[]);

int readServerMessage(int sockfd,char receive_buf[]);

int login( char* username, char* password, int control_socket, char cmd[], char resp[]);

int enterPassiveMode(int control_socket, char cmd[], char resp[]);

int getDoor(int control_socket, char resp[]);

int openDataSocket(char* IPAddress, int control_socket, char cmd[], char resp[]);

int requestFile( char* URLpath, char* filename ,int control_socket, char cmd[], char resp[]);

int downloadFile(char* filename , int data_socket, char resp[]);

int quit(int control_socket, char cmd[]);


#endif
