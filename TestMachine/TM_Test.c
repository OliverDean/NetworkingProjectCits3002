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
    randomstring[8] = '\0';
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
int attempt_login(int newtm_fd, char *username, char *password)
{
    curUser user;
    printf("Attempting login...\n");
    int login_result = login(username, password, &user);
    if (login_result == 0)
    {
        printf("Login succeeded.\n");
        
        // Copy the user_filename into a temporary variable
        char temp_filename[9];
        strncpy(temp_filename, user.user_filename, 9);
        
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
    // Only attempt login if both username and password are present in the query string
    if (hasUsername && hasPassword) {
        return attempt_login(newtm_fd, username, password);
    } else {
        //Send the login page as the HTTP response
        handleRequest(newtm_fd, httpRequest, user);
        return 1;
    }
}

// Function to send a question request
void send_question_request(curUser *user, int question_index, int cqbpipe[2], int pyqbpipe[2])
{
    char code[3];
    strncpy(code, "PQ", 3); // Setting the command code to "PQ"

    printf("Sending PQ for question %d\n", question_index + 1);

    // Send the code "PQ" to the correct question bank based on the question type
    int pipe_to_write = (strcmp(user->QB[question_index], "c") == 0) ? cqbpipe[1] : pyqbpipe[1];

    if (write(pipe_to_write, code, 3) == -1)
    {
        perror("write");
    }

    // Convert the index to network byte order before sending
    uint32_t network_order_index = htonl(question_index);
    if (write(pipe_to_write, &network_order_index, sizeof(network_order_index)) == -1)
    {
        perror("write");
    }

    // Send the question ID itself to the question bank
    if (write(pipe_to_write, user->QuestionID[question_index], 4) == -1)
    {
        perror("write");
    }

    printf("Request sent for question ID: %s\n", user->QuestionID[question_index]);
}

// send_question_request(&user, question_index, cqbpipe, pyqbpipe);

// Function to send an answer request
void send_answer_request(curUser *user, int question_index, char *answer, int QBsocket)
{
    char code[3];
    strncpy(code, "AN", 3); // Setting the command code to "AN"

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

    char isAnswer[2];                         // Buffer to store the response
    if (recv(QBsocket, isAnswer, 1, 0) == -1) // Is answer right?
    {
        perror("recv");
    }
    isAnswer[1] = '\0'; // Ensure null termination

    if (!strcmp(isAnswer, "Y"))
        printf("Answer is right\n");
    else
        printf("Answer is wrong\n");
}


void get_question(curUser user, int question_index, int cqbpipe[2], int pyqbpipe[2])
{
    int pipe[2] = 0;
    chartype = NULL;
    char question = NULL;
    charoption = NULL;
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

    if (write(pipe, "PQ", 3) == -1)
    {
        perror("write");
    }
    if (write(pipe[1], &index, sizeof(int)) == -1) // Sending question index
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

    // Write question, options, and type into a temporary text file
    char fileName[50];
    sprintf(fileName, "question_%d.txt", question_index);
    FILE *file = fopen(fileName, "w");
    if (file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "Question: %s\nOption: %s\nType: %s\n", question, option, type);
    fclose(file);

    // Clean up
    free(question);
    free(option);
    free(type);
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
        char acceptchar[2];
        FILE *fp;
        newtm_fd = accept(tm_fd, (struct sockaddr *)&tm_addr, &tm_size);
        if (newtm_fd == -1)
        {
            perror("accept");
            break;
        }
        printf("connection made!\n");
        // needs to change into if else loops
        // checks the http header and first line.
        // if the first line is a get request, then it will check the username and password

        curUser(user);
        char buffer[3000];
        read(newtm_fd, buffer, 3000);
        // printf("buffer:\n%s\n", buffer);
        // Parse the HTTP request
        HttpRequest httpRequest = parseHttpRequest(buffer);
        printf("httpRequest.requestLine.uri.queryString: %s\n", httpRequest.requestLine.uri.queryString);
        printf("httpRequest.requestLine.uri.path: %s\n", httpRequest.requestLine.uri.path);
        printf("checking user...\n");
        checkUser(newtm_fd, httpRequest, username, password);
        printf("thats done\n");
        handleRequest(newtm_fd, httpRequest);
        printf("httpRequest.requestLine.uri.queryString after handelrequest: %s\n", httpRequest.requestLine.uri.queryString);
        
        

        printf("dlReturn: %i\n", dlReturn);

        close(newtm_fd);

        if (1)
        //strlen(httpRequest.requestLine.uri.queryString) != 0
        {
            cuReturn = checkUser(newtm_fd, httpRequest, username, password);
            
            if (cuReturn == 0)
            {
                //close(tm_fd);
                curUser user;
                int loadValue = 0;
                char *cqbverf = NULL;
                char *pqbverf = NULL;
                int loginValue = -1;

                if (dlReturn == 0)
                {
                    printf("here!\n");
                    loginValue = login(username, password, &user);
                }
                else if (dlReturn == 2)
                {
                    printf("here2!\n");
                    loginValue = 0;
                }
                else if (dlReturn == 1)
                {
                    printf("here3!\n");
                    loginValue = -1;
                }
                
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

                loginValue = login(username, password, &user);
                
                if (loginValue == -2) // ADD || loadValue == -1) in the if statement once loadUser sorted
                {
                    generatenewfile(&user);
                    printf("user->user.filename: %s\n", user.user_filename);

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
                    if (cqbverf == NULL && pqbverf == NULL) 
                    {
                        if (read(cqbpipe[0], cqbverf, 3) == -1)
                            printf("error1\n");
                            perror("read");
                        if (read(pqbpipe[0], pqbverf, 3) == -1)
                            printf("error2\n");
                            perror("read");
                    }
                    printf("Verifications: c %s python %s\n", cqbverf, pqbverf);
                    loadValue = loadUser(&user);
                    if (loadValue == 0) {loginValue=0;}
                    printf("loadValue: %i\n", loadValue);
                }
                else if (loginValue == -1 || loginValue == 1) //username doesn't exist or password is wrong
                {
                    // SHOULD PROBABLY HAVE ANOTHER HTML FOR THAT OR JS THAT GENERATES ?

                    sendHttpResponse(newtm_fd,"./ClientBrowser/login.html", HTML, user.user_filename);
                    printf("\tclosing connection 2...\n");
                    close(newtm_fd);
                }
                else if (loadValue == 0 && loginValue == 0) // if all is well:
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
                    printf("user->user.filename: %s\n", user.user_filename);
                    //sendRedirectResponse(newtm_fd, "/question_dashboard");
                    displaylogin(newtm_fd, username, password);
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
                            if (strcasecmp(user.QB[index], "c"))
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
                            else if(strcasecmp(user.QB[index], "python"))
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
                }
            }
        }

        handleRequest(newtm_fd, httpRequest);
        // printf("httpRequest: %i\n", r);
        // printf("\tclosing connection...\n");
        close(newtm_fd);
    }
    shutdown(tm_fd, SHUT_RDWR);
    close(tm_fd);
    return 0;
}





