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
#include <fcntl.h>
#include <sys/file.h>

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
    randomstring[8] = '\0';
    return randomstring;
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


char* loginPage()
{
    char *returnString;
    FILE *fp;
    char *logintext = NULL;
    char *fullhttp = NULL;
    fp = fopen("./ClientBrowser/login.html", "r");
    if (fp == NULL) {perror("html file");}
    char *header = "HTTP/1.1 GET /login 200 OK\nContent-Type: text/html\nContent-Length: ";
    fseek(fp, 0, SEEK_END);
    int length = ftell(fp); // Grab file length
    fseek(fp, 0, SEEK_SET);
    logintext = (char*)malloc((length) * sizeof(char));       // Buffer for file
    fread(logintext, sizeof(char), length, fp); // Grab entire file
    int total_length = strlen(header) + strlen(logintext) + (sizeof(char) * 12);
    fullhttp = malloc(sizeof(char) * total_length);
    // printf("%s\n", logintext);
    snprintf(fullhttp, total_length, "%s%d\n\n%s", header, length, logintext);
    fclose(fp);
    strcpy(returnString, fullhttp);
    free(fullhttp);
    free(logintext);
    return returnString;
}

char *questionDashboard()
{
    char *returnString;
    FILE *fp;
    fp = fopen("/Users/karla/NetworkingProjectCits3002/ClientBrowser/question_dashboard.html", "r");
    if (fp == NULL) {perror("html file");}
    char header[5000] = "HTTP/1.1 202 OK\r\n\r\n";
    char line[180];
    while (fgets(line, sizeof(line), fp))
    {
        returnString = strcat(header, line);
    };
    return returnString;
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
        tm_size = sizeof tm_addr;
        FILE *fp;
        char acceptchar[2];
        newtm_fd = accept(tm_fd, (struct sockaddr *)&tm_addr, &tm_size);
        if (newtm_fd == -1)
        {
            perror("accept");
            break;
        }
        memset(username, 0, sizeof(username));
        memset(password, 0, sizeof(password));

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
                int loadvalue = 0;
                char *cqbverf = NULL;
                char *pqbverf = NULL;
                memset(username, 0, sizeof(username));
                memset(password, 0, sizeof(password));
                if (send(newtm_fd, "Please enter a username: ", 25, 0) == -1)
                    perror("send");
                if (recv(newtm_fd, username, sizeof(username), 0) == -1)
                    perror("recv");
                if (send(newtm_fd, "Please enter a password: ", 25, 0) == -1)
                    perror("send");
                if (recv(newtm_fd, password, sizeof(password), 0) == -1)
                    perror("recv");
                username[strcspn(username, "\n")] = '\0'; // Remove delimiters for string matching to work
                username[strcspn(username, "\r")] = '\0';
                password[strcspn(password, "\n")] = '\0';
                password[strcspn(password, "\r")] = '\0';
                //char *returnvalue;
                printf("USERNAME: %s\n", username);
                printf("PASS: %s\n", password);
                int loginValue = login(username, password, &user);
                printf("File name is: %s\n", user.user_filename);
                if (loginValue == -1) // Invalid Username (doesn't exist)
                {
                    if (send(newtm_fd, "Username Invalid.\n", 19, 0) == -1)
                        perror("send");
                    continue;
                }
                else if (loginValue == 1) // Invalid Password
                {
                    if (send(newtm_fd, "Incorrect Password.\n", 21, 0) == -1)
                        perror("send");
                    continue;
                }

                printf("User signed in!\n");
                printf("User filename is: %s\n", user.user_filename);

                if (loginValue == 0) { // If file does exist
                    printf("File exists.\n");
                    loadvalue = loadUser(&user);
                    printf("loadvalue is: %d\n", loadvalue);
                }
                
                if (loginValue == -2 || loadvalue == -1) // If file failed to open
                {
                    printf("Calling generate new file.\n");
                    generatenewfile(&user);
                    printf("Generated new cookiefile\n");
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
                    loadvalue = loadUser(&user);
                    if (loadvalue == 0)
                        loginValue = 0;
                }

                if (loadvalue == 0 && loginValue == 0) // Everything Works!
                {
                    char code[3] = {0};
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
                    while (1) // Testing QB connection
                    {
                        char indexbuf[4] = {0};
                        int index = 0;
                        char buffer[1024] = {0};
                        memset(code, 0, sizeof(code));
                        if (send(newtm_fd, "Please enter a code: ", 21, 0) == -1)
                            perror("send");
                        if (recv(newtm_fd, code, 3, 0) == -1)
                            perror("recv");
                        if (recv(newtm_fd, buffer, 1024, 0) == -1)
                            perror("recv");
                        code[strcspn(code, "\n")] = '\0';
                        code[strcspn(code, "\r")] = '\0';

                        printf("code is: %s\n", code);

                        if (!strcmp(code, "PQ")) 
                        {
                            printf("inside PQ.\n");
                            if (send(newtm_fd, "Please enter a question index (0-9): ", 37, 0) == -1)
                                perror("send");
                            if (recv(newtm_fd, indexbuf, 4, 0) == -1)
                                perror("recv");
                            printf("index buf is: %s\n", indexbuf);
                            index = atoi(indexbuf);
                            printf("index is: %d\n", index);
                            code[strcspn(code, "\n")] = '\0';
                            code[strcspn(code, "\r")] = '\0';
                            if (!strcasecmp(user.QB[index], "c"))
                            {
                                printf("Sending PQ to c\n");
                                if (write(cqbpipe[1], "PQ", 3) == -1)
                                {
                                    perror("write");
                                }
                                if (write(cqbpipe[1], &index, sizeof(int)) == -1) // Sending question index
                                {
                                    perror("write");
                                }
                                if (write(cqbpipe[1], user.QuestionID[index], 4) == -1) // Sending questionID
                                {
                                    perror("write");
                                }
                                printf("Grabbing question");
                            }
                            else if(!strcasecmp(user.QB[index], "python"))
                            {
                                printf("Sending PQ to p\n");
                                if (write(pqbpipe[1], "PQ", 3) == -1)
                                {
                                    perror("write");
                                }
                                if (write(pqbpipe[1], &index, sizeof(int)) == -1) // Question 2 is index 3
                                {
                                    perror("write");
                                }
                                printf("Question ID is: %s\n", user.QuestionID[index]);
                                if (write(pqbpipe[1], user.QuestionID[index], 4) == -1) // Question 2 is index 3
                                {
                                    perror("write");
                                }
                                printf("Grabbing question");
                            }
                        }
                        else if (!strcasecmp(code, "AN"))
                        {
                            printf("Inside AN\n");
                            if (send(newtm_fd, "Please enter a question index (0-9): ", 37, 0) == -1)
                                perror("send");
                            if (recv(newtm_fd, indexbuf, 4, 0) == -1)
                                perror("recv");
                            printf("index buf is: %s\n", indexbuf);
                            index = atoi(indexbuf);
                            printf("index is: %d\n", index);
                            if (strcasecmp(user.QB[index], "c"))
                            {
                                printf("Sending AN to c\n");
                                if (write(cqbpipe[1], "AN", 3) == -1)
                                {
                                    perror("write");
                                }
                                if (write(cqbpipe[1], user.QuestionID[index], sizeof(int)) == -1)
                                {
                                    perror("write");
                                }
                            }
                            else if(strcasecmp(user.QB[index], "python"))
                            {
                                printf("Sending AN to p\n");
                                if (write(pqbpipe[1], "AN", 3) == -1)
                                {
                                    perror("write");
                                }
                                if (write(pqbpipe[1], user.QuestionID[index], sizeof(int)) == -1)
                                {
                                    perror("write");
                                }
                            }
                        }
                        else if(!strcasecmp(code, "IQ"))
                        {
                            printf("Inside AN\n");
                            if (send(newtm_fd, "Please enter a question index (0-9): ", 37, 0) == -1)
                                perror("send");
                            if (recv(newtm_fd, indexbuf, 4, 0) == -1)
                                perror("recv");
                            if (strcasecmp(user.QB[index], "c"))
                            {
                                printf("Sending IQ to c\n");
                                if (write(cqbpipe[1], "IQ", 3) == -1)
                                {
                                    perror("write");
                                }
                                if (write(cqbpipe[1], user.QuestionID[index], sizeof(int)) == -1)
                                {
                                    perror("write");
                                }
                            }
                            else if(strcasecmp(user.QB[index], "python"))
                            {
                                printf("Sending IQ to p\n");
                                if (write(pqbpipe[1], "IQ", 3) == -1)
                                {
                                    perror("write");
                                }
                                if (write(pqbpipe[1], user.QuestionID[index], sizeof(int)) == -1)
                                {
                                    perror("write");
                                }
                            }
                        }
    
                    }
                    //send(newtm_fd, questionDashboard(), strlen(questionDashboard()), 0);
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