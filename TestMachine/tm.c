#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>

#define HTTP_port 9002
#define QB_port 30002
#define MAXDATASIZE 256

// CLIENT SOCKET CONNECTION
// Browser (client) <--> TM (server) <--> QB's (clients)

// CLIENT workflow: socket() --> connect() --> send()
// SERVER workflow: socket() --> bind() --> listen() --> accept() --> send()

int main()
{
    char buf[MAXDATASIZE] = "You have reached the server";

    int socket_fd;
    // make a socket:
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    // same as client:
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

    // listen to start listening to connections, second argument is how many connections can be waiting for a connection  
    listen(socket_fd, 5);

    int clientSocket_fd;
    // accepting the client socket, creating 2 way connection between client and server:
    clientSocket_fd = accept(socket_fd, NULL, NULL); // these double NULLs need to go later!

    send(socket_fd, buf, sizeof(buf), 0);

    close(socket_fd);
}