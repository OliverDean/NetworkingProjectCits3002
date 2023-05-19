#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define TMPORT "4125"
#define PQBPORT "4126"
#define CQBPORT "4127"
#define BACKLOG 10

typedef struct curUser
{
    char username[32];
    char password[32];
    char *QB[10];         // Each question is c or python, indicating where its
    char *QuestionID[10]; // Indicating what the questionID is
    int attempted;        // Max of 10
    int questions[10];    // Each has a max of 3
    int score[10];        // Max of 3
    int total_score;      // Max of 30
    char user_filename[13];
} curUser;
curUser user;

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

typedef enum {
    HTML,
    CSS,
    JS,
    JPEG
} ContentType;

// Loads user data into structure
// Returns -1 on failure with file (corrupted / not built correctly)
// Returns 0 on success
int loadUser(curUser *user)
{
    printf("Inside load user.\n");

    printf("filename: %s\n", user->user_filename);
    char *line = NULL;
    char *buf = NULL;
    size_t linesize = 0;
    ssize_t linelen = 0;
    int QBcounter = 0;
    memset(user->questions, 0, sizeof(user->questions)); // make array all 0
    user->total_score = 0;
    user->attempted = 0;

    printf("Opening file");

    FILE *fp;
    fp = fopen(user->user_filename, "r"); // Open user file
    if (fp == NULL)
    {
        perror("fopen");
        return -1;
    }

    while ((linelen = getline(&line, &linesize, fp)) != -1)
    {
        printf("this is the counter: %d\n", QBcounter);
        printf("line is %s\n", line);
        
        buf = strtok(line, ";");
        
        buf[strcspn(buf, "\n")] = '\0';
        buf[strcspn(buf, "\r")] = '\0';
        if ((strcmp(buf, user->username)) && (strcmp(buf, "q")))
        { // If username in file doesn't match signed in user
            printf("Incorrect username in file!\nUsername in file: %sUsername given:%s\n", buf, user->username);
            return -1;
        }
        else if (!strcmp(buf, "q"))
        { // question line
            char *t = NULL;
            buf = strtok(NULL, ";"); // QB its from
            user->QB[QBcounter] = malloc(strlen(buf));
            strcpy(user->QB[QBcounter], buf);
            user->QB[QBcounter][strlen(buf)] = '\0';

            printf("QB is: %s\n", user->QB[QBcounter]);

            buf = strtok(NULL, ";"); // QuestionID
            user->QuestionID[QBcounter] = malloc(strlen(buf));
            strcpy(user->QuestionID[QBcounter], buf);
            user->QuestionID[QBcounter][strlen(buf)] = '\0';

            printf("QuestionID is: %s\n", user->QuestionID[QBcounter]);

            buf = strtok(NULL, ";"); // Attempted marks

            printf("Attempted marks: %s\n", buf);
            printf("First attempted mark is: %c\n", *buf);

            // For the marks string
            for (t = buf; *t != '\0'; t++) {
                if (*t == 'N')
                { // If user has answered incorrectly
                    if (user->questions[QBcounter] == 0)
                        user->attempted++;
                    printf("User has attempted %d questions.\n", user->attempted);
                    user->questions[QBcounter]++;
                }
                else if (*t == 'Y')
                { // If user has answered correctly
                    user->score[QBcounter] = 3 - user->questions[QBcounter];
                    printf("Score is: %d\n", user->score[QBcounter]);
                    user->total_score += user->score[QBcounter];
                    printf("Total score is: %d\n", user->total_score);
                    if (user->questions[QBcounter] == 0)
                    {
                        user->attempted++;
                        printf("User has attempted %d questions.\n", user->attempted);
                    }
                    break;
                }
                else if (*t == '-')
                {
                    printf("Hasn't attempted yet.\n");
                    break;
                }
            }
            QBcounter++;
        }
    }
    fclose(fp);

    if (QBcounter == 10) // If there are 10 questions
        return 0;
    else if (QBcounter < 10) { // If not 10 questions, error out
        printf("Returning -1 from loaduser.\n");
        return -1;
    }
    else if (QBcounter <= 0) // If somehow Qbcounter is below 0 (if we get here we are screwed)
        return -1;
}

