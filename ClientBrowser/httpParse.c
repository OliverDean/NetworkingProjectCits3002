#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char path[500];
    char queryString[500];
} Uri;

typedef struct {
    char method[20];
    Uri uri;
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

    return uri;
}

HttpRequest parseHttpRequest(const char *request) {
    HttpRequest httpRequest;
    char copy[2000]; // Assumes that the request won't exceed 2000 characters
    strcpy(copy, request); // Make a copy because strtok modifies the string

    char *line = strtok(copy, "\n");

    // Parse request line
    char uriString[500];
    sscanf(line, "%s %s %s", httpRequest.requestLine.method, uriString, httpRequest.requestLine.version);
    httpRequest.requestLine.uri = parseUri(uriString);

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
    const char request[] = "GET /ClientBrowser/icon.jpg?size=100x100 HTTP/1.0\n";

    HttpRequest httpRequest = parseHttpRequest(request);

    printf("Method: %s\n", httpRequest.requestLine.method);
    printf("Path: %s\n", httpRequest.requestLine.uri.path);
    printf("Query: %s\n", httpRequest.requestLine.uri.queryString);
    printf("Version: %s\n", httpRequest.requestLine.version);

    return 0;
}
