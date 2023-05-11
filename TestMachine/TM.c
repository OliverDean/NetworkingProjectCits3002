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

#define PORT "4125"
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

int main(int argc, char *argv[])
{

    char *IP = "192.168.220.118";
    // FTP port is 21
    char username[32] = {0};
    char password[32] = {0};

    /* You will need to add (
     "__linux__",
    "__x86_64__",
    "_GNU_SOURCE"
    ) to c_cpp_properties.json (if using VSC on ubuntu)
    */

    // RUN TM IN ONE CMD WINDOW, IN A SEPERATE WINDOW RUN 'TELNET (REMOTEHOSTNAME) 4125

    int sockfd, new_fd, pqb_fd, cqb_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr, pqb_addr, cqb_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof(hints)); // Make the struct empty
    hints.ai_family = AF_INET;        // Use IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP Stream
    hints.ai_flags = AI_PASSIVE;      // Fill my IP for me

    // Get my local host address info
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all address info and bind to the first one we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {

        // Attempt to create a socket
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        // Allow the kernel to use the same address (gets rid of 'address in use' error)
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        // Associate a port with the above socket
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        printf("Server failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == 1)
    {
        perror("listen");
        exit(1);
    }

    printf("waiting for connections...\n");

    while (1)
    { // main accept() loop
        sin_size = sizeof their_addr;
        int PID = 0;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        char bufferpy[100];
        bool bufferflag = false;
        if (new_fd == -1)
        {
            perror("accept");
            break;
        }
        printf("Accepted connection!\n");

        int thepipe[2];

        if (pipe(thepipe) == -1)
        {
            perror("pipe");
            exit(1);
        }

        if (!fork())
        { // this is the child process
            char *returnvalue;
            //close(sockfd); // child doesn't need the listener
            memset(username, 0, sizeof(username));
            memset(password, 0, sizeof(password));
            if (recv(new_fd, bufferpy, sizeof(bufferpy), 0) == -1)
                perror("recv");
            bufferpy[strcspn(bufferpy, "\n")] = '\0';
            bufferpy[strcspn(bufferpy, "\r")] = '\0';
            if (!strcasecmp(bufferpy, "PQB"))
            { // Allow us to determine if connection is PQB or from clients
                char buffer[1024];
                printf("From PQB\n");
                pqb_fd = new_fd;
                pqb_addr = their_addr;
                bufferflag = true;
                read(thepipe[0], buffer, sizeof(buffer));
                exit(0);
            }
            else if (!strcasecmp(bufferpy, "CQB"))
            { // Allow us to determine if connection is CQB or from clients
                printf("From CQB\n");
                cqb_fd = new_fd;
                pqb_addr = their_addr;
                bufferflag = true;
                exit(0);
            }
            else if (!strcasecmp(bufferpy, "TM"))
            {
                printf("sending data\n");
                if (send(new_fd, "Please enter a username: ", 25, 0) == -1)
                    perror("send");
                printf("Waiting for data.\n");
                if (recv(new_fd, username, sizeof(username), 0) == -1)
                    perror("recv");
                if (send(new_fd, "Please enter a password: ", 25, 0) == -1)
                    perror("send");
                if (recv(new_fd, password, sizeof(password), 0) == -1)
                    perror("recv");
                username[strcspn(username, "\n")] = '\0';
                username[strcspn(username, "\r")] = '\0';
                password[strcspn(password, "\n")] = '\0';
                password[strcspn(password, "\r")] = '\0';

                int loginValue = login(username, password, &user.user_filename);
                if (loginValue == -1) // Inavlid Username (doesn't exist)
                {
                    if (send(new_fd, "Username Invalid.\n", 19, 0) == -1)
                        perror("send");
                    returnvalue = "IL";
                    close(new_fd);
                    exit(0);
                }
                else if (loginValue == -2) // File is broken
                {
                    returnvalue = "NF";
                    close(thepipe[1]);
                }
                else if (loginValue == 1) // Invalid Password
                {
                    if (send(new_fd, "Incorrect Password.\n", 21, 0) == -1)
                        perror("send");
                    returnvalue = "IL";
                    close(new_fd);
                    exit(0);
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
                close(thepipe[1]);
            }
            else
            {
                printf("Incorrect prefix supplied!\n");
                bufferflag = true;
                exit(0);
            }

            if (!strcmp(returnvalue, "YE")) // Success
            {
                close(new_fd);
                printf("Closing connection with client...\n");
                continue;
            }
            else if (!strcmp(returnvalue, "IL")) // Incorrect Login
            {
                printf("Incorrect Login.");
                close(new_fd);
                continue;
            }
            else if (!strcmp(returnvalue, "NF")) // Bad / No File
            {
                printf("NF tag\n");
                printf("Helping user: %s\n", user.username);
                char *randomstring = randomStringGenerator();
                user.user_filename = randomstring;
                FILE *fp = fopen(randomstring, "w");
                FILE *up = fopen("users.txt", "r+");
                if (fp == NULL)
                {
                    perror("fopen");
                    close(thepipe[1]);
                    exit(0);
                }
                if (up == NULL)
                {
                    perror("fopen");
                    close(thepipe[1]);
                    exit(0);
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
                            counter = counter - strlen(buf);
                        }
                        printf("Username matched\n");
                        printf("Went over %d characters\n", counter);
                        fseek(up, counter, SEEK_SET);
                        printf("Moved file pointer...\n");
                        fprintf(up, "%s;", user.user_filename);
                    }
                    else
                        continue;
                }
            }
        }
        close(new_fd);
    }
    return 0;
}