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
#include <time.h>

#define TMPORT "4125"
#define PQBPORT "4126"
#define CQBPORT "4127"
#define BACKLOG 2

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
// Returns -1 on failure with file (corrupted / not built correctly)
// Returns 0 on success
int loadUser()
{
    bool flag = false;
    char *line = NULL;
    char *buf;
    size_t linesize = 0;
    ssize_t linelen;
    int QBcounter = 0;
    memset(user.questions, 0, sizeof(user.questions)); // make array all 0
    user.total_score = 0;
    user.attempted = 0;

    FILE *fp;
    fp = fopen(user.user_filename, "r"); // Open user file
    if (fp == NULL)
    {
        fp = fopen(user.user_filename, "w+");
    }

    while ((linelen = getline(&line, &linesize, fp)) != -1)
    {
        buf = strtok(line, ";");
        if (!strcmp(buf, "//")) { // Comment line
            continue;
        }
        buf[strcspn(buf, "\n")] = '\0';
        buf[strcspn(buf, "\r")] = '\0';
        if ((strcasecmp(buf, user.username)) && (strcmp(buf, "q")))
        { // If username in file doesn't match signed in user
            printf("Incorrect username in file!\nUsername in file: %sUsername given:%s\n", buf, user.username);
            return -1;
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
    fclose(fp);

    if (QBcounter == 10)
        return 0;
    else if (10 > QBcounter > 0)
        return -1;
    else if (QBcounter <= 0)
        return -1;
}

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
int login(char username[], char password[], char **filename)
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
                strcpy(user.password, buf);
                strcpy(user.username, temp);
                buf = strtok(NULL, ";");
                if (buf == NULL)
                {
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

// Generates new cookie file for user, adds it
void generatenewfile()
{
    char *line = NULL;
    char *buf;
    size_t linesize = 0;
    ssize_t linelen;
    int counter = -1;
    char *randomstring = randomStringGenerator(); // Generate new user cookie
    user.user_filename = randomstring;
    FILE *fp = fopen(user.user_filename, "w"); // Create new user cookie file
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
        if (!strcmp(buf, "//")) { // If comment line
            continue;
        }
        else if (!strcasecmp(buf, user.username))
        { // Line we want
            buf = strtok(NULL, ";");
            if (!strcmp(buf, user.password))
            {
                buf = strtok(NULL, ";");
                if (buf && !buf[0])
                { // If user has filename, re-write over it in the next step
                    counter -= sizeof(buf);
                    remove(buf);
                }
                fseek(new, counter, SEEK_SET);           // Move up file pointer to end of line (after users password)
                fprintf(new, "%s;\n", user.user_filename); // Add users cookie filename
                fprintf(fp, "%s;\n", user.username);     // Add users username to their own cookie file
            }
            else {
                continue;
            }
        }
        else
            continue;      
    }
    free(line);
    fclose(new);
    remove("users.txt");
    rename("temptemptemp", "users.txt");
    fclose(up);
}


char* loginPage()
{
    char *returnString;
    FILE *fp;
    fp = fopen("/Users/karla/NetworkingProjectCits3002/ClientBrowser/login.html", "r");
    if (fp == NULL) {perror("html file");}
    char header[5000] = "HTTP/1.1 200 OK\r\n\n";
    char line[180];
    while (fgets(line, sizeof(line), fp))
    {
        returnString = strcat(header, line);
    };
    return returnString;
}

char *questionDashboard()
{
    char *returnString;
    FILE *fp;
    fp = fopen("/Users/karla/NetworkingProjectCits3002/ClientBrowser/question_dashboard.html", "r");
    if (fp == NULL) {perror("html file");}
    char header[5000] = "HTTP/1.1 404 OK\r\n Set-Cookie: myCookie= karlawashere\r\n\n";
    char line[180];
    while (fgets(line, sizeof(line), fp))
    {
        returnString = strcat(header, line);
    };
    return returnString;

}

void setUser(char *buffer, char username[32], char password[32])
{
        char *pch;
        pch = strtok(buffer, " ? = &");
        char *previous = pch;
        while (pch != NULL) 
        {

            pch=strtok(NULL, " ? = &");
            if (strcmp(previous, "username") == 0)
            {
                strcpy(username, pch);
            }

            if (strcmp(previous, "password") == 0)
            {
                strcpy(password, pch);
                break;
            }
            previous = pch;
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
    printf("waiting for connections...\n");

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
            while (1)
            { // main accept() loop
                char commandbuffer[3] = {0};
                char questionIDbuffer[9] = {0};
                char filename[9] = {0};
                char *buf;
                char GQ[strlen("GQ")];
                memcpy(GQ, "GQ", strlen(GQ));
                FILE *ft;
                read(cqbpipe[0], commandbuffer, 3);
                if (!strcmp(commandbuffer, "GQ")) // Generate Questions
                {
                    if (send(newc_fd, "GQ", 3, 0) == -1) {
                        perror("send");
                        break;
                    }
                    memset(commandbuffer, 0, sizeof(commandbuffer));
                    sleep(0.01);
                    if (recv(newc_fd, questionIDbuffer, sizeof(questionIDbuffer), 0) == -1) {
                        perror("recv");
                        break;
                    }
                    questionIDbuffer[8] = '\0';
                    read(cqbpipe[0], filename, sizeof(filename));
                    user.user_filename = filename;
                    read(cqbpipe[0], user.username, sizeof(user.username));
                    ft = fopen(user.user_filename, "w");
                    if (ft == NULL)
                    {
                        perror("fopen");
                    }
                    buf = strtok(questionIDbuffer, ";");
                    for (int i = 0; i < 4; i++) {
                        fprintf(ft, "q;c;%s;---;\n", buf);
                        buf = strtok(NULL, ";");
                    }
                    write(cqbpipe[1], "YE", 2);
                }
            }
            close(newc_fd);
            close(cqbpipe[0]);
            close(cqbpipe[1]);
            close(tmpipe[1]);
            close(tmpipe[0]);
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
            newp_fd = accept(pqb_fd, (struct sockaddr *)&pqb_addr, &pqb_size);
            if (newp_fd == -1)
            {
                perror("accept");
                break;
            }
            printf("accepted connection from PQB\n");
            while (1)
            { // main accept() loop
                char commandbuffer[3] = {0};
                char questionIDbuffer[13] = {0};
                char filename[9] = {0};
                char GQ[strlen("GQ")];
                memcpy(GQ, "GQ", strlen(GQ));
                char *buf;
                FILE *ft;
                read(pqbpipe[0], commandbuffer, 3);
                if (!strcmp(commandbuffer, "GQ")) // Generate Questions
                {
                    printf("sending gq\n");
                    if (send(newp_fd, "GQ", 3, 0) == -1) {
                        perror("send");
                        break;
                    }
                    printf("waiting for words\n");
                    memset(commandbuffer, 0, sizeof(commandbuffer));
                    sleep(0.01);
                    if (recv(newp_fd, questionIDbuffer, sizeof(questionIDbuffer), 0) == -1) {
                        perror("recv");
                        break;
                    }
                    questionIDbuffer[12] = '\0';
                    read(pqbpipe[0], filename, sizeof(filename));
                    user.user_filename = filename;
                    read(pqbpipe[0], user.username, sizeof(user.username));
                    ft = fopen(user.user_filename, "a");
                    if (ft == NULL)
                    {
                        perror("fopen");
                    }
                    buf = strtok(questionIDbuffer, ";");
                    for (int i = 0; i < 6; i++) {
                        fprintf(ft, "q;python;%s;---;\n", buf);
                        buf = strtok(NULL, ";");
                    }
                }
            }
            close(newp_fd);
            close(pqbpipe[0]);
            close(pqbpipe[1]);
            close(tmpipe[1]);
            close(tmpipe[0]);
        }
    }

    while (1)
    {// Main port is 4125
        memset(&user, 0, sizeof(user)); // Make sure user structure is empty
        tm_size = sizeof tm_addr;
        FILE *fp;
        char acceptchar[2];  
        newtm_fd = accept(tm_fd, (struct sockaddr *)&tm_addr, &tm_size);
        if (newtm_fd == -1)
        {
            perror("accept");
            break;
        }

        if (!fork())
        { // this is the child process
            char *returnvalue;
            memset(username, 0, sizeof(username));
            memset(password, 0, sizeof(password));

            username[strcspn(username, "\n")] = '\0'; // Remove delimiters for string matching to work
            username[strcspn(username, "\r")] = '\0';
            password[strcspn(password, "\n")] = '\0';
            password[strcspn(password, "\r")] = '\0';

            char buffer[2500];
            recv(newtm_fd, buffer, sizeof(buffer),0);
            printf("buffer received from browser: %s\n", buffer);
            send(newtm_fd, loginPage(), strlen(loginPage()), 0);
            setUser(buffer, username, password);


            int loginValue = login(username, password, &user.user_filename);
            if (loginValue == -1) // Invalid Username (doesn't exist)
            {
                printf("Username invalid\n");
                goto invalidsignin;
            }
            else if (loginValue == -2) // File is broken
            {
                goto nofile;
            }
            else if (loginValue == 1) // Invalid Password
            {
                printf("Password is wrong\n");
                goto invalidsignin;
            }

            int loadvalue = loadUser();
            if (loadvalue == -1) // If file failed to open
            {
                returnvalue = "NF";
            }
            else if (loadvalue == 0 && loginValue == 0)
            {
                returnvalue = "YE";
            }

            if (!strcmp(returnvalue, "YE")) // Success (login and filename is correct + loaded)
            {
                printf("success here\n");

                send(newtm_fd, questionDashboard(), strlen(questionDashboard()), 0);

                close(newtm_fd);
                close(tmpipe[0]);
                close(tmpipe[1]);
                continue;
            }
            else if (!strcmp(returnvalue, "IL")) // Incorrect Login
            {
            invalidsignin:
                close(newtm_fd);
                close(tmpipe[0]);
                close(tmpipe[1]);
                continue;
            }
            else if (!strcmp(returnvalue, "NF")) // Bad / No File
            {
            nofile:
                generatenewfile();
                write(cqbpipe[1], "GQ", 3);
                write(cqbpipe[1], user.user_filename, 9);
                write(cqbpipe[1], user.username, sizeof(user.username));
                recv(cqbpipe[0], acceptchar, sizeof(acceptchar), 0);
                write(pqbpipe[1], "GQ", 3);
                write(pqbpipe[1], user.user_filename, 9);
                write(pqbpipe[1], user.username, sizeof(user.username));
                close(newtm_fd);
                close(tmpipe[0]);
                close(cqbpipe[1]);
                close(cqbpipe[0]);
                close(pqbpipe[1]);
                close(pqbpipe[0]);
                close(pqbpipe[1]);
            }
        }
        close(newtm_fd);
        close(cqbpipe[0]);
        close(cqbpipe[1]);
        close(pqbpipe[0]);
        close(pqbpipe[1]);
        close(tmpipe[1]);
        close(tmpipe[0]);
    }
    return 0;
}