//         close(newtm_fd);

//         switch (fork())
//         {
//         case -1:
//             perror("fork");
//             exit(EXIT_FAILURE);
//             break;
//         case 0:
//         {
//             // close(tm_fd);
//             curUser user;
//             int loadValue = 0;
//             char *cqbverf = NULL;
//             char *pqbverf = NULL;
//             int loginValue = -1;
//             int cheUer = checkUser(newtm_fd, httpRequest, username, password);
//             printf("dlReturn: %i\n", dlReturn);

//             if (loginValue == -1) // Invalid Username (doesn't exist)
//             {
//                 if (send(newtm_fd, "Username Invalid.\n", 19, 0) == -1)
//                     perror("send");
//                 continue;
//             }
//             else if (loginValue == 1) // Invalid Password
//             {
//                 if (send(newtm_fd, "Incorrect Password.\n", 21, 0) == -1)
//                     perror("send");
//                 continue;
//             }
//             printf("User signed in!\n");
//             printf("User filename is: %s\n", user.user_filename);

//             if (loginValue == 0)
//             { // If file does exist
//                 printf("File exists.\n");
//                 loadValue = loadUser(&user);
//                 printf("loadvalue is: %d\n", loadValue);
//             }

//             while (loginValue == -2 || loadValue == -1) // If file failed to open
//             {
//                 printf("Calling generate new file.\n");
//                 generatenewfile(&user);
//                 printf("Generated new cookiefile\n");
//                 printf("user->user.filename: %s\n", user.user_filename);
//                 printf("sending gq request\n");
//                 if (write(cqbpipe[1], "GQ", 3) == -1) // Generate questions from C QB
//                 {
//                     perror("write");
//                 }
//                 if (write(cqbpipe[1], user.user_filename, 8) == -1)
//                 {
//                     perror("write");
//                 }
//                 printf("C QB success.\n");
//                 if (write(pqbpipe[1], "GQ", 3) == -1) // Generate questions from Python QB
//                 {
//                     perror("write");
//                 }
//                 if (write(pqbpipe[1], user.user_filename, 8) == -1)
//                 {
//                     perror("write");
//                 }
//                 printf("received gq requests.\n");
//                 if (cqbverf == NULL && pqbverf == NULL)
//                 {
//                     if (read(cqbpipe[0], cqbverf, 3) == -1)
//                         printf("error1\n");
//                     perror("read");
//                     if (read(pqbpipe[0], pqbverf, 3) == -1)
//                         printf("error2\n");
//                     perror("read");
//                 }
//                 // printf("Verifications: c %s python %s", cqbverf, pqbverf);
//                 loadValue = loadUser(&user);
//                 if (loadValue == 0)
//                     loginValue = 0;
//                 printf("user->user.filename: after all the writing to the file %s\n", user.user_filename);
//             }

