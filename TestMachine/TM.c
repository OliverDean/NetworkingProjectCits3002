#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>

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
    char *user_filename;
} curUser;

curUser user;

// Loads user data into structure
// Returns -1 on failure with file
// Returns -2 if file is corrupted (or not properly built)
// Returns 0 on success
int loadUser(char *filename)
{

    char *line = NULL;
    char *buf;
    size_t linesize = 0;
    ssize_t linelen;
    int QBcounter = 0;
    memset(user.questions, 0, sizeof(user.questions)); // make array all 0
    user.total_score = 0;
    user.attempted = 0;

    FILE *fp = fopen(filename, "r"); // Open user file
    if (fp == NULL)
    {
        perror("fopen");
        return -1;
    }

    while ((linelen = getline(&line, &linesize, fp)) != -1)
    {
        buf = strtok(line, ";");
        if (!strcmp(buf, "//")) // Comment line
            continue;
        else if ((strcasecmp(buf, user.username)) && (strcasecmp(buf, "q")))
        { // If username in file doesn't match signed in user
            printf("Incorrect username in file!\nUsername in file: %s\nUsername given:%s\n", buf, user.username);
            return -2;
        }
        else if (!strcmp(buf, "q"))
        {                            // question line
            buf = strtok(NULL, ";"); // QB its from
            user.QB[QBcounter] = buf;
            buf = strtok(NULL, ";"); // QuestionID
            user.QuestionID[QBcounter] = buf;
            buf = strtok(NULL, ";"); // Attempted marks

            for (buf; *buf != '\0'; buf++)
            { // For the marks string
                if (*buf == 'N')
                { // If user has answered incorrectly
                    if (user.questions[QBcounter] == 0)
                        user.attempted++;
                    user.questions[QBcounter]++;
                }
                else if (*buf == 'Y')
                { // If user has answered correctly
                    user.score[QBcounter] = 3 - user.questions[QBcounter];
                    user.total_score += user.score[QBcounter];
                    if (user.questions[QBcounter] == 0)
                    {
                        user.attempted++;
                    }
                    break;
                }
                else if (*buf == '-')
                    break;
            }

            QBcounter++;
        }
    }

    if (QBcounter == 10)
        return 0;
    else if (10 > QBcounter > 0)
        return -2;
    else if (QBcounter <= 0)
        return -1;
}

