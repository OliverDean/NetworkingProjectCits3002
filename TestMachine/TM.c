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
    char *user_filename;
} curUser;

curUser user;

// Loads user data into structure
// Returns -1 on failure with file (corrupted / not built correctly)
// Returns 0 on success
int loadUser()
{
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
        return -1;
    }

    while ((linelen = getline(&line, &linesize, fp)) != -1)
    {
        buf = strtok(line, ";");
        if (!strcmp(buf, "//"))
        { // Comment line
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

    if (QBcounter == 10) // If there are 10 questions
        return 0;
    else if (10 > QBcounter > 0) // If not 10 questions, error out
        return -1;
    else if (QBcounter <= 0) // If there are 10 or less questions error out
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
void generatenewfile()
{
    char *line = NULL;
    char *buf = NULL;
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
        if (!strcmp(buf, "//"))
        { // If comment line
            continue;
        }
        else if (!strcasecmp(buf, user.username))
        { // Line we want
            buf = strtok(NULL, ";");
            if (!strcmp(buf, user.password))
            {
                buf = strtok(NULL, ";");
                if (buf != NULL && strcmp(buf, "\n"))
                { // If user has filename, re-write over it in the next step
                    printf("Found additional user cookie file: %s\n", buf);
                    counter -= sizeof(buf);
                    remove(buf);
                }
                fseek(new, counter, SEEK_SET);             // Move up file pointer to end of line (after users password)
                fprintf(new, "%s;\n", user.user_filename); // Add users cookie filename
                fprintf(fp, "%s;\n", user.username);       // Add users username to their own cookie file
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
    free(buf);
    fclose(new);
    fclose(up);
    fclose(fp);
    remove("users.txt");
    rename("temptemptemp", "users.txt");
}

// Code for both question banks
// They will share the same code, however pipe[] will be different for both QB's
// Will automatically break out once connection is broken.
void QuestionBanks(int QBsocket, int pipe[2])
{
    while (1)
    {
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
            char filename[9] = {0};
            char *buf = NULL;
            if (send(QBsocket, "GQ", 2, 0) == -1) // Send QB what it needs to prep for
            {
                perror("send");
            }
            if (recv(QBsocket, questionIDbuffer, 32, MSG_WAITALL) == -1) // Wait for random questionID's
            {
                perror("recv");
            }
            if (read(pipe[0], filename, sizeof(filename)) == -1) // Send data to parent
            {
                perror("read");
            }
            user.user_filename = filename;
            if (read(pipe[0], user.username, sizeof(user.username)) == -1) // Send data to parent
            {
                perror("read");
            }
            FILE *ft = fopen(user.user_filename, "a"); // Open file in append mode
            if (ft == NULL)
            {
                perror("fopen");
            }
            buf = strtok(questionIDbuffer, ";"); // Grab the questionID
            for (int i = 0; i < 4; i++)
            {
                fprintf(ft, "q;c;%s;---;\n", buf); // Add it to users cookie file
                buf = strtok(NULL, ";");
            }
            fclose(ft);
        }
        else if (!strcmp(commandbuffer, "AN"))
        { // If QB's need to answer questions
            printf("meant to answer here..\n");
            char *answer = "#include <stdio>\n\nint main(int argv, char *argv[]){\nprintf(\"Hello World!\");\nreturn 0;}";
            int length = sizeof(answer);
            int sendnumber = htonl(length);
            char *isAnswer;
            if (send(QBsocket, "AN", 2, 0) == -1) // Send QB what it needs to prep for
            {
                perror("send");
            }
            if (send(QBsocket, &sendnumber, sizeof(sendnumber), 0) == -1) // Send answer string size to QB
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
            int length = 0;
            char *questionBuf;
            if (send(QBsocket, "PQ", 2, 0) == -1) // Send QB what it needs to prep for
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
            if (recv(QBsocket, questionBuf, ntohl(length), MSG_WAITALL) == -1) // Get answer
            {
                perror("recv");
            }
            printf("%s\n", questionBuf);
        }
    }
    shutdown(QBsocket, SHUT_RDWR);
    close(pipe[0]);
    close(pipe[1]);
}
void displaylogin(int newtm_fd) {
    char buffer[3000];
    int length = 0;
    char *logintext = NULL;
    char *fullhttp = NULL;
    read(newtm_fd, buffer, 3000);
    char *header = "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: "; // Default header
    FILE *loginfile = fopen("./ClientBrowser/login.html", "r"); // Open login.html
    if (loginfile == NULL)
    {
        perror("fopen");
    }
    fseek(loginfile, 0, SEEK_END);
    length = ftell(loginfile); // Grab file length
    fseek(loginfile, 0, SEEK_SET);
    logintext = (char*)malloc((length) * sizeof(char));       // Buffer for file
    fread(logintext, sizeof(char), length, loginfile); // Grab entire file
    int total_length = strlen(header) + strlen(logintext) + (sizeof(char) * 12);
    fullhttp = malloc(sizeof(char) * total_length);
    printf("%s\n", logintext);
    snprintf(fullhttp, total_length, "%s%d\n\n%s", header, length, logintext);
    fclose(loginfile);
    write(newtm_fd, fullhttp, strlen(fullhttp));
    free(logintext);
    free(fullhttp);
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
            QuestionBanks(newc_fd, cqbpipe);
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
            newp_fd = accept(pqb_fd, (struct sockaddr *)&pqb_addr, &pqb_size);
            if (newp_fd == -1)
            {
                perror("accept");
                break;
            }
            printf("accepted connection from PQB\n");
            QuestionBanks(newp_fd, pqbpipe);
            shutdown(newp_fd, SHUT_RDWR);
            close(newp_fd);
            close(pqbpipe[0]);
            close(pqbpipe[1]);
        }
    }

    // Main port is 4125
    while (1)
    {
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
        /*
        char buffer[3000];
        read(newtm_fd, buffer, 3000);
        char *test = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";
        write(newtm_fd, test, strlen(test));
        */
        displaylogin(newtm_fd);
        switch (fork())
        {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
            break;
        case 0:
            while (1)
            {
                close(tm_fd);
                char *returnvalue;
                char httprequestvalue[2];
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

                int loginValue = login(username, password, &user.user_filename);
                if (loginValue == -1) // Invalid Username (doesn't exist)
                {
                    if (send(newtm_fd, "Username Invalid.\n", 19, 0) == -1)
                        perror("send");
                    continue;
                }
                else if (loginValue == -2) // File is broken
                {
                    goto nofile;
                }
                else if (loginValue == 1) // Invalid Password
                {
                    if (send(newtm_fd, "Incorrect Password.\n", 21, 0) == -1)
                        perror("send");
                    continue;
                }

                int loadvalue = loadUser();
                if (loadvalue == -1) // If file failed to open
                {
                    returnvalue = "NF";
                }
                else if (loadvalue == 0 && loginValue == 0) // Everything Works!
                {
                    usersignedin:
                    /*
                    Here is where the HTTP requests will come through once the user is signed in
                    In here all user data is loaded into the structure
                    You will need to send off the 3 byte (2 bytes sent to the python QB cause C has null terminated strings) to the correct QB through the pipes
                    cqbpipe[1] is the input for the C QB, cqbpipe[0] is the output from the C QB
                    pqbpipe[1] is the input for the Python QB, pqbpipe[1] is the output from the Python QB
                    REMEMBER: CQB NOR PQB HAVE THE CORRECT USER STRUCTURE, YOU WILL NEED TO PASS THEM THE REQUIRED INFO.
                    After the user closes the browser make sure the connection is broken (goes through below close() steps)
                    */
                    shutdown(newtm_fd, SHUT_RDWR);
                    close(newtm_fd);
                    close(cqbpipe[1]);
                    close(cqbpipe[0]);
                    close(pqbpipe[0]);
                    close(pqbpipe[1]);
                    break;
                }
                else if (!strcmp(returnvalue, "NF")) // Bad / No File
                {
                nofile:
                    generatenewfile();
                    if (write(cqbpipe[1], "GQ", 3) == -1) // Generate questions from C QB
                    {
                        perror("write");
                    }
                    if (write(cqbpipe[1], user.user_filename, 9) == -1)
                    {
                        perror("write");
                    }
                    if (write(cqbpipe[1], user.username, sizeof(user.username)) == -1)
                    {
                        perror("write");
                    }
                    if (write(pqbpipe[1], "GQ", 3) == -1) // Generate questions from Python QB
                    {
                        perror("write");
                    }
                    if (write(pqbpipe[1], user.user_filename, 9) == -1)
                    {
                        perror("write");
                    }
                    if (write(pqbpipe[1], user.username, sizeof(user.username)) == -1)
                    {
                        perror("write");
                    }
                    goto usersignedin;
                }
            }
            break;
        default:
            shutdown(newtm_fd, SHUT_RDWR);
            close(newtm_fd);
            break;
        }
    }
    shutdown(tm_fd, SHUT_RDWR);
    close(tm_fd);
    return 0;
}