//             if (loadValue == 0 && loginValue == 0) // Everything Works!
//             {
//                 char code[3] = {0};
//                 /*
//                 Here is where the HTTP requests will come through once the user is signed in
//                 In here all user data is loaded into the structure
//                 You will need to send off the 3 byte (2 bytes sent to the python QB cause C has null terminated strings) to the correct QB through the pipes
//                 cqbpipe[1] is the input for the C QB, cqbpipe[0] is the output from the C QB
//                 pqbpipe[1] is the input for the Python QB, pqbpipe[1] is the output from the Python QB
//                 REMEMBER: CQB NOR PQB HAVE THE CORRECT USER STRUCTURE, YOU WILL NEED TO PASS THEM THE REQUIRED INFO.
//                 After the user closes the browser make sure the connection is broken (goes through below close() steps)
//                 */
//                 printf("user->user.filename: %s\n", user.user_filename);
//                 // sendRedirectResponse(newtm_fd, "/question_dashboard");
//                 cuReturn = checkUser(newtm_fd, httpRequest, username, password);
//                 printf("successful signin.\n");
//                 while (1) // Testing QB connection
//                 {
//                     char indexbuf[4] = {0};
//                     int index = 0;
//                     char buffer[1024] = {0};
//                     memset(code, 0, sizeof(code));
//                     if (send(newtm_fd, "Please enter a code: ", 21, 0) == -1)
//                         perror("send");
//                     if (recv(newtm_fd, code, 3, 0) == -1)
//                         perror("recv");
//                     if (recv(newtm_fd, buffer, 1024, 0) == -1)
//                         perror("recv");
//                     code[strcspn(code, "\n")] = '\0';
//                     code[strcspn(code, "\r")] = '\0';

//                     printf("code is: %s\n", code);

