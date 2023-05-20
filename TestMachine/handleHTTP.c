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
void handleRequest(int socket_fd, HttpRequest httpRequest, curUser user, int cqbpipe[2], int pqbpipe[2]) {
    printf("Inside handlerequest\n");
    char *filePath = NULL;
    ContentType contentType = HTML;
    printf("Username is: %s\n", user.user_filename);
    // DEBUG: Print the received URI
    printf("Received URI: %s\n", httpRequest.requestLine.uri.path);
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
        sendImageResponse(socket_fd, filePath, contentType, user.user_filename);
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
    } else if (strstr(httpRequest.requestLine.uri.path, "/question.html") != NULL) {
        // Extract the 'id' query parameter from the URI
        char *quest = strstr(httpRequest.requestLine.uri.queryString, "id=");
        if (quest != NULL) {
            quest += strlen("id=");
            char *end = strchr(quest, '&');  // find the end of the 'id' query parameter
            if (end == NULL) end = quest + strlen(quest);
            char questionIndex[4] = {0};
            strncpy(questionIndex, quest, end - quest);
            printf("Question index is: %s\n", questionIndex);

            int questionNumber = atoi(questionIndex);
            if (questionNumber >= 1 && questionNumber <= 100) {
                // Fetch the question from the question bank
                get_question(&user, questionNumber, cqbpipe, pqbpipe);

                // Redirect the client to the "question.html" file with the question number as a query parameter
                char questionHtmlPath[255];
                //sprintf(questionHtmlPath, "./ClientBrowser/question.html?id=%d", questionNumber);
                sprintf(questionHtmlPath, "./ClientBrowser/question.html");
                filePath = questionHtmlPath;
                contentType = HTML;

                sendHttpResponse(socket_fd, filePath, contentType, user.user_filename);
            } else {
                // Invalid question number
                filePath = "./ClientBrowser/error.html";
                contentType = HTML;
            }
        }
    }

    //printf("No similarities found in handleRequest.\n");

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
        // DEBUG: Print filePath and contentType before sending response
        printf("Sending HTTP Response: filePath=%s, contentType=%d\n", temp, contentType);

        // const char *filename = user.user_filename[0] != '\0' ? user.user_filename : "no filepath";
        // printf("user_filename before sending HTTP response: %s\n", filename);
        sendHttpResponse(socket_fd, temp, contentType, user.user_filename);
    } else {
        // If filePath is null, send error response
        printf("File path null: %s\n", filePath);
        //const char *filename = user.user_filename[0] != '\0' ? user.user_filename : "error";
        sendHttpResponse(socket_fd, "./ClientBrowser/login.html", HTML, user.user_filename);
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

    FILE *file = fopen(filePath, "r");
    if (file == NULL) {
        perror("fopen");
        return;
    }

    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    if(length == -1){
        perror("ftell");
        fclose(file);
        return;
    }
    fseek(file, 0, SEEK_SET);

    char *fileText = (char*)malloc(length * sizeof(char));
    if(fileText == NULL){
        perror("malloc");
        fclose(file);
        return;
    }

    size_t read_length = fread(fileText, sizeof(char), length, file);
    if(read_length != length){
        perror("fread");
        free(fileText);
        fclose(file);
        return;
    }

    int total_length = strlen(header) + strlen(fileText) + (sizeof(char) * 12);
    char *fullhttp = malloc(sizeof(char) * total_length);
    if(fullhttp == NULL){
        perror("malloc");
        free(fileText);
        fclose(file);
        return;
    }

    int snprintf_length = snprintf(fullhttp, total_length, "%s%d\n\n%s", header, length, fileText);
    if(snprintf_length < 0 || snprintf_length >= total_length){
        perror("snprintf");
        free(fileText);
        free(fullhttp);
        fclose(file);
        return;
    }

    ssize_t write_length = write(socket_fd, fullhttp, strlen(fullhttp));
    if(write_length != strlen(fullhttp)){
        perror("write");
    }

    fclose(file);
    free(fileText);
    free(fullhttp);
}


// Same as sendHttpResponse, but for images
void sendImageResponse(int socket_fd, const char *filePath, ContentType contentType, const char *user_filename) {
    const char *contentTypeString = getContentTypeString(contentType);
    char header[256];

    if (*user_filename != 0)
        sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nSet-Cookie: session_id=%s\r\n", contentTypeString, user_filename);
    else if (*user_filename == 0)
        sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n", contentTypeString);

    FILE *file = fopen(filePath, "rb");
    if (file == NULL) {
        perror("fopen");
        return;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    if (length == -1) {
        perror("ftell");
        fclose(file);
        return;
    }
    fseek(file, 0, SEEK_SET);

    char contentLengthHeader[64];
    sprintf(contentLengthHeader, "Content-Length: %ld\r\n", length);

    ssize_t write_length = write(socket_fd, header, strlen(header));
    if (write_length != strlen(header)) {
        perror("write");
        fclose(file);
        return;
    }

    ssize_t write_length2 = write(socket_fd, contentLengthHeader, strlen(contentLengthHeader));
    if (write_length2 != strlen(contentLengthHeader)) {
        perror("write");
        fclose(file);
        return;
    }

    unsigned char *imageData = (unsigned char*)malloc(length * sizeof(unsigned char));
    if (imageData == NULL) {
        perror("malloc");
        fclose(file);
        return;
    }

    size_t bytesRead = fread(imageData, sizeof(unsigned char), length, file);
    if (bytesRead != length) {
        perror("fread");
        free(imageData);
        fclose(file);
        return;
    }

    ssize_t bytesSent = send(socket_fd, imageData, length, 0);
    if (bytesSent != length) {
        perror("send");
    }

    fclose(file);
    free(imageData);
}
