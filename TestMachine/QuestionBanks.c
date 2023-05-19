#include "TM.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
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
            FILE *ft = fopen(filename, "a"); // Open file in append mode
            if (ft == NULL)
            {
                perror("fopen");
            }
            buf = strtok(questionIDbuffer, ";"); // Grab the questionID
            while (buf != NULL)
            {
                fprintf(ft, "q;%s;%s;---;\n", QBversion, buf);
                buf = strtok(NULL, ";");
            }
            fclose(ft);
        }
        else if (!strcmp(commandbuffer, "AN"))
        { // If QB's need to answer questions
            printf("meant to answer here..\n");
            char *isAnswer = NULL;
            char questionIDBuf[4] = {0};
            char *answer = "#include <stdio.h>\n#include <stdlib.h>\n\nint main(int argc, char *argv[]){\nprintf(\"Hello World!\");\nreturn 0;}";
            printf("Answer is %s\n", answer);
            int questionID = 0;
            int length = strlen(answer);
            int answerLength = 0;
            if (read(pipe[0], questionIDBuf, 4) == -1) 
            {
                perror("read");
            }
            printf("Question ID is: %s\n", questionIDBuf);
            questionID = atoi(questionIDBuf);
            printf("Sending index: %d\n", questionID);
            if (send(QBsocket, "AN", 2, 0) == -1) // Send QB what it needs to prep for
            {
                perror("send");
            }
            if (send(QBsocket, &length, sizeof(int), 0) == -1) // Send answer string size to QB
            {
                perror("send");
            }
            if (send(QBsocket, answer, length, 0) == -1) // Send answer to QB
            {
                perror("send");
            }
            if (send(QBsocket, &questionID, sizeof(int), 0) == -1) // Send QuestionID to QB
            {
                perror("send");
            }
            if (recv(QBsocket, &answerLength, sizeof(int), 0) == -1) // Is answer right?
            {
                perror("recv");
            }
            isAnswer = malloc(ntohl(answerLength + 1));
            if (recv(QBsocket, isAnswer, ntohl(answerLength), 0) == -1) // Is answer right?
            {
                perror("recv");
            }
            if (!strcmp(isAnswer, "Y"))
                printf("Answer is right\n");
            else
                printf("%s\n", isAnswer);
        }
        else if (!strcmp(commandbuffer, "IQ"))
        { // If QB's need to return the answer to a question (3 failed attempts)
            printf("meant to get answer here...\n");
            int length = 0;
            int questionID = 0;
            char *answerBuf = NULL;
            char questionIDBuf[4] = {0};
            if (read(pipe[0], questionIDBuf, 4) == -1) 
            {
                perror("read");
            }
            printf("Question ID is: %s\n", questionIDBuf);
            questionID = atoi(questionIDBuf);
            printf("Sending index: %d\n", questionID);
            if (send(QBsocket, "IQ", 2, 0) == -1) // Send QB what it needs to prep for
            {
                perror("send");
            }
            if (send(QBsocket, &questionID, sizeof(int), 0) == -1) // Send QB the questionID
            {
                perror("send");
            }
            if (recv(QBsocket, &length, sizeof(int), 0) == -1) // Get length of answer from QB
            {
                perror("recv");
            }
            answerBuf = malloc(ntohl(length + 1));
            if (recv(QBsocket, answerBuf, ntohl(length), MSG_WAITALL) == -1) // Get answer
            {
                perror("recv");
            }
            printf("answer is: %s\n", answerBuf);
            free(answerBuf);
        }
        else if (!strcmp(commandbuffer, "PQ"))
        { // If QB's need to return the question from questionID
            printf("meant to return the question here...\n");
            int questionlength = 0;
            int optionlength = 0;
            int typelength = 0;
            int questionID_index = 0;
            int questionID_check = 0;
            int questionID = 0;
            char questionIDBuf[4] = {0};
            char *questionBuf = NULL;
            char *optionBuf = NULL;
            char *typeBuf = NULL;
            if (read(pipe[0], &questionID_index, sizeof(int)) == -1) 
            {
                perror("read");
            }
            printf("id is %d\n", questionID_index);
            if (read(pipe[0], questionIDBuf, 4) == -1) 
            {
                perror("read");
            }
            printf("Question ID is: %s\n", questionIDBuf);
            if (send(QBsocket, "PQ", 2, 0) == -1) // Send QB what it needs to prep for
            {
                printf("Error sending PQ");
                perror("send");
            }
            printf("Send PQ\n");
            questionID = atoi(questionIDBuf);
            printf("Sending index: %d\n", questionID);
            questionID = htonl(questionID);
            if (send(QBsocket, &questionID, sizeof(questionID), 0) == -1) // Send QB the questionID
            {
                printf("Error sending user questionID");
                perror("send");
            }
            printf("Got user ID\n");
            if (recv(QBsocket, &questionlength, sizeof(int), 0) == -1) // Get length of answer from QB
            {
                perror("recv");
            }
            printf("Got question length: %d\n", ntohl(questionlength));
            questionBuf = malloc(ntohl(questionlength + 1));
            if (recv(QBsocket, questionBuf, ntohl(questionlength), MSG_WAITALL) == -1) // Get answer
            {
                perror("recv");
            }
            printf("got question: %s\n", questionBuf);
            if (recv(QBsocket, &optionlength, sizeof(int), 0) == -1)
            {
                perror("recv");
            }
            printf("got option length: %d\n", ntohl(optionlength));
            optionBuf = malloc(ntohl(optionlength + 1));
            if (recv(QBsocket, optionBuf, ntohl(optionlength), 0) == -1)
            {
                perror("recv");
            }
            printf("got options: %s\n", optionBuf);
            if (recv(QBsocket, &typelength, sizeof(int), 0) == -1)
            {
                perror("recv");
            }
            printf("got type length: %d\n", ntohl(typelength));
            typeBuf = malloc(ntohl(typelength + 1));
            if (recv(QBsocket, typeBuf, ntohl(typelength), 0) == -1)
            {
                perror("recv");
            }
            printf("got type: %s\n", typeBuf);
            if (recv(QBsocket, &questionID_check, sizeof(int), 0) == -1)
            {
                perror("recv");
            }
            printf("got questionID: %d\n", ntohl(questionID_check));
            if (ntohl(questionID_check) != ntohl(questionID)) {
                printf("Error, received wrong question!\n");
                printf("Expected questionID: %d - Got: %d\n", ntohl(questionID), ntohl(questionID_check));
                break;
            }
            int tmquestionlength = ntohl(questionlength);
            if (write(pipe[1], &tmquestionlength, sizeof(int)) == -1) // Sending question index
            {
                perror("write");
            }
            if (write(pipe[1], questionBuf, tmquestionlength) == -1) // Sending question index
            {
                perror("write");
            }
            int tmoptionlength = ntohl(optionlength);
            if (write(pipe[1], &tmoptionlength, sizeof(int)) == -1) // Sending question index
            {
                perror("write");
            }
            if (write(pipe[1], optionBuf, tmoptionlength) == -1) // Sending question index
            {
                perror("write");
            }
            int tmtypelength = ntohl(typelength);
            if (write(pipe[1], &tmtypelength, sizeof(int)) == -1) // Sending question index
            {
                perror("write");
            }
            if (write(pipe[1], typeBuf, tmtypelength) == -1) // Sending question index
            {
                perror("write");
            }     
        }
    }
    close(QBsocket);
    close(pipe[0]);
    close(pipe[1]);
}