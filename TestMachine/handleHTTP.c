#include "TM.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
// The handleRequest function processes an HTTP request and sends the appropriate response.
// It takes a socket file descriptor and an HttpRequest structure.
void handleRequest(int socket_fd, HttpRequest httpRequest, curUser user) {
    printf("Inside handlerequest\n");
    char *filePath = NULL;
    ContentType contentType = HTML;
    printf("Username is: %s\n", user.user_filename);

    printf("File path is: %s\n", httpRequest.requestLine.uri.path);
    //char *qIDs[10] = {"a","b","c","d","e","f","g","h","i","k"};
    char *queryCopy = strdup(httpRequest.requestLine.uri.path);
    free(queryCopy); // remember to free the allocated memory

    if (strcmp(httpRequest.requestLine.uri.path, "/") == 0 || strcmp(httpRequest.requestLine.uri.path, "/login") == 0) {
        filePath = "./ClientBrowser/login.html";
        printf("File path is: %s\n", filePath);
    } else if (strcmp(httpRequest.requestLine.uri.path, "/logout") == 0) {
        filePath = "./ClientBrowser/logout.html";
    } else if (strcmp(httpRequest.requestLine.uri.path, "/question_coding") == 0) {
        filePath = "./ClientBrowser/question_coding.html";
    } else if (strcmp(httpRequest.requestLine.uri.path, "/question_multi") == 0) {
        filePath = "./ClientBrowser/question_multi.html";
    } else if (strcmp(httpRequest.requestLine.uri.path, "/question_dashboard") == 0) {
        filePath = "./ClientBrowser/question_dashboard.html";
    } else if (strcmp(httpRequest.requestLine.uri.path, "/styles.css") == 0) {
        filePath = "./ClientBrowser/styles.css";
        contentType = CSS;
    } else if (strcmp(httpRequest.requestLine.uri.path, "/icon.jpg") == 0) {
        filePath = "./ClientBrowser/icon.jpg";
        contentType = JPEG;
        sendImageResponse(socket_fd, filePath, contentType);
        return; // Return after sending image response
    } else if (strcmp(httpRequest.requestLine.uri.path, "/populateDashboard.js") == 0) {
        filePath = "./ClientBrowser/populateDashboard.js";
        contentType = JS;
    } else if (strcmp(httpRequest.requestLine.uri.path, "/populateQuestion.js") == 0) {
        filePath = "./ClientBrowser/populateQuestion.js";
        contentType = JS;
    } else if (strcmp(httpRequest.requestLine.uri.path, "/populateQuestionCoding.js") == 0) {
        filePath = "./ClientBrowser/populateQuestionCoding.js";
        contentType = JS;
    } else if (strcmp(httpRequest.requestLine.uri.path, "/question.js") == 0) {
        filePath = "./ClientBrowser/question.js";
        contentType = JS;
    } else if (strstr(httpRequest.requestLine.uri.path, "/question=") != NULL) {
        printf("Inside question grab.\n");
        char *quest = httpRequest.requestLine.uri.path;
        printf("Quest is: %s\n", quest);
        char *end = strstr(httpRequest.requestLine.uri.path, ".html");
        printf("End is %s\n", end);
        char questionIndex[4] = {0};
        quest += 10;
        printf("Quest now is: %s\n", quest);
        int counter = 0;
        while (*quest != *end)
        {
            questionIndex[counter] = *quest;
            quest++;
            counter++;
        }
        printf("Question index is: %s\n", questionIndex); // This is a-g plz change
        
        // Grab question info using PQ command to questionbank
    }
    printf("No similarities found in handleRequest.\n");

    printf("Checking to see if path is true.\n");
    if (*httpRequest.requestLine.uri.path != 0 && filePath == NULL) {
        printf("File path null in check: %s\n", filePath);
        filePath = httpRequest.requestLine.uri.path;
        printf("File path null in check: %s\n", filePath);
        printf("File path is true!!\n");
        printf("Changing filePath to the new file path.\n");
    }

    if (filePath != NULL) {
        char temp[1024] = {0};
        sprintf(temp, ".%s", filePath);
        printf("Temp is: %s\n", temp);
        const char *filename = user.user_filename[0] != '\0' ? user.user_filename : "no filepath";
        printf("user_filename before sending HTTP response: %s\n", filename);
        sendHttpResponse(socket_fd, temp, contentType, user.user_filename);
    } else {
        // If filePath is null, send error response
        printf("File path null: %s\n", filePath);
        const char *filename = user.user_filename[0] != '\0' ? user.user_filename : "butts";
        sendHttpResponse(socket_fd, "./ClientBrowser/error.html", HTML, filename);
    }
}

// Sends an HTTP response with a text-based file. 
// It takes a socket file descriptor, the file path to respond with, the content type of the file, and a session ID.
void sendHttpResponse(int socket_fd, const char *filePath, ContentType contentType, const char *user_filename) {
    const char *contentTypeString = getContentTypeString(contentType);
    char header[256];
    printf("session_id %s\n", user_filename);
    if (*user_filename != 0)
        sprintf(header, "HTTP/1.1 200 OK\nContent-Type: %s\nSet-Cookie: session_id=%s\nContent-Length: ", contentTypeString, user_filename);
    else if (*user_filename == 0)
        sprintf(header, "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length: ", contentTypeString);
    char *fileText = NULL;
    FILE *file = fopen(filePath, "r");
    if (file == NULL) {
        perror("fopen");
        return;
    }

    // Get the length of the file
    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read the file into a buffer
    fileText = (char*)malloc(length * sizeof(char));
    fread(fileText, sizeof(char), length, file);

    // Construct the full HTTP response
    int total_length = strlen(header) + strlen(fileText) + (sizeof(char) * 12);
    char *fullhttp = malloc(sizeof(char) * total_length);
    snprintf(fullhttp, total_length, "%s%d\n\n%s", header, length, fileText);
    
    printf("FULLHTTP IS: %s\n", fullhttp);
    // Send the HTTP response
    write(socket_fd, fullhttp, strlen(fullhttp));

    // Clean up
    fclose(file);
    free(fileText);
    free(fullhttp);
}

// Same as sendHttpResponse, but for images
void sendImageResponse(int socket_fd, const char *filePath, ContentType contentType) {
    const char *contentTypeString = getContentTypeString(contentType);
    char header[128];
    sprintf(header, "HTTP/1.1 200 OK\nContent-Type: %s\n\n", contentTypeString);

    FILE *file = fopen(filePath, "rb");
    if (file == NULL) {
        perror("fopen");
        return;
    }

    // Get the length of the file
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate a buffer for the image data
    unsigned char *imageData = (unsigned char*)malloc(length * sizeof(unsigned char));
    if (imageData == NULL) {
        fclose(file);
        perror("malloc");
        return;
    }

    // Read the image data into the buffer
    size_t bytesRead = fread(imageData, sizeof(unsigned char), length, file);
    if (bytesRead != length) {
        fclose(file);
        free(imageData);
        perror("fread");
        return;
    }

    // Send the HTTP response header
    write(socket_fd, header, strlen(header));

    // Send the image data as the response body
    size_t bytesSent = send(socket_fd, imageData, length, 0);
    if (bytesSent != length) {
        perror("send");
    }

    // Clean up
    fclose(file);
    free(imageData);
}