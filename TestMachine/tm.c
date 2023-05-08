#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>

#define HTTP_port 9002
#define QB_port 30003
#define MAXDATASIZE 256
#define MAXUSERS    5

typedef struct{
    char username[10];
    char password[10];
    char cookie[20];
} User;

User user[MAXUSERS];

// CLIENT SOCKET CONNECTION
// Browser (client) <--> TM (server) <--> QB's (clients)

// CLIENT workflow: socket() --> connect() --> send()
// SERVER workflow: socket() --> bind() --> listen() --> accept() --> send()

void qbSocket()
{
    char buf[MAXDATASIZE] = "You have reached the server";

    int socket_fd;
    // make a socket:
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    // same as client:
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(QB_port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // bind the socket to our specified IP and port
    int bindReturn = bind(socket_fd, (struct sockaddr*) &server_address, sizeof(server_address));
    if (bindReturn == -1)
    {
        close(socket_fd);
        perror("bind");
    }

    // listen to start listening to connections, second argument is how many connections can be waiting for a connection  
    listen(socket_fd, 5);

    int clientSocket_fd;
    // accepting the client socket, creating 2 way connection between client and server:
    clientSocket_fd = accept(socket_fd, NULL, NULL); // these double NULLs need to go later!
    send(clientSocket_fd, buf, sizeof(buf), 0);

}

bool loginCheck(char name[], char password[])
{
    bool userFound = false;
    // check the user name and password against all the user names and password
    // currently stored in the users.txt file
    FILE *fp;
    fp = fopen("users.txt", "r");
    // error checking
    if (fp == NULL) {perror("users file");}
    int i = 0;

    while (fscanf(fp,"%s\n%s\n%s\n", user[i].username, user[i].password, user[i].cookie) != EOF)
    {
        if (strcmp(name, user[i].username) == 0 && strcmp(password,user[i].password) == 0) 
        {
            userFound = true;
        }
        else {
            i++;
        }

        if (feof(fp) && userFound == false)
        {
            printf("No user with that name exists\n");
        }
    }
    fclose(fp);
    return userFound;
} 

void httpSocket()
{
    FILE *fp;
    fp = fopen("index.html", "r");
    // error checking
    if (fp == NULL) {perror("html file");}

    //response header: status code
    char http_header[2048] = "HTTP/1.1 200 OK\r\n\n";
    char line[128];
    char html_data[2048] = {'\0'};
    
    while (fgets(line, sizeof(line), fp))
    {
        strcat(html_data, line);
    };
    strcat(http_header, html_data);
    
    // creating socket
    int socket_fd;
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(HTTP_port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // bind the socket to our specified IP and port
    int bindReturn = bind(socket_fd, (struct sockaddr*) &server_address, sizeof(server_address));
    if (bindReturn == -1)
    {
        close(socket_fd);
        perror("bind");
    }
    listen(socket_fd, 5);

    int clientSocket_fd;
    // keep responding and listening to requests:
    while (1)
    {
        clientSocket_fd = accept(socket_fd, NULL, NULL);
        send(clientSocket_fd, http_header, sizeof(http_header), 0);
    }
}

int main()
{
    // 1. CHECK LOGIN DETAILS AGAINST USER INPUT
    bool login = loginCheck("George", "Michael");
    if (login == 1)
    {
        printf("user matches someone in the database, let them in!");
    }
    else{
        printf("refuse access!!");
    }

    httpSocket();
}