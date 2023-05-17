#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char method[20];
    char url[500];
    char version[10];
} RequestLine;

typedef struct {
    char fieldname[500];
    char value[500];
} HeaderField;

typedef struct {
    RequestLine requestLine;
    HeaderField headers[50];
    int headerCount;
} HttpRequest;

HttpRequest parseHttpRequest(const char *request) {
    HttpRequest httpRequest;
    char copy[2000]; // Assumes that the request won't exceed 2000 characters
    strcpy(copy, request); // Make a copy because strtok modifies the string

    char *line = strtok(copy, "\n");

    // Parse request line
    sscanf(line, "%s %s %s", httpRequest.requestLine.method, httpRequest.requestLine.url, httpRequest.requestLine.version);
    
    // Parse headers
    httpRequest.headerCount = 0;
    while(line = strtok(NULL, "\n")) {
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

    return httpRequest;
}

int main() {
    const char request[] = "GET /ClientBrowser/icon.jpg HTTP/1.0\n";

    HttpRequest httpRequest = parseHttpRequest(request);

    return 0;
}
