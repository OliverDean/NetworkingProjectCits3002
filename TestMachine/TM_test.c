#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {

    char* buf = (char*)calloc(20000, sizeof(char)); 

    int fd = open("TM_user.c", O_WRONLY | O_CREAT | O_TRUNC); // Open TM_user.c file stream (empty file) - opens in write only, creates the file and trunicates said file
    if (fd == -1) {
        printf("Error number %d\n", errno);
        perror("Program");
    }

    int fp = open("TM_test.txt", O_RDONLY); // Open TM_test.txt file stream
    if (fp == -1) {                         // TM_test.txt is a pre-defined text file containing a .c program
        printf("Error number %d\n", errno);
        perror("Program");
    }

    int sz = read(fp, buf, 20000); // This is a pretend TCP stream (from user inputted browser)
    buf[sz] = '\0';

    int fz = write(fd, buf, sz); // This writes the TM_test.txt to TM_user.c (again pretend inputted file stream from browser)

    /*  
        printf("Please enter the code you are wanting to compile:\n");
            while (fgets(yes, 20000, stdin) != NULL) { // getting from stdin
            fputs(yes, fp);
        }
    */

    int thepipe[2];

    if (pipe(thepipe) != 0) {
        printf("Error number %d\n", errno);
        perror("Program");
    }
    switch (fork()) {
        case -1:
            printf("Error number %d\n", errno);
            perror("Program");
            break;
        case 0:
            close (thepipe[1]);
            dup2(thepipe[0], 0);
            close(thepipe[0]);

            execl("/usr/bin/gcc", "gcc", "-o", "TM_user", "TM_user.c", (char *)0);
            perror("/sr/bin/gcc");
            break;
        default:
            break;
    }
    return 0;
}