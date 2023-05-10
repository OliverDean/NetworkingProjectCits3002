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