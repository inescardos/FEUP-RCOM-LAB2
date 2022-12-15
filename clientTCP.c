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

#define SERVER_PORT 21
#define SERVER_ADDR "192.168.28.96"
#define MAX_URL_SIZE 300
#define MAX_FILE_SIZE 10000


// Struct where the parsed URL elements will be stored.
typedef struct{
    char username[MAX_URL_SIZE]; 
    char password[MAX_URL_SIZE]; 
    char hostname[MAX_URL_SIZE]; 
    char url_path[MAX_URL_SIZE]; 
    char filename[MAX_URL_SIZE]; 
} URLelements;


/**
 * @brief This function parses de elements of the URL passed as argument in main.
 * 
 * @param elems struct where the elements will be stored
 * @param url url in a structure similar to:  ftp://[<user>:<password>@]<host>/<url-path>
 * @return 0 if sucess otherwise -1
 */
int parse_URLelements(URLelements *elems, char *url){

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

/**
 * @brief This function is responsible by making the connection to the server
 * 
 * @param server_ip_addr 
 * @param server_port 
 * @return int 
 */
int create_and_connect(char* server_ip_addr,uint16_t server_port){
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

/**
 * @brief This function returns the IP of the hostname
 * 
 * @param hostName 
 * @return char* 
 */
char* getIP(char* hostName){
    struct hostent *h;
    h = gethostbyname(hostName);
    return inet_ntoa(*((struct in_addr *) h->h_addr));
}

/**
 * @brief Verifies if the last packet read is, in fact, the last packet of data. 
 * The importance of this function relies on the fact that
 *  when 2 packets are sent, the first one isn't read properly, so this function exists in order to fix that problem
 * 
 * @param resp 
 * @return 
 */
int verify_end_of_msg(char* resp){

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

int verify_server_code(int sockfd,char receive_buf[]){
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

int main(int argc, char **argv) {

    if (argc !=2){
        printf("ERROR, enough arguments given. Usage: download ftp://anonymous:password@ftp.up.pt/pub/timestamp.txt");
        return -1;
    }
    
    // parse the url given
    URLelements elems;
    if (parse_URLelements(&elems, argv[1]) != 0) return -1;

    // get the ip adress through the hostname
    char* IPAddress = getIP(elems.hostname);

    // create and connect socket1
    int control_socket;
    char cmd[MAX_FILE_SIZE], resp[MAX_FILE_SIZE];  

     if ((control_socket = create_and_connect(IPAddress, SERVER_PORT)) < 0) {
        printf("ERROR while creating and connecting socket1\n");
        return -1;
    }

    // read the initial packets - until it finds "220 "
    do{
        size_t bytes_read = read(control_socket, resp, MAX_FILE_SIZE);
        resp[bytes_read] = 0;
    }while(verify_end_of_msg(resp) != 1);


    // LOGIN
    // username
    sprintf(cmd, "USER %s\n", elems.username);
    if( write(control_socket, cmd, strlen(cmd)) == 0){
        printf("ERROR during login UM");
        return -1;
    }
    int server_code = verify_server_code(control_socket, resp);
    if(server_code == 331){
        // password
        sprintf(cmd, "PASS %s\n", elems.password);
        if( write(control_socket, cmd, strlen(cmd)) == 0){
            printf("ERROR during login DOIS");
            return -1;
        }
        server_code = verify_server_code(control_socket, resp);
    }
    if(server_code != 230){
        printf("Error during login\n");
        printf("Server response code%d\n", server_code);
        return -1;
    }
    printf("Successfully logged in\n");


    // Enter Passive Mode
    strcpy(cmd, "PASV\n");
    if( write(control_socket, cmd, strlen(cmd)) == 0){
        printf("ERROR while sending username command");
        return -1;
    }
    server_code = verify_server_code(control_socket, resp);
    if(server_code != 227){
        printf("ERROR entering passive mode\n");
        printf("Server response code%d\n", server_code);
        return -1;
    }
    printf("Entering passive mode\n");
    
    // parse result
    size_t comma_counter=0, door1 =0, door2 = 0, door = 0;
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
    door = door1 * 256 + door2;

    // create socket TCP and connect
    int data_socket;
    if ((data_socket = create_and_connect(IPAddress, door)) < 0) {
        printf("ERROR while creating and connecting data socket\n");
        return -1;
    }

    // send "retr path" command
    sprintf(cmd, "RETR %s/%s\n", elems.url_path, elems.filename);
    if( write(control_socket, cmd, strlen(cmd)) == 0){
        printf("ERROR while sending RETR command");
        return -1;
    }
    server_code = verify_server_code(control_socket, resp);
    if(server_code != 150){
        printf("ERROR sending RETR\n");
        printf("Server response code%d\n", server_code);
        return -1;
    }
    printf("Sending RETR command\n");

    // receive file
    int bytes_read;
    FILE* file = fopen(elems.filename, "wb");
    while( (bytes_read = read(data_socket, resp, MAX_FILE_SIZE)) != 0){
        fwrite(resp, 1, bytes_read, file);
    }
    fclose(file);


    //send QUIT command
    sprintf(cmd, "QUIT\n");
    if( write(control_socket, cmd, strlen(cmd)) == 0){
        printf("ERROR while sending RETR command");
        return -1;
    }
    printf("Quiting\n");

    return 0;
}


// TESTES PARA FAZER:
// sem login:
// ficheiro grande: ftp://ftp.up.pt/pub/ubuntu-feup-legacy/2014/ubuntu-feup/pacotes.txt
// ficheiro pequeno: ftp://ftp.up.pt/pub/kodi/timestamp.txt
// com login: 
// meio rafado: ftp://anonymous:password@ftp.up.pt/pub/kodi/timestamp.txt
// um bom: 

// cenas para fazer:
// verificar se os códigos retornados estão corretos