// Generates random string for user cookie file
// Returns said string
char *randomStringGenerator()
{
    static char stringSet[] = "abcdefghijklmnopqrstuvwxyz1234567890";
    char *randomstring = malloc(sizeof(char *) * 8 + 1);
    srand(time(NULL));

    for (int i = 0; i <= 8; i++)
    {
        int k = rand() % (int)(sizeof(stringSet) - 1);
        randomstring[i] = stringSet[k];
    }
    randomstring[8] = '\0';
    return randomstring;
}

// Logs user in, Returns 0 on success,
// 1 on password failure, returns -1 on login failure, returns -2 if user file is missing
int login(char username[], char password[], curUser *user)
{
    char temp[32];
    char *line = NULL;
    char *buf;
    size_t linesize = 0;
    ssize_t linelen;
    int counter = 0;
    FILE *fpa = fopen("users.txt", "r");

    while ((linelen = getline(&line, &linesize, fpa)) != -1)
    {
        buf = strtok(line, ";");
        if (!strcmp(buf, "//")) // If comment line
            continue;
        if (!strcasecmp(buf, username))
        { // If strings equal (ignoring case-sensitive for username)
            strcpy(temp, buf);
            buf = strtok(NULL, ";");
            if (!strcmp(buf, password))
            { // If passwords equal (must be case-sensitive)
                strcpy(user->password, buf);
                strcpy(user->username, temp);
                buf = strtok(NULL, ";");
                buf[strcspn(buf, "\n")] = '\0';
                buf[strcspn(buf, "\r")] = '\0';
                printf("File looking at is: %s\n", buf);
                if (buf == NULL || *buf == '\0')
                {
                    return -2;
                }
                strcpy(user->user_filename, buf);
                fclose(fpa);
                return 0;
            }
            else
            {
                return 1;
            }
        }
        else
            continue;
    }
    return -1;
}


const char* getContentTypeString(ContentType contentType) {
    switch(contentType) {
        case HTML: return "text/html";
        case CSS: return "text/css";
        case JS: return "application/javascript";
        case JPEG: return "image/jpeg";
        default: return "text/plain";
    }
}

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

    return uri;
}

// Function to parse the http request by parsing the request line and headers
HttpRequest parseHttpRequest(const char *request) {
    HttpRequest httpRequest;
    char copy[20000]; // Assumes that the request won't exceed 20000 characters
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

// Returns 0 on success, -1 on failure
// Returns socket file descriptor
int setupsocket(char *port)
{
    int rv, socketfd, yes = 1;
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof(hints)); // Make the struct empty
    hints.ai_family = AF_INET;        // Use IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP Stream
    hints.ai_flags = AI_PASSIVE;      // Fill my IP for me

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0)
    {
        perror("getaddrinfo");
        return -1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        // Attempt to create socket
        if ((socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server: socket");
            return -1;
        }

        // Allow the kernel to reuse local addresses when validating in later bind() call
        if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockfd");
            return -1;
        }

        if (bind(socketfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(socketfd);
            perror("server: bind");
            return -1;
        }
        break;
    }
    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        printf("Server failed to bind.\n");
        return -1;
    }

    if (listen(socketfd, BACKLOG) == 1)
    {
        perror("listen");
        return -1;
    }
    return socketfd;
}

// Generates new cookie file for user, adds it to users.txt
// Re-creates users.txt to avoid over-writing user data
void generatenewfile(curUser *user)
{
    char *line = NULL;
    char *buf = NULL;
    size_t linesize = 0;
    ssize_t linelen;
    int counter = 0;
    char *randomstring = randomStringGenerator(); // Generate new user cookie
    strcpy(user->user_filename, randomstring);
    strcat(user->user_filename, ".txt");
    FILE *fp = fopen(user->user_filename, "w"); // Create new user cookie file
    if (fp == NULL)
    {
        perror("fopen");
    }
    FILE *up = fopen("users.txt", "r+");
    if (up == NULL)
    {
        perror("fopen");
    }
    FILE *new = fopen("temptemptemp", "w");
    if (new == NULL)
    {
        perror("fopen");
    }
    while ((linelen = getline(&line, &linesize, up)) != -1)
    {
        counter += (int)linelen;
        fprintf(new, "%s", line);
        buf = strtok(line, ";");
        if (!strcmp(buf, "//"))
        { // If comment line
            continue;
        }
        else if (!strcasecmp(buf, user->username))
        { // Line we want
            buf = strtok(NULL, ";");
            if (!strcmp(buf, user->password))
            {
                buf = strtok(NULL, ";");
                if (*buf == '\n') 
                {
                    counter--;
                }
                else if (buf != NULL)
                { // If user has filename, re-write over it in the next step
                    printf("Found additional user cookie file: %s\n", buf);
                    counter -= sizeof(buf);
                    counter -= 2; // Take into account the 2 lost ';'
                    remove(buf);
                }
                fseek(new, counter, SEEK_SET);             // Move up file pointer to end of line (after users password)
                fprintf(new, "%s;\n", user->user_filename); // Add users cookie filename
                fprintf(fp, "%s;\n", user->username);       // Add users username to their own cookie file
            }
            else
            {
                continue;
            }
        }
        else
            continue;
    }
    free(line);
    fclose(new);
    fclose(up);
    fclose(fp);
    remove("users.txt");
    rename("temptemptemp", "users.txt");
}

