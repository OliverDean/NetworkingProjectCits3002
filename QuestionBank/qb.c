#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>

#define HTTP_port 9002
#define MAXDATASIZE 256

// CLIENT SOCKET CONNECTION
// Browser (client) <--> TM (server) <--> QB's (clients)

// CLIENT workflow: socket() --> connect() --> send()
// SERVER workflow: socket() --> bind() --> listen() --> accept() --> send()
int main()
{
    int socket_fd;
    // make a socket:
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    // htons(): setting value to Network Byte Order. Basically you'll want to convert the numbers to Network Byte Order before they 
    // go out on the wire, and convert them to Host Byte Order as they come in off the wire.
    server_address.sin_port = htons(HTTP_port);
    // ... struct in_addr   sin_addr; --> so the struct in_addr {uint32_t   s_addr;};
    // server_address.sin_addr.s_addr references the 4-byte IP address (in Network Byte Order)
    server_address.sin_addr.s_addr = INADDR_ANY;

    int connectReturn = connect(socket_fd, (struct sockaddr *) &server_address, sizeof(server_address));
    if (connectReturn == -1)
    {
        perror("connect");
        exit(1);
    }

    char buf[MAXDATASIZE];
    // receive function
    int recvReturn = recv(socket_fd, &buf, sizeof(buf), 0);
    if (recvReturn == -1)
    {
        perror("recv");
        exit(1);
    }   

    printf("Data sent from the server: %s", buf);

    close(socket_fd);
}
