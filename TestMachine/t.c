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
#define BUFFERSIZE 5000

// USER
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
    char user_filename[9];
} curUser;
curUser user;


// HTTP HEADER -> FIELDS
typedef struct {
    char fieldname[500];
    char value[500];
} HeaderField;

// HTTP HEADER -> FIRST LINE -> URL
typedef struct {
    char path[500];
    char queryString[500];
} Uri;

// HTTP HEADER -> FIRST LINE
typedef struct {
    char method[20];
    Uri uri;
    char version[10];
} RequestLine;

// HTTP HEADER
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

int setupsocket(char *port)
{
    int rv, socketfd, yes = 1;
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo) != 0))
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

// Parse the URI into path and query strings
Uri parseUri(char *uri)
{
    Uri uriString;
    char copy[500];

    strcpy(copy, uri);
    char *qMark = strchr(copy, '?');
    if (qMark)
    {
        *qMark = 0;
        strcpy(uriString.queryString, qMark+1);
    }
    else
    {
        uriString.queryString[0] = 0;
    }

    strcpy(uriString.path, copy);
    printf("uriString.path: %s\nuriString.queryString: %s\n", uriString.path, uriString.queryString);
    return uriString;
}

HttpRequest parseHttpRequest(char *buffer)
{
    HttpRequest httpRequest;
    char copy[BUFFERSIZE];
    strcpy(copy, buffer);

    char *line = strtok(copy, "\n");

    // Parse request line
    char uri[500];
    sscanf(line, "%s %s %s", httpRequest.requestLine.method, uri, httpRequest.requestLine.version);
    httpRequest.requestLine.uri = parseUri(uri);

    printf("httpRequest.requestLine.method: %s\nuri: %s\nhttpRequest.requestLine.version: %s\n",httpRequest.requestLine.method, uri, httpRequest.requestLine.version);
    return httpRequest;
}

char* getContentTypeString(ContentType contentType) {
    switch(contentType) {
        case HTML: return "text/html";
        case CSS: return "text/css";
        case JS: return "application/javascript";
        case JPEG: return "image/jpeg";
        default: return "text/plain";
    }
}

char *constructHttpResponse(int newtm_fd, char *path, ContentType contentType)
{
    char *type = getContentTypeString(contentType);
    char *fullhttp = NULL;

    char header[200];
    sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nSet-Cookie: session_id=%s\r\nContent-Length: ", type, user.user_filename);

    char *file = NULL;
    FILE *fp = fopen(path,"r");
    if (fp == NULL)
    {
        printf("sendHttpResponse -> fp -> fopen");
        perror("fopen");
        return fullhttp;
    }

    // Get the length of the file
    fseek(fp, 0, SEEK_END);
    int length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Read the file into a buffer
    file = (char*)malloc(length * sizeof(char));
    fread(file, sizeof(char), length, fp);

    // Construct the full HTTP response
    int total_length = strlen(header) + strlen(file) + (sizeof(char) * 12);
    fullhttp = malloc(sizeof(char) * total_length);
    snprintf(fullhttp, total_length, "%s%d\r\n\r\n%s", header, length, file);

    fclose(fp);
    free(file);
    return fullhttp;
}

