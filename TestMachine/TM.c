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
    int counter = 0;
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
            if (read(pipe[0], filename, 9) == -1) // Send data to parent
            {
                perror("read");
            }
            printf("User file name is: %s\n", filename);
            user.user_filename = filename;
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
            if (write(pipe[1], "YE", 3) == -1)
            {
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
            char *questionBuf = NULL;
            char *optionBuf = NULL;
            if (read(pipe[0], &questionID_index, sizeof(int)) == -1) 
            {
                perror("read");
            }
            printf("id is %d\n", questionID_index);
            if (send(QBsocket, "PQ", 2, 0) == -1) // Send QB what it needs to prep for
            {
                perror("send");
            }
            printf("Send PQ\n");
            if (send(QBsocket, user.QuestionID[questionID_index], sizeof(int), 0) == -1) // Send QB the questionID
            {
                perror("send");
            }
            printf("Got user ID/\n");
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

void setUser(char *buffer, char username[32], char password[32])
{
        char *pch;
        pch = strtok(buffer, "?=&" );
        char *previous = pch;
        while (pch != NULL) 
        {

            pch=strtok(NULL, " ? = & ");
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
        // int count = 0;
        // for (int i =0; i < strlen(password); i++)
        // {
        //     if (password[i] != '\n' || password[i] != '\r' || password[i] != '\0')
        //     {
        //         printf("\t%c\n", password[i]);
        //     }
        // }
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

        memset(username, 0, sizeof(username));
        memset(password, 0, sizeof(password));

        //printf("1USERNAME: %s\n", username);
        //printf("1PASSWORD: %s\n", password);
        /*
        displaylogin(newtm_fd);
        char buffer[2500];
        recv(newtm_fd, buffer, sizeof(buffer),0);
        setUser(buffer, username, password);
        username[strcspn(username, "\n")] = '\0'; // Remove delimiters for string matching to work
        username[strcspn(username, "\r")] = '\0';
        password[strcspn(password, "\n")] = '\0';
        password[strcspn(password, "\r")] = '\0';
        */
        //printf("2USERNAME: %s\n Length of USERNAME: %lu\n", username, strlen(username));
        //printf("2PASSWORD: %s\n Length of PASSWORD: %lu\n", password, strlen(password));

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
                //char httprequestvalue[2];
                printf("USERNAME: %s\n", username);
                printf("PASS: %s\n", password);
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
                printf("load value: %i\n", loadvalue);
                if (loadvalue == -1) // If file failed to open
                {
                    goto nofile;
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
                    printf("successful signin.\n");
                    char *cqbconf = NULL;
                    char *pqbconf = NULL;
                    printf("Waiting for confirmation.\n");
                    if (loadvalue != 0 && loginValue != 0)
                    {
                        while (strcasecmp(cqbconf, "YE"))
                        {
                            if (read(cqbpipe[0], cqbconf, 3) == -1)
                            {
                                perror("read");
                            }
                        }
                        printf("Confirmation from CQB\n");
                        while (strcasecmp(pqbconf, "YE"))
                        {
                            if (read(pqbpipe[0], pqbconf, 3) == -1)
                            {
                                perror("read");
                            }
                        }
                    }
                    printf("Files made, conf is: %s & %s", cqbconf, pqbconf);
                    int index = 3;
                    if (strcasecmp(user.QB[2], "c"))
                    {
                        printf("Sending PQ to c\n");
                        if (write(cqbpipe[1], "PQ", 3) == -1)
                        {
                            perror("write");
                        }
                        if (write(cqbpipe[1], &index, sizeof(int)) == -1) // Question 2 is index 3
                        {
                            perror("write");
                        }
                        printf("Grabbing question");
                    }
                    else if(strcasecmp(user.QB[2], "python"))
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
                        printf("Grabbing question");
                    }
                    //send(newtm_fd, questionDashboard(), strlen(questionDashboard()), 0);
                    close(newtm_fd);
                    close(cqbpipe[1]);
                    close(cqbpipe[0]);
                    close(pqbpipe[0]);
                    close(pqbpipe[1]);
                    break;
                }
                else if (loadvalue != 0 || loginValue != 0) // Bad / No File
                {
                nofile:
                    generatenewfile();
                    printf("Generated new cookiefile\n");
                    printf("sending gq request\n");
                    if (write(cqbpipe[1], "GQ", 3) == -1) // Generate questions from C QB
                    {
                        perror("write");
                    }
                    if (write(cqbpipe[1], user.user_filename, 9) == -1)
                    {
                        perror("write");
                    }
                    printf("C QB success.\n");
                    if (write(pqbpipe[1], "GQ", 3) == -1) // Generate questions from Python QB
                    {
                        perror("write");
                    }
                    if (write(pqbpipe[1], user.user_filename, 9) == -1)
                    {
                        perror("write");
                    }
                    printf("received gq requests.\n");
                    goto usersignedin;
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