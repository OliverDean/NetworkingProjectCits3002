#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 4125

int fetch_page(const char *page) {
    int sockfd, readSize;
    struct sockaddr_in servaddr;
    char request[128];
    char response[4096];

    // Create the HTTP GET request
    snprintf(request, sizeof(request), "GET /%s HTTP/1.1\r\n\r\n", page);

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // Assign IP, PORT
    servaddr.sin_family = AF_INET;
    if(inet_pton(AF_INET, "127.0.0.1", &(servaddr.sin_addr))<=0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }
    servaddr.sin_port = htons(PORT);

    // Connect the client socket to server socket
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))!= 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
        printf("connected to the server..\n");

    // Send request to the server
    if (send(sockfd, request, sizeof(request), 0) < 0) {
        perror("Send failed");
        return -1;
    }

    // Receive and print response from the server
    while ((readSize = recv(sockfd, response, sizeof(response) - 1, 0)) > 0) {
        response[readSize] = '\0';
        printf("%s", response);
        
        // Check for HTTP/1.1 200 OK in the response
        if (strstr(response, "HTTP/1.1 200 OK") != NULL) {
            printf("Received OK response from the server.\n");
        } else {
            fprintf(stderr, "Did not receive OK response from the server.\n");
            return -1;
        }
    }

    // Close the socket
    close(sockfd);

    return 0;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    const char *pages[] = {"login.html", "logout.html", "question_multi.html", "question_coding.html", "error.html", "question_dashboard.html"};
    size_t num_pages = sizeof(pages) / sizeof(pages[0]);

    for (size_t i = 0; i < num_pages; i++) {
        if (fetch_page(pages[i]) != 0) {
            return 1;
        }
    }

    return 0;
}
