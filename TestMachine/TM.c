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

typedef struct curUser {
    char username[32];
    char password[32];
    char *QB[10]; // Each question is c or python, indicating where its 
    char *QuestionID[10]; // Indicating what the questionID is
    int attempted; // Max of 10
    int questions[10]; // Each has a max of 3
    int score[10]; // Max of 3
    int total_score; // Max of 30
}curUser;

curUser user;

// Loads user data into structure
int loadUser(char* filename) {

    char * line = NULL;
    char *buf;
    size_t linesize = 0;
    ssize_t linelen;
    int QBcounter = 0;
    memset(user.questions, 0, sizeof(user.questions)); // make array all 0
    user.total_score = 0;
    user.attempted = 0;
    
    FILE *fp = fopen(filename, "r"); // Open user file
    if (fp == NULL) {
        perror("fopen");
        return -1;
    }

    while ((linelen = getline(&line, &linesize, fp)) != -1) { 
        buf = strtok(line, ";");
        if (!strcmp(buf, "//")) // Comment line
            continue;
        else if ((strcasecmp(buf, user.username)) && (strcasecmp(buf, "q"))) { // If username in file doesn't match signed in user
            printf("Incorrect username in file!\nUsername in file: %s\nUsername given:%s\n", buf, user.username);
            return -1;
        }
        else if (!strcmp(buf, "q")) { // question line
            buf = strtok(NULL, ";"); // QB its from
            user.QB[QBcounter] = buf;
            buf = strtok(NULL, ";"); // QuestionID
            user.QuestionID[QBcounter] = buf;
            buf = strtok(NULL, ";"); // Attempted marks

            for (buf; *buf != '\0'; buf++) { // For the marks string
                if (*buf == 'N') { // If user has answered incorrectly
                    if (user.questions[QBcounter] == 0)
                        user.attempted++;
                    user.questions[QBcounter]++;
                }
                else if (*buf == 'Y') { // If user has answered correctly
                    user.score[QBcounter] = 3 - user.questions[QBcounter];
                    user.total_score += user.score[QBcounter];
                    if (user.questions[QBcounter] == 0) {
                        user.attempted++;
                    }
                    break;
                }
                else if (*buf == '-') {
                    break;
                }
            }

            QBcounter++;
        }
    }
    printf("User %s has attempted %d questions, resulting in a total score of: %d\n", user.username, user.attempted, user.total_score);
}

// Logs user in, Returns 0 on success, 1 on password failure and returns 2 on login failure
int login(char username[], char password[]) {

    char temp[32];
    char* line = NULL;
    char* buf;
    size_t linesize = 0;
    ssize_t linelen;
    int counter = 0;

    FILE *fp = fopen("users.txt", "r");
    printf("Opening users text file\n");

    while ((linelen = getline(&line, &linesize, fp)) != -1) {
        buf = strtok(line, " ");
        if (!strcmp(buf, "//")) // If comment line
            continue;
        if (!strcasecmp(buf, username)) { // If strings equal (ignoring case-sensitive for username)
            printf("Usernames match\n");
            strcpy(temp, buf);
            buf = strtok(NULL, " "); 
            if (!strcmp(buf, password)) { // If passwords equal (must be case-sensitive)
                strcpy(user.password, buf);
                strcpy(user.username, temp);
                printf("Signed in!!!\n");
                buf = strtok(NULL, ";");
                printf("Filename is %s\n", buf);
                fclose(fp);
                if (!loadUser(buf))
                    return 0;
                else
                    return -1;
            }
            else {
                printf("Incorrect Password\n");
                return 1;
            }
        }
        else
            continue; 
    }
    return 2;
}

void handle_sigchld(int sig) {
  int saved_errno = errno;
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
  errno = saved_errno;
}

int main(int argc, char* argv[]) {

    char *IP = "192.168.220.118";
    //FTP port is 21
    char username[32] = {0};
    char password[32] = {0};
    
    /* You will need to add (
     "__linux__",
    "__x86_64__",
    "_GNU_SOURCE" 
    ) to c_cpp_properties.json (if using VSC on ubuntu)
    */

   // RUN TM IN ONE CMD WINDOW, IN A SEPERATE WINDOW RUN 'TELNET (REMOTEHOSTNAME) 4125

    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof(hints)); // Make the struct empty
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP Stream
    hints.ai_flags = AI_PASSIVE; // Fill my IP for me

    // Get my local host address info
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all address info and bind to the first one we can
    for(p = servinfo; p != NULL; p = p->ai_next) {

        // Attempt to create a socket
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        // Allow the kernel to use the same address (gets rid of 'address in use' error)
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        // Associate a port with the above socket
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        printf("Server failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == 1) {
        perror("listen");
        exit(1);
    }

    printf("waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        int PID = 0;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        char bufferpy[100];
        bool bufferflag = false;
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        printf("Accepted connection!\n");

        int thepipe[2];

        if (pipe(thepipe) == -1) {
            perror("pipe");
            exit(1);
        }

        if (!fork()) { // this is the child process
            //char buffer[32] = {0};
            close(thepipe[0]); // Child never needs to
            close(sockfd); // child doesn't need the listener
            memset(username,0,sizeof(username));
            memset(password,0,sizeof(password));
            if (recv(new_fd, bufferpy, sizeof(bufferpy), 0) == -1)
                perror("recv");
            bufferpy[strcspn(bufferpy, "\n")] = '\0';
            bufferpy[strcspn(bufferpy, "\r")] = '\0';
            if (!strcasecmp(bufferpy, "QB")) { // Allow us to determine if connection is QB or from clients
                printf("From QB\n");
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
                write(thepipe[1], username, sizeof(username));
                if (send(new_fd, "Please enter a password: ", 25, 0) == -1)
                    perror("send");
                if (recv(new_fd, password, sizeof(password), 0) == -1)
                    perror("recv");
                write(thepipe[1], password, sizeof(password));
                close(thepipe[1]);
                exit(0);
            }
            else {
                printf("Incorrect prefix supplied!\n");
                bufferflag = true;
                exit(0);
            }
        }
        if (bufferflag)
            continue;
        close(thepipe[1]);
        read(thepipe[0], username, sizeof(username));
        read(thepipe[0], password, sizeof(password));
        close(thepipe[0]);
        username[strcspn(username, "\n")] = '\0';
        username[strcspn(username, "\r")] = '\0';
        password[strcspn(password, "\n")] = '\0';
        password[strcspn(password, "\r")] = '\0';
        close(new_fd);  // parent doesn't need this
        int temp = login(username, password);

        //break;
    }

    int temp = login(username, password);

    return 0;
}