//                     if (!strcmp(code, "PQ"))
//                     {
//                         printf("inside PQ.\n");
//                         if (send(newtm_fd, "Please enter a question index (0-9): ", 37, 0) == -1)
//                             perror("send");
//                         if (recv(newtm_fd, indexbuf, 4, 0) == -1)
//                             perror("recv");
//                         printf("index buf is: %s\n", indexbuf);
//                         index = atoi(indexbuf);
//                         printf("index is: %d\n", index);
//                         code[strcspn(code, "\n")] = '\0';
//                         code[strcspn(code, "\r")] = '\0';
//                         if (strcasecmp(user.QB[index], "c"))
//                         {
//                             printf("Sending PQ to c\n");
//                             if (write(cqbpipe[1], "PQ", 3) == -1)
//                             {
//                                 perror("write");
//                             }
//                             if (write(cqbpipe[1], &index, sizeof(int)) == -1) // Sending question index
//                             {
//                                 perror("write");
//                             }
//                             if (write(cqbpipe[1], user.QuestionID[index], 4) == -1) // Sending questionID
//                             {
//                                 perror("write");
//                             }
//                             printf("Grabbing question");
//                         }
//                         else if (strcasecmp(user.QB[index], "python"))
//                         {
//                             printf("Sending PQ to p\n");
//                             if (write(pqbpipe[1], "PQ", 3) == -1)
//                             {
//                                 perror("write");
//                             }
//                             if (write(pqbpipe[1], &index, sizeof(int)) == -1) // Question 2 is index 3
//                             {
//                                 perror("write");
//                             }
//                             printf("Question ID is: %s\n", user.QuestionID[index]);
//                             if (write(pqbpipe[1], user.QuestionID[index], 4) == -1) // Question 2 is index 3
//                             {
//                                 perror("write");
//                             }
//                             printf("Grabbing question");
//                         }
//                     }
//                     else if (!strcasecmp(code, "AN"))
//                     {
//                         printf("Inside AN\n");
//                         if (send(newtm_fd, "Please enter a question index (0-9): ", 37, 0) == -1)
//                             perror("send");
//                         if (recv(newtm_fd, indexbuf, 4, 0) == -1)
//                             perror("recv");
//                         printf("index buf is: %s\n", indexbuf);
//                         index = atoi(indexbuf);
//                         printf("index is: %d\n", index);
//                         if (strcasecmp(user.QB[index], "c"))
//                         {
//                             printf("Sending AN to c\n");
//                             if (write(cqbpipe[1], "AN", 3) == -1)
//                             {
//                                 perror("write");
//                             }
//                             if (write(cqbpipe[1], user.QuestionID[index], sizeof(int)) == -1)
//                             {
//                                 perror("write");
//                             }
//                         }
//                         else if (strcasecmp(user.QB[index], "python"))
//                         {
//                             printf("Sending AN to p\n");
//                             if (write(pqbpipe[1], "AN", 3) == -1)
//                             {
//                                 perror("write");
//                             }
//                             if (write(pqbpipe[1], user.QuestionID[index], sizeof(int)) == -1)
//                             {
//                                 perror("write");
//                             }
//                         }
//                     }
//                     else if (!strcasecmp(code, "IQ"))
//                     {
//                         printf("Inside AN\n");
//                         if (send(newtm_fd, "Please enter a question index (0-9): ", 37, 0) == -1)
//                             perror("send");
//                         if (recv(newtm_fd, indexbuf, 4, 0) == -1)
//                             perror("recv");
//                         if (strcasecmp(user.QB[index], "c"))
//                         {
//                             printf("Sending IQ to c\n");
//                             if (write(cqbpipe[1], "IQ", 3) == -1)
//                             {
//                                 perror("write");
//                             }
//                             if (write(cqbpipe[1], user.QuestionID[index], sizeof(int)) == -1)
//                             {
//                                 perror("write");
//                             }
//                         }
//                         else if (strcasecmp(user.QB[index], "python"))
//                         {
//                             printf("Sending IQ to p\n");
//                             if (write(pqbpipe[1], "IQ", 3) == -1)
//                             {
//                                 perror("write");
//                             }
//                             if (write(pqbpipe[1], user.QuestionID[index], sizeof(int)) == -1)
//                             {
//                                 perror("write");
//                             }
//                         }
//                     }
//                 }
//                 // send(newtm_fd, questionDashboard(), strlen(questionDashboard()), 0);
//                 close(newtm_fd);
//                 close(cqbpipe[1]);
//                 close(cqbpipe[0]);
//                 close(pqbpipe[0]);
//                 close(pqbpipe[1]);
//                 break;
//             }
//         }
//         break;
//         default:
//             close(newtm_fd);
//             break;
//         }
//         shutdown(tm_fd, SHUT_RDWR);
//         close(tm_fd);
//         return 0;
//     }
// }