// Code for both question banks
// They will share the same code, however pipe[] will be different for both QB's
// Will automatically break out once connection is broken.
void QuestionBanks(int QBsocket, int pipe[2], char *QBversion)
{
    while (1)
    {
        curUser user;
        char commandbuffer[3] = {0};
        if (read(pipe[0], commandbuffer, 3) == -1) // Wait for instructions
        {
            perror("read");
            break;
        }
        if (!strcmp(commandbuffer, "GQ")) // Generate Questions
        {
            int temp = 0;
            char questionIDbuffer[32] = {0};
            char filename[13] = {0};
            int length = 0;
            char *buf = NULL;
            printf("Received gq request..\n");
            if (send(QBsocket, "GQ", 2, 0) == -1) // Send QB what it needs to prep for
            {
                perror("send");
            }
            memset(commandbuffer, 0, sizeof(commandbuffer));
            sleep(0.01);
            if (recv(QBsocket, &length, sizeof(int), 0) == -1)
            {
                perror("recv");
            }
            printf("Size of packet expected is: %d\n", ntohl(length));
            if (recv(QBsocket, questionIDbuffer, ntohl(length), 0) == -1) // Wait for random questionID's
            {
                perror("recv");
            }
            questionIDbuffer[ntohl(length) + 1] = '\0';
            printf("questionID buffer is: %s\n", questionIDbuffer);
            if (read(pipe[0], filename, 8) == -1) // Send data to parent
            {
                perror("read");
            }
            strcat(filename, ".txt");
            strcpy(user.user_filename, filename);
            FILE *ft = fopen(user.user_filename, "a"); // Open file in append mode
            if (ft == NULL)
            {
                perror("fopen");
            }
            buf = strtok(questionIDbuffer, ";"); // Grab the questionID
            while (buf != NULL)
            {
                fprintf(ft, "q;%s;%s;---;\n", QBversion, buf); // Add it to users cookie file
                buf = strtok(NULL, ";");
            }
            printf("Sending verification!\n");
            if (!strcasecmp(QBversion, "c"))
            {
                if (write(pipe[1], "YE", 3) == -1)
                    perror("write");
            }
            else{
                if (write(pipe[1], "YE", 3) == -1)
                    perror("write");
            }
            fclose(ft);
        }
        else if (!strcmp(commandbuffer, "AN"))
        { // If QB's need to answer questions
            printf("meant to answer here..\n");
            char *answer = "#include <stdio>\n\nint main(int argv, char *argv[]){\nprintf(\"Hello World!\");\nreturn 0;}";
            int length = sizeof(answer);
            char *isAnswer = NULL;
            if (send(QBsocket, "AN", 2, 0) == -1) // Send QB what it needs to prep for
            {
                perror("send");
            }
            if (send(QBsocket, &length, htonl(length), 0) == -1) // Send answer string size to QB
            {
                perror("send");
            }
            if (send(QBsocket, answer, sizeof(answer), 0) == -1) // Send answer to QB
            {
                perror("send");
            }
            if (send(QBsocket, user.QuestionID[3], sizeof(user.QuestionID[3]), 0) == -1) // Send QuestionID to QB
            {
                perror("send");
            }
            if (recv(QBsocket, isAnswer, 1, 0) == -1) // Is answer right?
            {
                perror("recv");
            }
            if (!strcmp(isAnswer, "Y"))
                printf("Answer is right\n");
            else
                printf("Answer is wrong\n");
        }
        else if (!strcmp(commandbuffer, "IQ"))
        { // If QB's need to return the answer to a question (3 failed attempts)
            printf("meant to get answer here...\n");
            int length = 0;
            char *answerBuf;
            if (send(QBsocket, "IQ", 2, 0) == -1) // Send QB what it needs to prep for
            {
                perror("send");
            }
            if (send(QBsocket, user.QuestionID[3], sizeof(user.QuestionID[3]), 0) == -1) // Send QB the questionID
            {
                perror("send");
            }
            if (recv(QBsocket, &length, sizeof(int), 0) == -1) // Get length of answer from QB
            {
                perror("recv");
            }
            if (recv(QBsocket, answerBuf, ntohl(length), MSG_WAITALL) == -1) // Get answer
            {
                perror("recv");
            }
            printf("%s\n", answerBuf);
        }
        else if (!strcmp(commandbuffer, "PQ"))
        { // If QB's need to return the question from questionID
            printf("meant to return the question here...\n");
            int questionlength = 0;
            int optionlength = 0;
            int questionID_index = 0;
            int questionID_check = 0;
            char *questionID = NULL;
            char *questionBuf = NULL;
            char *optionBuf = NULL;
            if (read(pipe[0], &questionID_index, sizeof(int)) == -1) 
            {
                perror("read");
            }
            printf("id is %d\n", questionID_index);
            if (read(pipe[0], questionID, 4) == -1) 
            {
                perror("read");
            }
            printf("Question ID is: %s\n", questionID);
            if (send(QBsocket, "PQ", 2, 0) == -1) // Send QB what it needs to prep for
            {
                printf("Error sending PQ");
                perror("send");
            }
            printf("Send PQ\n");
            printf("Sending index: %s\n", questionID);
            if (send(QBsocket, user.QuestionID[questionID_index], sizeof(user.QuestionID[questionID_index]), 0) == -1) // Send QB the questionID
            {
                printf("Error sending user questionID");
                perror("send");
            }
            printf("Got user ID\n");
            if (recv(QBsocket, &questionlength, sizeof(int), 0) == -1) // Get length of answer from QB
            {
                perror("recv");
            }
            printf("Got question length\n");
            if (recv(QBsocket, questionBuf, ntohl(questionlength), MSG_WAITALL) == -1) // Get answer
            {
                perror("recv");
            }
            printf("got question.\n");
            if (recv(QBsocket, &optionlength, sizeof(int), 0) == -1)
            {
                perror("recv");
            }
            printf("got option length");
            if (recv(QBsocket, optionBuf, ntohl(optionlength), 0) == -1)
            {
                perror("recv");
            }
            printf("got options.\n");
            if (recv(QBsocket, &questionID_check, sizeof(int), 0) == -1)
            {
                perror("recv");
            }
            printf("got questionID/\n");
            if (ntohl(questionID_check) != questionID_index) {
                printf("Error, received wrong question!\n");
                printf("Expected questionID: %d - Got: %d\n", questionID_index, ntohl(questionID_check));
                break;
            }
            printf("Question: %s\n", questionBuf);
            printf("Options: %s\n", optionBuf);
        }
    }
    close(QBsocket);
    close(pipe[0]);
    close(pipe[1]);
}

