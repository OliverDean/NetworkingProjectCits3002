#include "TM.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// Loads user data into structure
// Returns -1 on failure with file (corrupted / not built correctly)
// Returns 0 on success
int loadUser(curUser *user)
{
    printf("Inside load user.\n");
    char *line = NULL;
    char *buf = NULL;
    size_t linesize = 0;
    ssize_t linelen = 0;
    int QBcounter = 0;
    memset(user->questions, 0, sizeof(user->questions)); // make array all 0
    user->total_score = 0;
    user->attempted = 0;

    printf("Opening file\n");
    FILE *fp;
    fp = fopen(user->user_filename, "r"); // Open user file
    if (fp == NULL)
    {
        perror("fopen");
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
        if ((strcmp(buf, user->username)) && (strcmp(buf, "q")))
        { // If username in file doesn't match signed in user
            printf("Incorrect username in file!\nUsername in file: %sUsername given:%s\n", buf, user->username);
            return -1;
        }
        else if (!strcmp(buf, "q"))
        { // question line
            char *t = NULL;
            buf = strtok(NULL, ";"); // QB its from
            user->QB[QBcounter] = malloc(strlen(buf));
            strcpy(user->QB[QBcounter], buf);
            user->QB[QBcounter][strlen(buf)] = '\0';

            buf = strtok(NULL, ";"); // QuestionID
            user->QuestionID[QBcounter] = malloc(strlen(buf));
            strcpy(user->QuestionID[QBcounter], buf);
            user->QuestionID[QBcounter][strlen(buf)] = '\0';

            buf = strtok(NULL, ";"); // Attempted marks

            // For the marks string
            for (t = buf; *t != '\0'; t++) {
                if (*t == 'N')
                { // If user has answered incorrectly
                    if (user->questions[QBcounter] == 0)
                        user->attempted++;
                    user->questions[QBcounter]++;
                }
                else if (*t == 'Y')
                { // If user has answered correctly
                    user->score[QBcounter] = 3 - user->questions[QBcounter];
                    printf("Score is: %d\n", user->score[QBcounter]);
                    user->total_score += user->score[QBcounter];
                    printf("Total score is: %d\n", user->total_score);
                    if (user->questions[QBcounter] == 0)
                    {
                        user->attempted++;
                    }
                }
                else if (*t == '-')
                {
                    break;
                }
            }
            QBcounter++;
        }
    }
    fclose(fp);


    if (QBcounter == 10) // If there are 10 questions
        return 0;
    else if (QBcounter < 10) { // If not 10 questions, error out
        printf("Returning -1 from loaduser.\n");
        return -1;
    }
    else if (QBcounter <= 0) // If somehow Qbcounter is below 0 (if we get here we are screwed)
        return -1;
    return -1;
}

// Logs user in, Returns 0 on success,
// 1 on password failure, returns -1 on login failure, returns -2 if user file is missing
int login(char username[], char password[], curUser *user)
{

    char temp[32];
    char *line = NULL;
    char *buf;
    size_t linesize = 0;
    ssize_t linelen;
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
                strcpy(user->password, buf);
                strcpy(user->username, temp);
                buf = strtok(NULL, ";");
                buf[strcspn(buf, "\n")] = '\0';
                buf[strcspn(buf, "\r")] = '\0';
                printf("File looking at is: %s\n", buf);
                if (buf == NULL || *buf == '\0')
                {
                    return -2;
                }
                strcpy(user->user_filename, buf);
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

// Generates new cookie file for user, adds it to users.txt
// Re-creates users.txt to avoid over-writing user data
void generatenewfile(curUser *user)
{
    char *line = NULL;
    char *buf = NULL;
    size_t linesize = 0;
    ssize_t linelen;
    int counter = 0;
    char *randomstring = randomStringGenerator(); // Generate new user cookie
    strcpy(user->user_filename, randomstring);
    FILE *fp = fopen(user->user_filename, "w"); // Create new user cookie file
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
        else if (!strcasecmp(buf, user->username))
        { // Line we want
            buf = strtok(NULL, ";");
            if (!strcmp(buf, user->password))
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
                fprintf(new, "%s;\n", user->user_filename); // Add users cookie filename
                fprintf(fp, "%s;\n", user->username);       // Add users username to their own cookie file
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
    printf("Removing users.");
    remove("users.txt");
    printf("renaming temp.\n");
    rename("temptemptemp", "users.txt");
}