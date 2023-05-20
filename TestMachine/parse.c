#include "TM.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// function to parse the url in a http request for the login page
Uri parseUri(const char *uriString) {
    Uri uri;
    char copy[500];
    strcpy(copy, uriString);

    char *question = strchr(copy, '?');
    if(question) {
        *question = 0;
        strcpy(uri.queryString, question + 1);
    } else {
        uri.queryString[0] = 0;
    }

    strcpy(uri.path, copy);

    printf("Uri is: %s\n", uri.path);
    return uri;
}

// Function to parse the http request by parsing the request line and headers
HttpRequest parseHttpRequest(const char *request) {
    HttpRequest httpRequest;
    printf("Inside request.\n");
    char copy[20000]; // Assumes that the request won't exceed 20000 characters
    strcpy(copy, request); // Make a copy because strtok modifies the string

    char *line = strtok(copy, "\n");
    // DEBUG: Print the request string
    printf("Request string: %s\n", request);

    // Parse request line
    char uriString[500];
    sscanf(line, "%s %s %s", httpRequest.requestLine.method, uriString, httpRequest.requestLine.version);
    printf("Uri string is: %s\n", uriString);
    httpRequest.requestLine.uri = parseUri(uriString);

    // Parse headers
    httpRequest.headerCount = 0;
    while(line != NULL) {
        line = strtok(NULL, "\n");
        if(strlen(line) < 2) { // this is an empty line, end of headers
            break;
        }

        char *colon = strchr(line, ':');
        if(colon) {
            *colon = 0; // replace colon with null-terminator
            strcpy(httpRequest.headers[httpRequest.headerCount].fieldname, line);
            strcpy(httpRequest.headers[httpRequest.headerCount].value, colon + 2); // skip colon and space
            httpRequest.headerCount++;
        }
    }
    // DEBUG: Print some parts of the HttpRequest after parsing
    printf("Parsed HttpRequest: method=%s, uri=%s, version=%s\n",
    httpRequest.requestLine.method,
    httpRequest.requestLine.uri.path,
    httpRequest.requestLine.version);


    return httpRequest;
}