// Sends an HTTP response with a text-based file. 
// It takes a socket file descriptor, the file path to respond with, and the content type of the file.
void sendHttpResponse(int socket_fd, const char *filePath, ContentType contentType) {
    const char *contentTypeString = getContentTypeString(contentType);
    char header[256];
    printf("session_id %s\n", user.user_filename);
    sprintf(header, "HTTP/1.1 200 OK\nContent-Type: %s\nSet-Cookie: session_id=%s\nContent-Length: ", contentTypeString, user.user_filename);

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

// Function sends an HTTP response that redirects to a different location.
// It takes a socket file descriptor and the target location URL.
void sendRedirectResponse(int socket_fd, const char *location) {
    char header[256];
    sprintf(header, "HTTP/1.1 302 Found\r\nLocation: %s\r\nSet-Cookie: session_id=%s\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n", location, user.user_filename);
    printf("Redirecting to: %s\n", location);
    // Send the HTTP response
    write(socket_fd, header, strlen(header));
}

// The handleRequest function processes an HTTP request and sends the appropriate response.
// It takes a socket file descriptor and an HttpRequest structure.
int handleRequest(int socket_fd, HttpRequest httpRequest) {
    // Determine the file path based on the request
    const char *filePath = NULL;
    ContentType contentType = HTML;
    int pathType = 0;

    // printf("httpRequest.requestLine.uri.path: %s\n", httpRequest.requestLine.uri.path);
    char *qIDs[10] = {"a","b","c","d","e","f","g","h","i","k"};

    char *queryCopy = strdup(httpRequest.requestLine.uri.path);
    char uriID[15];
    char *pch = NULL;
    pch = strtok(queryCopy, "_");
    char *previous = pch;
    // printf("previous: %s\n", previous);

    // pch=strtok(NULL, "_");
    // if (strcmp(previous, "/question") == 0)
    // {
    //     strcpy(uriID, pch);
    // }

    // char *pchID;
    // pchID = strtok(uriID, ".");

    // for (int i=0; i<10; i++)
    // {
    //     if (strcmp(pchID, qIDs[i]) == 0)
    //     {
    //         printf("pchID: %s\n", pchID); 
    //         printf("qIDs[i]: %s\n", qIDs[i]);
    //         filePath="./ClientBrowser/question_coding.html";
    //         contentType= HTML;     
    //         // sendHttpResponse(socket_fd, filePath, contentType);
    //         return pathType;
    //     }
    // }

    if (strcmp(httpRequest.requestLine.uri.path, "/") == 0) {
        filePath = "./ClientBrowser/login.html";
        contentType = HTML;
        pathType = 1;
    } else if (strcmp(httpRequest.requestLine.uri.path, "/login") == 0) {
        filePath = "./ClientBrowser/login.html";
        contentType = HTML;
        pathType = 2;
    } else if (strcmp(httpRequest.requestLine.uri.path, "/logout") == 0) {
        filePath = "./ClientBrowser/logout.html";
        contentType = HTML;
        pathType = 3;
    } else if (strcmp(httpRequest.requestLine.uri.path, "/question_coding") == 0) {
        filePath = "./ClientBrowser/question_coding.html";
        contentType = HTML;
        pathType = 4;
    } else if (strcmp(httpRequest.requestLine.uri.path, "/question_multi") == 0) {
        filePath = "./ClientBrowser/question_multi.html";
        contentType = HTML;
        pathType = 5;
    } else if (strcmp(httpRequest.requestLine.uri.path, "/question_dashboard") == 0) {
        filePath = "./ClientBrowser/question_dashboard.html";
        contentType = HTML;
        pathType = 6;
    } else if (strcmp(httpRequest.requestLine.uri.path, "/styles.css") == 0) {
        filePath = "./ClientBrowser/styles.css";
        contentType = CSS;
        pathType = 7;
    } else if (strcmp(httpRequest.requestLine.uri.path, "/icon.jpg") == 0) {
        filePath = "./ClientBrowser/icon.jpg";
        contentType = JPEG;
        pathType = 8;
        sendImageResponse(socket_fd, filePath, contentType);
    } else if (strcmp(httpRequest.requestLine.uri.path, "/populateDashboard.js") == 0) {
        filePath = "./ClientBrowser/populateDashboard.js";
        contentType = JS;
        pathType = 9;
    } else if (strcmp(httpRequest.requestLine.uri.path, "/populateQuestion.js") == 0) {
        filePath = "./ClientBrowser/populateQuestion.js";
        contentType = JS;
        pathType = 10;
    } else if (strcmp(httpRequest.requestLine.uri.path, "/populateQuestionCoding.js") == 0) {
        filePath = "./ClientBrowser/populateQuestionCoding.js";
        contentType = JS;
        pathType = 11;
    } else if (strcmp(httpRequest.requestLine.uri.path, "/question.js") == 0) {
        filePath = "./ClientBrowser/question.js";
        contentType = JS;
        pathType = 12;
    }
    else {
        // Handle file not found error
        sendHttpResponse(socket_fd, "/error.html", HTML);
        pathType = -1;
        return pathType;
    }
    // Send the HTTP response with the appropriate file
    sendHttpResponse(socket_fd, filePath, contentType);
    return pathType;
}

// Tries to log in with a given username and password.
int attempt_login(int newtm_fd, char *username, char *password) {
    curUser user;
    printf("Attempting login...\n");
    int login_result = login(username, password, &user);
    if (login_result == 0) {
        printf("Login succeeded.\n");
        // Redirect to the question dashboard
        sendRedirectResponse(newtm_fd, "/question_dashboard");
        return 0;
    } else {
        printf("Login failed with error code: %d\n", login_result);
        return login_result;
    }
}

// The displaylogin function reads an HTTP request, extracts a username and password from the query string, and attempts to log in.
int displaylogin(int newtm_fd, char *username, char *password) {
    char buffer[3000];
    read(newtm_fd, buffer, 3000);

    // Parse the HTTP request
    HttpRequest httpRequest = parseHttpRequest(buffer);

    // Extract the username and password from the query string
    int hasUsername = 0;
    int hasPassword = 0;
    if (strchr(httpRequest.requestLine.uri.queryString, '&') != NULL) {
        char *queryCopy = strdup(httpRequest.requestLine.uri.queryString);
        char *pair = strtok(queryCopy, "&");
        printf("Query string: %s\n", httpRequest.requestLine.uri.queryString);

        while (pair != NULL) {
            char *equalsSign = strchr(pair, '=');
            if (equalsSign != NULL) {
                *equalsSign = '\0'; // Replace the equals sign with a null terminator
                char *key = pair;
                char *value = equalsSign + 1;

                if (strcmp(key, "username") == 0) {
                    strcpy(username, value);
                    username[sizeof(username)] = '\0';
                    hasUsername = 1;
                } else if (strcmp(key, "password") == 0) {
                    strcpy(password, value);
                    password[sizeof(password)] = '\0';
                    hasPassword = 1;
                }
            }
            pair = strtok(NULL, "&");
        }
        free(queryCopy);
    }

    // Only attempt login if both username and password are present in the query string
    if (hasUsername && hasPassword) {

        return attempt_login(newtm_fd, username, password);
    } else {
        //Send the login page as the HTTP response
        handleRequest(newtm_fd, httpRequest);
        return 1;
    }
}


// Function to send a question request
void send_question_request(curUser* user, int question_index, int cqbpipe[2], int pyqbpipe[2]) {
    char code[3]; 
    strncpy(code, "PQ", 3);  // Setting the command code to "PQ"

    printf("Sending PQ for question %d\n", question_index + 1);

    // Send the code "PQ" to the correct question bank based on the question type
    int pipe_to_write = (strcmp(user->QB[question_index], "c") == 0) ? cqbpipe[1] : pyqbpipe[1];

    if (write(pipe_to_write, code, 3) == -1) {
        perror("write");
    }

    // Convert the index to network byte order before sending
    uint32_t network_order_index = htonl(question_index);
    if (write(pipe_to_write, &network_order_index, sizeof(network_order_index)) == -1) {
        perror("write");
    }

    // Send the question ID itself to the question bank
    if (write(pipe_to_write, user->QuestionID[question_index], 4) == -1) {
        perror("write");
    }

    printf("Request sent for question ID: %s\n", user->QuestionID[question_index]);
}


//send_question_request(&user, question_index, cqbpipe, pyqbpipe);


// Function to send an answer request
void send_answer_request(curUser* user, int question_index, char* answer, int QBsocket) {
    char code[3]; 
    strncpy(code, "AN", 3);  // Setting the command code to "AN"

    printf("Sending AN for question %d\n", question_index + 1);

    if (send(QBsocket, code, 3, 0) == -1) // Send QB what it needs to prep for
    {
        perror("send");
    }

    // Convert the answer length to network byte order before sending
    uint32_t network_order_length = htonl(strlen(answer));
    if (send(QBsocket, &network_order_length, sizeof(network_order_length), 0) == -1) // Send answer string size to QB
    {
        perror("send");
    }

    if (send(QBsocket, answer, strlen(answer), 0) == -1) // Send answer to QB
    {
        perror("send");
    }

    // Send the user_filename instead of the QuestionID
    if (send(QBsocket, user->user_filename, sizeof(user->user_filename), 0) == -1) 
    {
        perror("send");
    }

    char isAnswer[2];  // Buffer to store the response
    if (recv(QBsocket, isAnswer, 1, 0) == -1) // Is answer right?
    {
        perror("recv");
    }
    isAnswer[1] = '\0';  // Ensure null termination

    if (!strcmp(isAnswer, "Y"))
        printf("Answer is right\n");
    else
        printf("Answer is wrong\n");
}




int main(int argc, char *argv[])
{
    char username[32] = {0};
    char password[32] = {0};

    socklen_t cqb_size, pqb_size, tm_size;
    struct addrinfo cqb_addr, pqb_addr, tm_addr;
    int cqb_fd, pqb_fd, tm_fd;
    int newc_fd, newp_fd, newtm_fd;
    int cqbpipe[2], pqbpipe[2];

    if (pipe(cqbpipe) == -1)
    {
        perror("pipe");
        exit(1);
    }
    if (pipe(pqbpipe) == -1)
    {
        perror("pipe");
        exit(1);
    }

    cqb_fd = setupsocket(CQBPORT);
    pqb_fd = setupsocket(PQBPORT);
    tm_fd = setupsocket(TMPORT);
    if (cqb_fd == -1)
    {
        printf("CQB server failed to bind!\n");
        return -1;
    }
    if (pqb_fd == -1)
    {
        printf("PQB server failed to bind!\n");
        return -1;
    }
    if (tm_fd == -1)
    {
        printf("TM server failed to bind!\n");
        return -1;
    }

    // This is the C Question Bank loop
    // CQB is port 4127
    if (!fork())
    { // child process
        while (1)
        {
            close(pqbpipe[0]); // No need for QB's to communicate
            close(pqbpipe[1]);
            cqb_size = sizeof cqb_addr;
            newc_fd = accept(cqb_fd, (struct sockaddr *)&cqb_addr, &cqb_size);
            if (newc_fd == -1)
            {
                perror("accept");
                break;
            }
            printf("accepted connection from CQB\n");
            QuestionBanks(newc_fd, cqbpipe, "c");
            close(newc_fd);
            close(cqbpipe[0]);
            close(cqbpipe[1]);
        }
    }

    // This is the Python Question Bank loop
    // Accept only one PQB
    // PQB is port 4126
    if (!fork())
    { // child process
        while (1)
        {
            close(cqbpipe[0]); // No need for QB's to communicate
            close(cqbpipe[1]);
            pqb_size = sizeof pqb_addr;
            printf("Waiting for PQB connection...\n");
            newp_fd = accept(pqb_fd, (struct sockaddr *)&pqb_addr, &pqb_size);
            if (newp_fd == -1)
            {
                perror("accept");
                break;
            }
            printf("accepted connection from PQB\n");
            QuestionBanks(newp_fd, pqbpipe, "python");
            close(newp_fd);
            close(pqbpipe[0]);
            close(pqbpipe[1]);
        }
    }

    // Main port is 4125
    while (1)
    {
        memset(&user, 0, sizeof(user)); // Make sure user structure is empty
        memset(username, 0, sizeof(username));
        memset(password, 0, sizeof(password));
        tm_size = sizeof tm_addr;
        char acceptchar[2];
        FILE *fp;
        newtm_fd = accept(tm_fd, (struct sockaddr *)&tm_addr, &tm_size);
        if (newtm_fd == -1)
        {
            perror("accept");
            break;
        }
        printf("connection made!\n");
        //needs to change into if else loops
        //checks the http header and first line.
        //if the first line is a get request, then it will check the username and password

        int dlReturn = displaylogin(newtm_fd, username, password);

        printf("dlReturn: %i\n", dlReturn);
        close(newtm_fd);

        switch (fork())
        {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
            break;
        case 0:
            while (1)
            {
                //close(tm_fd);
                curUser user;
                int loadValue = 0;
                char *cqbverf = NULL;
                char *pqbverf = NULL;
                int loginValue = -1;

                if (dlReturn == 0)
                {
                    printf("here!\n");
                    loginValue = login(username, password, &user);
                }
                
                // printf("LOGIN VALUE: %i\n", loginValue);
                
                if (loginValue == -1) // Invalid Username (doesn't exist)
                {
                    // if (send(newtm_fd, "Username Invalid.\n", 19, 0) == -1)
                    //     perror("send");
                    // continue;
                }
                else if (loginValue == 1) // Invalid Password
                {
                    if (send(newtm_fd, "Incorrect Password.\n", 21, 0) == -1)
                        perror("send");
                    continue;
                }
                // printf("User signed in!\n");
                // printf("User filename is: %s\n", user.user_filename);

                if (loginValue == 0) { // If file does exist
                    printf("File exists.\n");
                    loadValue = loadUser(&user);
                    printf("loadvalue is: %d\n", loadValue);
                }
                
                while (loginValue == -2 || loadValue == -1) // If file failed to open
                {
                    printf("Calling generate new file.\n");
                    generatenewfile(&user);
                    printf("Generated new cookiefile\n");
                    printf("user->user.filename: %s\n", user.user_filename);
                    printf("sending gq request\n");
                    if (write(cqbpipe[1], "GQ", 3) == -1) // Generate questions from C QB
                    {
                        perror("write");
                    }
                    if (write(cqbpipe[1], user.user_filename, 8) == -1)
                    {
                        perror("write");
                    }
                    printf("C QB success.\n");
                    if (write(pqbpipe[1], "GQ", 3) == -1) // Generate questions from Python QB
                    {
                        perror("write");
                    }
                    if (write(pqbpipe[1], user.user_filename, 8) == -1)
                    {
                        perror("write");
                    }
                    printf("received gq requests.\n");
                    if (cqbverf == NULL && pqbverf == NULL) 
                    {
                        if (read(cqbpipe[0], cqbverf, 3) == -1)
                            printf("error1\n");
                            perror("read");
                        if (read(pqbpipe[0], pqbverf, 3) == -1)
                            printf("error2\n");
                            perror("read");
                    }
                    printf("Verifications: c %s python %s", cqbverf, pqbverf);
                    loadValue = loadUser(&user);
                    if (loadValue == 0)
                        loginValue = 0;
                }

                if (loadValue == 0 && loginValue == 0) // Everything Works!
                {
                    char code[2] = {0};
                    /*
                    Here is where the HTTP requests will come through once the user is signed in
                    In here all user data is loaded into the structure
                    You will need to send off the 3 byte (2 bytes sent to the python QB cause C has null terminated strings) to the correct QB through the pipes
                    cqbpipe[1] is the input for the C QB, cqbpipe[0] is the output from the C QB
                    pqbpipe[1] is the input for the Python QB, pqbpipe[1] is the output from the Python QB
                    REMEMBER: CQB NOR PQB HAVE THE CORRECT USER STRUCTURE, YOU WILL NEED TO PASS THEM THE REQUIRED INFO.
                    After the user closes the browser make sure the connection is broken (goes through below close() steps)
                    */

                    printf("successful signin.\n");
                    // while (1) // Testing QB connection
                    // {
                    //     int indexbuf = 0;
                    //     int index = 0;
                    //     memset(code, 0, sizeof(code));
                    //     if (send(newtm_fd, "Please enter a code: ", 20, 0) == -1)
                    //         perror("send");
                    //     if (recv(newtm_fd, code, 2, 0) == -1)
                    //         perror("recv");
                    //     code[strcspn(code, "\n")] = '\0';
                    //     code[strcspn(code, "\r")] = '\0';

                    //     if (!strcmp(code, "PQ")) {
                    //         if (send(newtm_fd, "Please enter a question index: ", 31, 0) == -1)
                    //             perror("send");
                    //         if (recv(newtm_fd, &indexbuf, 4, 0) == -1)
                    //             perror("recv");
                    //         index = ntohl(indexbuf);
                    //         code[strcspn(code, "\n")] = '\0';
                    //         code[strcspn(code, "\r")] = '\0';
                    //         if (strcasecmp(user.QB[2], "c"))
                    //         {
                    //             printf("Sending PQ to c\n");
                    //             if (write(cqbpipe[1], "PQ", 3) == -1)
                    //             {
                    //                 perror("write");
                    //             }
                    //             if (write(cqbpipe[1], &index, sizeof(int)) == -1) // Question 2 is index 3
                    //             {
                    //                 perror("write");
                    //             }
                    //             if (write(cqbpipe[1], user.QuestionID[index], 4) == -1) // Question 2 is index 3
                    //             {
                    //                 perror("write");
                    //             }
                    //             printf("Grabbing question");
                    //         }
                    //         else if(strcasecmp(user.QB[2], "python"))
                    //         {
                    //             printf("Sending PQ to p\n");
                    //             if (write(pqbpipe[1], "PQ", 3) == -1)
                    //             {
                    //                 perror("write");
                    //             }
                    //             if (write(pqbpipe[1], &index, sizeof(int)) == -1) // Question 2 is index 3
                    //             {
                    //                 perror("write");
                    //             }
                    //             printf("Question ID is: %s\n", user.QuestionID[index]);
                    //             if (write(pqbpipe[1], user.QuestionID[index], 4) == -1) // Question 2 is index 3
                    //             {
                    //                 perror("write");
                    //             }
                    //             printf("Grabbing question");
                    //         }
                    //     }
                    // }
                    // //send(newtm_fd, questionDashboard(), strlen(questionDashboard()), 0);
                    close(newtm_fd);
                    close(cqbpipe[1]);
                    close(cqbpipe[0]);
                    close(pqbpipe[0]);
                    close(pqbpipe[1]);
                    break;
                }
            }
            break;
        default:
            close(newtm_fd);
            break;
        }
    }
    shutdown(tm_fd, SHUT_RDWR);
    close(tm_fd);
    return 0;
}