char *randomStringGenerator()
{
    static char stringSet[] = "abcdefghijklmnopqrstuvwxyz1234567890";
    char *randomstring = malloc(sizeof(char *) * 8 + 1);

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
int login(char username[], char password[], char **filename)
{

    char temp[32];
    char *line = NULL;
    char *buf;
    size_t linesize = 0;
    ssize_t linelen;
    int counter = 0;
    printf("password:%s\n", password);
    printf("Username: %s\n", username);

    FILE *fpa = fopen("users.txt", "r");
    printf("Opening users text file\n");

    while ((linelen = getline(&line, &linesize, fpa)) != -1)
    {
        buf = strtok(line, ";");
        if (!strcmp(buf, "//")) // If comment line
            continue;
        printf("Checking username...:%s\n", buf);
        if (!strcasecmp(buf, username))
        { // If strings equal (ignoring case-sensitive for username)
            printf("Usernames match\n");
            strcpy(temp, buf);
            buf = strtok(NULL, ";x");
            if (!strcmp(buf, password))
            { // If passwords equal (must be case-sensitive)
                strcpy(user.password, buf);
                strcpy(user.username, temp);
                printf("Signed in!!!\n");
                buf = strtok(NULL, ";");
                if (buf == NULL)
                {
                    printf("User doesn't have cookiefile!\n");
                    return -2;
                }
                char *temp = malloc(1024);
                strcpy(temp, buf);
                *filename = temp;
                fclose(fpa);
                return 0;
            }
            else
            {
                printf("Incorrect Password\n");
                return 1;
            }
        }
        else
            continue;
    }
    return -1;
}

void handle_sigchld(int sig)
{
    int saved_errno = errno;
    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0)
    {
    }
    errno = saved_errno;
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

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
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
        if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
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

void fixbrokenfile()
{
    printf("NF tag\n");
    printf("Helping user: %s\n", user.username);
    char *randomstring = randomStringGenerator();
    user.user_filename = randomstring;
    FILE *fp = fopen(user.user_filename, "w");
    FILE *up = fopen("users.txt", "r+");
    if (fp == NULL)
    {
        perror("fopen");
    }
    if (up == NULL)
    {
        perror("fopen");
    }
    char *line = NULL;
    char *buf;
    size_t linesize = 0;
    ssize_t linelen;
    int counter = 0;
    printf("Grabbing line...\n");
    while ((linelen = getline(&line, &linesize, up)) != -1)
    {
        printf("Current line is: %s\n", line);
        counter += (int)linelen;
        buf = strtok(line, ";");
        if (!strcmp(buf, "//")) // If comment line
            continue;
        else if (!strcasecmp(buf, user.username))
        { // Line we want
            buf = strtok(NULL, ";");
            buf = strtok(NULL, ";");
            if (buf != NULL)
            { // If user has filename
                remove(buf);
                counter = counter - strlen(buf);
            }
            printf("Username matched\n");
            printf("Went over %d characters\n", counter);
            fseek(up, counter, SEEK_SET);
            printf("Moved file pointer...\n");
            fprintf(up, "%s;", user.user_filename);
            if (fp == NULL)
            {
                fp = fopen(user.user_filename, "w");
            }
            fprintf(fp, "%s", "test");
            fclose(fp);
            fclose(up);
        }
        else
            continue;
    }
}

int main(int argc, char *argv[])
{
    char username[32] = {0};
    char password[32] = {0};

    socklen_t cqb_size, pqb_size, tm_size;
    struct addrinfo cqb_addr, pqb_addr, tm_addr;
    int cqb_fd, pqb_fd, tm_fd;
    int newc_fd, newp_fd, newtm_fd;
    int cqbpipe[2], pqbpipe[2], tmpipe[2];

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
    if (pipe(tmpipe) == -1)
    {
        perror("pipe");
        exit(1);
    }

    cqb_fd = setupsocket(CQBPORT);
    printf("CQB socket is: %d\n", cqb_fd);
    pqb_fd = setupsocket(PQBPORT);
    printf("PQB socket is: %d\n", pqb_fd);
    tm_fd = setupsocket(TMPORT);
    printf("TM socket is: %d\n", tm_fd);
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
    printf("waiting for connections...\n");

    // This is the C Question Bank loop
    if (!fork())
    { // child process
        printf("Listening for CQB..\n");
        while (1)
        { // main accept() loop
            cqb_size = sizeof cqb_addr;
            newc_fd = accept(cqb_fd, (struct sockaddr *)&cqb_addr, &cqb_size);
            if (newc_fd == -1)
            {
                perror("accept");
                break;
            }
            printf("Accepted connection from CQB!\n");

            if (!fork())
            { // this is the child process
                //close(cqb_fd);
                char buffer[1024];
                printf("From CQB\n");
                if(send(newc_fd, "Hi CQB!\n", 9, 0) == -1)
                {
                    perror("send");
                    continue;
                }
                read(cqbpipe[0], buffer, sizeof(buffer));
                close(cqbpipe[0]);
                close(cqbpipe[1]);
                close(newc_fd);
                exit(0);
            }
            close(newc_fd);
            close(cqbpipe[0]);
            close(cqbpipe[1]);
        }
    }

    // This is the Python Question Bank loop
    if (!fork())
    { // child process
        printf("Listening for PQB...\n");
        while (1)
        { // main accept() loop
            pqb_size = sizeof pqb_addr;
            newp_fd = accept(pqb_fd, (struct sockaddr *)&pqb_addr, &pqb_size);
            if (newp_fd == -1)
            {
                perror("accept");
                break;
            }
            printf("Accepted connection from PQB!\n");

            if (!fork())
            { // this is the child process
                close(pqb_fd);
                char buffer[1024];
                printf("From PQB\n");
                if(send(newp_fd, "Hi PQB!\n", 9, 0) == -1)
                {
                    perror("send");
                    continue;
                }
                read(pqbpipe[0], buffer, sizeof(buffer));
                close(pqbpipe[0]);
                close(pqbpipe[1]);
                close(newp_fd);
                exit(0);
            }
            close(newp_fd);
        }
    }

    while (1)
    { // main accept() loop
        printf("Listening for TM...\n");
        tm_size = sizeof tm_addr;
        int PID = 0;
        newtm_fd = accept(tm_fd, (struct sockaddr *)&tm_addr, &tm_size);
        char bufferpy[100];
        bool bufferflag = false;
        if (newtm_fd == -1)
        {
            perror("accept");
            break;
        }
        printf("Accepted connection!\n");

        if (!fork())
        { // this is the child process
            char *returnvalue;
            //close(tm_fd);
            memset(username, 0, sizeof(username));
            memset(password, 0, sizeof(password));
            printf("sending data\n");
            if (send(newtm_fd, "Please enter a username: ", 25, 0) == -1)
                perror("send");
            printf("Waiting for data.\n");
            if (recv(newtm_fd, username, sizeof(username), 0) == -1)
                perror("recv");
            if (send(newtm_fd, "Please enter a password: ", 25, 0) == -1)
                perror("send");
            if (recv(newtm_fd, password, sizeof(password), 0) == -1)
                perror("recv");
            username[strcspn(username, "\n")] = '\0';
            username[strcspn(username, "\r")] = '\0';
            password[strcspn(password, "\n")] = '\0';
            password[strcspn(password, "\r")] = '\0';

            int loginValue = login(username, password, &user.user_filename);
            if (loginValue == -1) // Inavlid Username (doesn't exist)
            {
                if (send(newtm_fd, "Username Invalid.\n", 19, 0) == -1)
                    perror("send");
                returnvalue = "IL";
                close(newtm_fd);
            }
            else if (loginValue == -2) // File is broken
            {
                returnvalue = "NF";
                close(tmpipe[1]);
            }
            else if (loginValue == 1) // Invalid Password
            {
                if (send(newtm_fd, "Incorrect Password.\n", 21, 0) == -1)
                    perror("send");
                returnvalue = "IL";
                close(newtm_fd);
            }
            printf("Opening user file..\n");
            int loadvalue = loadUser(user.user_filename);
            if (loadvalue == -1) // If file failed to open
            {
                returnvalue = "NF";
            }
            else if (loadvalue == -2) // If file is corrupted
            {
                returnvalue = "NF";
            }
            if (loadvalue == 0 && loginValue == 0)
            {
                returnvalue = "YE";
            }
            close(tmpipe[1]);

            if (!strcmp(returnvalue, "YE")) // Success
            {
                close(newtm_fd);
                printf("Closing connection with client...\n");
                continue;
            }
            else if (!strcmp(returnvalue, "IL")) // Incorrect Login
            {
                printf("Incorrect Login.");
                close(newtm_fd);
                continue;
            }
            else if (!strcmp(returnvalue, "NF")) // Bad / No File
            {
                fixbrokenfile();
            }
        }
        close(newtm_fd);
    }
    return 0;
}