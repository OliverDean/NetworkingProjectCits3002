#include "TM.h"
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

curUser user;

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
    randomstring[8] = '.';
    randomstring[9] = 't';
    randomstring[10] = 'x';
    randomstring[11] = 't';
    randomstring[12] = '\0';
    return randomstring;
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

// Function sends an HTTP response that redirects to a different location.
// It takes a socket file descriptor and the target location URL.
void sendRedirectResponse(int socket_fd, const char *location, const char *user_filename) {
    char header[256];
    sprintf(header, "HTTP/1.1 302 Found\r\nLocation: %s\r\nSet-Cookie: session_id=%s\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n", location, user_filename);
    printf("Redirecting to: %s\n", location);
    printf("t\t\tsession_id %s\n", user_filename);
    // Send the HTTP response
    write(socket_fd, header, strlen(header));
}

// Tries to log in with a given username and password.
int attempt_login(int newtm_fd, char *username, char *password) {
    curUser user;
    printf("Attempting login...\n");
    int login_result = login(username, password, &user);
    if (login_result == 0) {
        printf("Login succeeded.\n");
        
        // Copy the user_filename into a temporary variable
        char temp_filename[9];
        strncpy(temp_filename, user.user_filename, 13);
        
        // Redirect to the question dashboard
        printf("t\t\tsession_id %s\n", temp_filename);
        
        if (!temp_filename[0] == '\0') {
            // Pass the temp_filename to the sendRedirectResponse function
            sendRedirectResponse(newtm_fd, "/question_dashboard", temp_filename);
        }
        
        return 2;
    } else {
        printf("Login failed with error code: %d\n", login_result);
        return login_result;
    }
}

void displaylogin(int newtm_fd, curUser user) {
    char buffer[3000];
    read(newtm_fd, buffer, 3000);

    // Parse the HTTP request
    HttpRequest httpRequest = parseHttpRequest(buffer);

    // Extract the username and password from the query string
    char username[500] = {0};
    char password[500] = {0};
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
                    strncpy(username, value, sizeof(username) - 1);
                    username[sizeof(username) - 1] = '\0';
                    hasUsername = 1;
                } else if (strcmp(key, "password") == 0) {
                    strncpy(password, value, sizeof(password) - 1);
                    password[sizeof(password) - 1] = '\0';
                    hasPassword = 1;
                }
            }
            pair = strtok(NULL, "&");
        }

        free(queryCopy);
    }

    // Only attempt login if both username and password are present in the query string
    if (hasUsername && hasPassword) {
        curUser user;
        printf("Attempting login...\n");
        int login_result = login(username, password, &user);
        if (login_result == 0) {
            //user.user_filename = filename;
            
            // Redirect to the question dashboard
            // HttpRequest httpRequest;
            // strcpy(httpRequest.requestLine.uri.path, "/question_dashboard");
            // handleRequest(newtm_fd, httpRequest);
            sendRedirectResponse(newtm_fd, "/question_dashboard", user.user_filename);

        } else {
            printf("Login failed with error code: %d\n", login_result);
        }
    }

    // Send the login page as the HTTP response
    handleRequest(newtm_fd, httpRequest, user);
}

