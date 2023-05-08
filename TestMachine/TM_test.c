#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    char* buf = (char*)calloc(20000, sizeof(char)); 
    int fd = open("TM_user.txt", O_WRONLY | O_CREAT);
    if (fd == -1) {
        printf("Error number %d\n", errno);
        perror("Program");
    }
    
    printf("Please enter the code you are wanting to compile:\n");
    while (fgets(yes, 20000, stdin) != NULL) { // getting from stdin
        fputs(yes, fp);
    }

    int thepipe[2];

    if (pipe(thepipe) != 0) {
        printf("cannot create pipe\n");
        exit(EXIT_FAILURE);
    }
    switch (fork()) {
        case -1:
            printf("fork failed\n");
            exit(EXIT_FAILURE);
            break;
        case 0:
            close (thepipe[1]);
            dup2(thepipe[0], 0);
            close(thepipe[0]);

            execl("/usr/bin/gcc", "gcc", "-o", "TM_test", "TM_test.c", (char *)0);
            perror("/sr/bin/gcc");
            exit(EXIT_FAILURE);
            break;
        default:
            break;
    }
    return 0;
}