int checkUser(int newtm_fd, HttpRequest httpRequest, char *username, char *password) 
{
    int hasUsername = 0;
    int hasPassword = 0;

    if (strchr(httpRequest.requestLine.uri.queryString, '&') != NULL) {
        char *queryCopy = strdup(httpRequest.requestLine.uri.queryString);
        char *pair = strtok(queryCopy, "&");

        // printf("Query string: %s\n", httpRequest.requestLine.uri.queryString);

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

    if (hasUsername && hasPassword)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

// Generates "cookie"
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

// Generates new "cookie".txt file
void generateNewFile(curUser *user)
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

int loadUser(curUser *user)
{
    printf("LOADING USER\n");

    char *line = NULL;
    char *buf = NULL;
    size_t linesize = 0;
    ssize_t linelen = 0;
    int QBcounter = 0;
    memset(user->questions, 0, sizeof(user->questions)); 
    user->total_score = 0;
    user->attempted = 0;

    FILE *fp;
    fp = fopen(user->user_filename, "r");
    if (fp == NULL)
    {
        perror("fopen");
        return -1;
    }
        // Store the content of the file
        char myString[500];

        // Read the content and store it inside myString
        fgets(myString, 100, fp);

        // Print the file content
        printf("%s", myString);

        // Close the file
        fclose(fp);
        return 0;
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
            sleep(1);
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

int main(int argc, char *argv[])
{
    socklen_t cqb_size, pqb_size, tm_size;
    struct addrinfo cqb_addr, pqb_addr, tm_addr;
    int cqb_fd, pqb_fd, tm_fd;
    int newc_fd, newp_fd, newtm_fd;
    int cqbpipe[2], pqbpipe[2];

    if (pipe(cqbpipe) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(pqbpipe) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // socket set up:
    cqb_fd = setupsocket(CQBPORT);
    if (cqb_fd == -1) {return -1;}
    pqb_fd = setupsocket(PQBPORT);
    if (pqb_fd == -1) {return -1;}
    tm_fd = setupsocket(TMPORT);
    if (tm_fd == -1) {return -1;}

    // CQB Loop, port: 4127
    if (!fork())
    {
        while (1)
        {
            close(pqbpipe[0]);
            close(pqbpipe[1]);
            cqb_size = sizeof(cqb_addr);
            printf("Waiting for CQB connection...\n");
            newc_fd = accept(cqb_fd, (struct sockaddr*)&cqb_addr, &cqb_size);
            if (newc_fd == -1) 
            {
                perror("accept");
                break;
            }
            printf("accepted connection from CQB!\n");
            // QuestionBanks(newtm_fd, cqbpipe, "c"); // Listening to requests that go to CQB
            close(newc_fd);
            close(cqbpipe[0]);
            close(cqbpipe[1]);
        }
    }

    // PQB Loop, port: 4126
    if (!fork())
    {
        while (1)
        {
            close(cqbpipe[0]);
            close(cqbpipe[1]);
            pqb_size = sizeof(pqb_addr);
            printf("Waiting for PQB connection...\n");
            newp_fd = accept(pqb_fd, (struct sockaddr *)&pqb_addr, &pqb_size);
            if (newp_fd == -1)
            {
                perror("accept");
                break;
            }
            printf("accepted connection from PQB!\n");
            // QuestionBanks(newtm_fd, pqbpipe, "python"); // Listening to requests that go to PQB
            close(newp_fd);
            close(pqbpipe[0]);
            close(pqbpipe[1]);
        }
    }

    // TM Loop, port: 4125
    while (1)
    {
        // read the file content:
        char buffer[BUFFERSIZE];
        tm_size = sizeof(tm_addr);
        char *cookie;

        FILE *fp;
        char acceptchar[2];
        curUser user;

        newtm_fd = accept(tm_fd, (struct sockaddr *)&tm_addr, &tm_size);
        if (newtm_fd == -1)
        {
            perror("accept");
            break;
        }

        switch(fork())
        {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
            break;

        case 0:
            if (read(newtm_fd, buffer, BUFFERSIZE) == -1)
            {
                perror("read");
                break;
            }

            // parse the http header:
            HttpRequest hr = parseHttpRequest(buffer);

            printf("URI: %s\n",hr->requestLine.uri.queryString);

            // if statements for the different URL we could receive:
            if (strcmp(hr.requestLine.uri.path, "/") == 0) // LOGIN 1
            {
                char *http = constructHttpResponse(newtm_fd, "./ClientBrowser/login.html", HTML);
                if (write(newtm_fd, http, strlen(http)) == -1)
                {
                    printf("write -> login1");
                    perror("write");
                }
            }
            else if (strcmp(hr.requestLine.uri.path, "/login.html") == 0) // LOGIN 2
            {
                char *http = constructHttpResponse(newtm_fd, "./ClientBrowser/login.html", HTML);
                // printf("http:%s\n", http);
                if (write(newtm_fd, http, strlen(http)) == -1)
                {
                    printf("write -> login2");
                    perror("write");
                }
            }
            else if(strcmp(hr.requestLine.uri.path, "/logout.html") == 0) // LOGOUT 
            {
                char *http = constructHttpResponse(newtm_fd, "./ClientBrowser/logout.html", HTML);
                // printf("http:%s\n", http);
                if (write(newtm_fd, http, strlen(http)) == -1)
                {
                    printf("write -> logout");
                    perror("write");
                }
                memset(&cookie, 0, sizeof(cookie));
                printf("closing connection...\n");
                close(newtm_fd);
            }
            else if (strlen(hr.requestLine.uri.queryString) != 0) // if someone is trying to log in:
            {
                printf("IN HERE\n");
                int loginValue = 0;
                int cuReturn = checkUser(newtm_fd, hr, user.username, user.password);

                if (cuReturn == 0) // username and password parsed
                {
                    char *cqbverf = malloc(3);
                    char *pqbverf = malloc(3);

                    // loginValue = login(&user); // check user in users.txt
                    loginValue=0;
                
                    if (loginValue == -2)
                    {
                        printf("file is missing for this user.\n");
                        // first time trying out the test
                        generateNewFile(&user);  //generate file 
                        //load user into the struct for user
                        //check the result of the load
                        //change loginvalue to 0 if all good and loaded in
                    }
                    else if (loginValue == -1 || loginValue) //username/password is wrong
                    {
                        char *http = constructHttpResponse(newtm_fd, "./ClientBrowser/login.html", HTML);
                        // printf("http:%s\n", http);
                        if (write(newtm_fd, http, strlen(http)) == -1)
                        {
                            printf("write -> login3");
                            perror("write");
                        }
                    }
                    else if (loginValue == 0)
                    {

                    }
                }

            }
            else 
            {
                char *http = constructHttpResponse(newtm_fd, "./ClientBrowser/error.html", HTML);
                // printf("http:%s\n", http);
                if (write(newtm_fd, http, strlen(http)) == -1)
                {
                    printf("write -> error");
                    perror("write");
                }
            }
            close(newtm_fd);
            close(cqbpipe[1]);
            close(cqbpipe[0]);
            close(pqbpipe[0]);
            close(pqbpipe[1]);
            break;
        default:
            close(newtm_fd);
            break;
        } }
    shutdown(tm_fd, SHUT_RDWR);
    printf("closing connection...\n");
    close(newtm_fd);
    close(tm_fd);
    return 0;
}