void get_question(curUser *user, int question_index, int cqbpipe[2], int pyqbpipe[2])
{
    int pipe[2] = {0};
    char *type = NULL;
    char *question = NULL;
    char *option = NULL;
    int typesize = 0;
    int questionsize = 0;
    int optionsize = 0;
    if (strcasecmp(user->QB[question_index], "c")) {
        pipe[1] = cqbpipe[1];
        pipe[0] = cqbpipe[0];
    }
    else {
        pipe[1] = pyqbpipe[1];
        pipe[0] = cqbpipe[0];
    }
    
    if (write(pipe[1], "PQ", 3) == -1)
    {
        perror("write");
    }
    if (write(pipe[1], &question_index, sizeof(int)) == -1) // Sending question index
    {
        perror("write");
    }
    if (write(pipe[1], user->QuestionID[question_index], 4) == -1) // Sending questionID
    {
        perror("write");
    }
    if (read(pipe[0], &questionsize, sizeof(int)) == -1)
    {
        perror("read");
    }
    question = malloc(questionsize + 1);
    if (read(pipe[0], question, questionsize) == -1)
    {
        perror("read");
    }
    if (read(pipe[0], &optionsize, sizeof(int)) == -1)
    {
        perror("read");
    }
    option = malloc(optionsize + 1);
    if (read(pipe[0], option, optionsize) == -1)
    {
        perror("read");
    }
    if (read(pipe[0], &typesize, sizeof(int)) == -1)
    {
        perror("read");
    }
    type = malloc(typesize + 1);
    if (read(pipe[0], type, typesize) == -1)
    {
        perror("read");
    }
    printf("Question: %s\nOption: %s\nType%s\n", question, option, type);
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

char* loginPage()
{
    printf("Within login page.\n");
    FILE *fp;
    char *logintext = NULL;
    char *fullhttp = NULL;
    fp = fopen("./ClientBrowser/login.html", "r");
    if (fp == NULL) {perror("html file");}
    char *header = "HTTP/1.0 200 OK\nContent-Type: text/html\nContent-Length: ";
    printf("Header is: %s\n", header);
    fseek(fp, 0, SEEK_END);
    int length = ftell(fp); // Grab file length
    fseek(fp, 0, SEEK_SET);
    logintext = (char*)malloc((length) * sizeof(char));       // Buffer for file
    fread(logintext, sizeof(char), length, fp); // Grab entire file
    printf("Login HTML is: %s\n", logintext);
    int total_length = strlen(header) + strlen(logintext) + (sizeof(char) * 12);
    fullhttp = malloc(sizeof(char) * total_length);
    snprintf(fullhttp, total_length, "%s%d\n\n%s", header, length, logintext);
    printf("Full HTTP is: %s\n", fullhttp);
    fclose(fp);
    free(logintext);
    return fullhttp;
}

int handleGETRequest(char *filepath, int newtm_fd)
{
    if(strstr(filepath, "username") != NULL)
    {
        ContentType contenttype = HTML;
        printf("Signing user in!.\n");
        char username[32] = {0};
        char password[32] = {0};
        int counter = 0;
        char *usert = strstr(filepath, "username");
        char *pas = strstr(filepath, "&password");
        usert += 9;
        while (*usert != *pas) 
        {
            username[counter] = *usert;
            counter++;
            usert++;
        }
        counter = 0;
        pas += 10;
        while (*pas != '\0')
        {
            password[counter] = *pas;
            counter++;
            pas++;
        }
        printf("Username: %s\n", username);
        printf("Password: %s\n", password);
        int loginvalue = login(username, password, &user);
        int loadvalue = 0;
        printf("Login value is: %d\n", loginvalue);
        if (loginvalue == 0) {
            loadvalue = loadUser(&user);
            if (loadvalue == -1)
            {
                generatenewfile(&user);
                return -1;
            }
            printf("Loading question dashboard up.\n");
            printf("User file name is: %s\n", user.user_filename);
            sendHttpResponse(newtm_fd, "./ClientBrowser/question_dashboard.html", contenttype, user.user_filename);
            return 0;
        }
        else if (loginvalue == -2) {
            printf("Generating new file..\n");
            generatenewfile(&user);
            return -1;
        }
        else if(loginvalue == -1 || loginvalue == 1) {
            sendHttpResponse(newtm_fd, "./ClientBrowser/question_dashboard.html", contenttype, user.user_filename);
            return 0;
        }
    }
    else if (!strcmp(filepath, "/")|| !strcmp(filepath, "/ClientBrowser/login.html"))
    {
        ContentType contenttype = HTML;
        printf("Getting login page!\n");
        sendHttpResponse(newtm_fd, "./ClientBrowser/login.html", contenttype, user.user_filename);
    }
    return 1;
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
        newtm_fd = accept(tm_fd, (struct sockaddr *)&tm_addr, &tm_size);
        if (newtm_fd == -1)
        {
            perror("accept");
            break;
        }
        printf("connection made!\n");

        switch (fork())
        {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
            break;
        case 0:
            printf("Accepted Client!\n");
            //displaylogin(newtm_fd, user);
            int revalue = 0;
            char getBuffer[4128] = {0};
            if (recv(newtm_fd, getBuffer, 4128, 0) == -1)
            {
                perror("recv");
            }
            HttpRequest htpr = parseHttpRequest(getBuffer);
            printf("Request mode is: %s\n", htpr.requestLine.method);
            printf("\n\nfile path is: %s\n", htpr.requestLine.uri.path);
            printf("\n\nrequest path is: %s\n", htpr.requestLine.uri.queryString);
            if (strstr(htpr.requestLine.uri.queryString, "username") == NULL)
                handleRequest(newtm_fd, htpr, user);
            if (!strcmp(htpr.requestLine.method, "GET")) // GET Request
            {
                printf("Accepted GET Request!\n");
                revalue = handleGETRequest(htpr.requestLine.uri.queryString, newtm_fd);
            }
            if (revalue == -1) // Cookie file broken
            {
                printf("Sending GQ requests.\n");
                printf("User cookie file name is: %s\n", user.user_filename);
                if (write(cqbpipe[1], "GQ", 3) == -1) // Generate questions from C QB
                {
                    perror("write");
                }
                printf("Send request.\n");
                if (write(cqbpipe[1], user.user_filename, 12) == -1)
                {
                    perror("write");
                }
                printf("C QB success.\n");
                if (write(pqbpipe[1], "GQ", 3) == -1) // Generate questions from Python QB
                {
                    perror("write");
                }
                if (write(pqbpipe[1], user.user_filename, 12) == -1)
                {
                    perror("write");
                }
            }
            else if (revalue == 0)
            {
                printf("Successful html send.\n");
            }   
            //send(newtm_fd, questionDashboard(), strlen(questionDashboard()), 0);
            close(newtm_fd);
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