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
    int score; // Max of 30
}curUser;

curUser user;

int login(char username[], char password[]) {

    char* line = NULL;
    char* buf;
    size_t linesize = 0;
    ssize_t linelen;
    int counter = 0;

    /*
    int fd = open("users.txt", O_RDONLY); // Open users.txt in read only mode
    if (fd == -1) {
        printf("Error number %d\n", errno);
        perror("Program");
    }
    */

    FILE *fp = fopen("users.txt", "r");

    while ((linelen = getline(&line, &linesize, fp)) != -1) {
        buf = strtok(line, " ");
        if (!strcasecmp(buf, username)) { // If strings equal (ignoring case-sensitive for username)
            strcpy(user.username, buf);
            buf = strtok(NULL, " "); 
            if (!strcasecmp(buf, password)) {
                strcpy(user.password, buf);
                printf("Correct Credentials\n");
            }
        }
        else
            continue;  
    }   
    //printf("%s", password);
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