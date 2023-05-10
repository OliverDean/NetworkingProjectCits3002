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

#define PORT "4125"
#define BACKLOG 10

typedef struct curUser {
    char username[32];
    char password[32];
    int userID;
    int QB[10]; // Each question is 0 or 1 (showing what QB from)
    int questions; // Max of 10
    int attempted[10]; // Each has a max of 3
    int score[10]; // Max of 3
    int total_score; // Max of 30
}curUser;

curUser user;

// Loads user data into structure
void loadUser(char* buf) {
    buf = strtok(NULL, ";");
    user.userID = atoi(buf);
    //printf("UserID: %d\n", user.userID);
    for (int i = 0; i < 9; i++) {
        buf = strtok(NULL, " ");
        user.QB[i] = atoi(buf);
        //printf("Question %d from QB %d\n", i+1, user.QB[i]);
    }
    buf = strtok(NULL, ";");
    user.QB[9] = atoi(buf);
    //printf("Question %d from QB %d\n", 10, user.QB[9]);
    buf = strtok(NULL, ";");
    user.questions = atoi(buf);
    //printf("How many questions has user attempted: %d\n", user.questions);
    for (int y = 0; y < 9; y++) {
        buf = strtok(NULL, " ");
        user.attempted[y] = atoi(buf);
        //printf("User has attempted question %d, %d times\n", y+1, user.attempted[y]);
    }
    buf = strtok(NULL, ";");
    user.attempted[9] = atoi(buf);
    //printf("User has attempted question %d, %d times\n", 10, user.attempted[9]);
    for (int x = 0; x < 9; x++) {
        buf = strtok(NULL, " ");
        user.score[x] = atoi(buf);
        //printf("User has scored %d on question %d\n", user.score[x], x+1);
    }
    buf = strtok(NULL, ";");
    user.score[9] = atoi(buf);
    //printf("User has scored %d on question %d\n", user.score[9], 10);
    buf = strtok(NULL, ";");
    user.total_score = atoi(buf);
    //printf("User has a total score of: %d\n", user.total_score);
}

// Logs user in, Returns 0 on success, 1 on password failure and returns 2 on login failure
int login(char username[], char password[]) {

    char* line = NULL;
    char* buf;
    size_t linesize = 0;
    ssize_t linelen;
    int counter = 0;

    FILE *fp = fopen("users.txt", "r");

    while ((linelen = getline(&line, &linesize, fp)) != -1) {
        char temp[32];
        buf = strtok(line, " ");
        if (!strcmp(buf, "//")) // If comment line
            continue;
        if (!strcasecmp(buf, username)) { // If strings equal (ignoring case-sensitive for username)
            strcpy(temp, buf);
            buf = strtok(NULL, " "); 
            if (!strcasecmp(buf, password)) {
                strcpy(user.password, buf);
                strcpy(user.username, temp);
                loadUser(buf);
                return 0;
            }
            else {
                printf("Incorrect Password\n");
                return 1;
            }
        }
        else
            continue;  
    }
    fclose(fp);
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
    char username[32];
    char password[32];
    
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

    while(1) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        //getpeername(new_fd, (struct sockaddr*)&their_addr, sizeof(struct sockaddr));
        //printf("server: got connection from %s\n", s);

        if (!fork()) { // Child Process
            close(sockfd);
            if (send(new_fd, "Please enter a username: ", 25, 0) == -1)
                perror("send");
            if (recv(new_fd, username, 32, 0) == -1)
                perror("recv");
            if (send(new_fd, "Please enter a password: ", 25, 0) == -1)
                perror("send");
            if (recv(new_fd, password, 32, 0) == -1)
                perror("recv");
        }
        close(new_fd);
    }
    
    int temp = login(username, password);

    return 0;
}