#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>


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

int login(char username[], char password[]) {

    char* line = NULL;
    char* buf;
    size_t linesize = 0;
    ssize_t linelen;
    int counter = 0;

    FILE *fp = fopen("users.txt", "r");

    while ((linelen = getline(&line, &linesize, fp)) != -1) {
        char *temp;
        buf = strtok(line, " ");
        if (!strcmp(buf, "//")) // If comment line
            continue;
        if (!strcasecmp(buf, username)) { // If strings equal (ignoring case-sensitive for username)
            strcpy(&temp, buf);
            buf = strtok(NULL, " "); 
            if (!strcasecmp(buf, password)) {
                strcpy(user.password, buf);
                strcpy(user.username, &temp);
                printf("Correct Credentials\n");
            }
            else {
                printf("Incorrect Password\n");
            }
        }
        else
            continue;  
    }   
    return 0;
}

int main(int argc, char* argv[]) {
    char username[32];
    char password[32];

    printf("Please enter a username: ");
    scanf("%s", username);

    printf("\nPlease enter your password: ");
    scanf("%s", password);
 
    int temp = login(username, password);
    return 0;
}