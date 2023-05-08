#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#define OPTLIST "pc"

int main(int argc, char *argv[]) {

    // Call using ./TM_test -c to compile C
    // Call using ./TM_test -p to compile python

    char* buf = (char*)calloc(20000, sizeof(char));
    int fd, fp, sz, fz, opt;
    char buffer;
    opt = getopt(argc, argv, OPTLIST);
    if (opt == -1) {
        printf("Opt Error\n");
        perror("Program");
    }

    fd = open("TM_user.c", O_WRONLY | O_CREAT | O_TRUNC); // Open TM_user.c file stream (empty file) - opens in write only, creates the file and trunicates said file
    if (fd == -1) {
        printf("Error number %d\n", errno);
        perror("Program");
    }

    fp = open("TM_test.txt", O_RDONLY); // Open TM_test.txt file stream
    if (fp == -1) {                         // TM_test.txt is a pre-defined text file containing a .c program
        printf("Error number %d\n", errno);
        perror("Program");
    }

    sz = read(fp, buf, 20000); // This is a pretend TCP stream (from user inputted browser)
    buf[sz] = '\0';

    fz = write(fd, buf, sz); // This writes the TM_test.txt to TM_user.c (again pretend inputted file stream from browser)
    close(fz);
    close(sz);
    close(fd);
    close(fp);

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
            close (thepipe[0]); // Close the reading end of the pipe.
            fflush(stdout);
            dup2(thepipe[1], 1); // Duplicate the stdout for the pipe writing.
            close (thepipe[1]);
            if (opt == 'c') {
                printf("c selected\n");
                switch (fork()) {
                    case -1: 
                        printf("Error number %d\n", errno);
                        perror("Program");
                        break;
                    case 0:
                        execl("/usr/bin/gcc", "gcc", "-o", "TM_user", "TM_user.c", (char *)0);
                        perror("/usr/bin/gcc");
                    default:
                        wait(NULL);
                        break;
                }
                system("./TM_user");
            }
            else if (opt == 'p') {
                printf("python selected\n");
                system("python TM_python.py");
            }
            else {
                printf("incorrect input\n");
                perror("Program");
            }
            break;
        default:
            close(thepipe[1]);
            while (read(thepipe[0], &buffer, 1) > 0) 
                write(STDOUT_FILENO, &buffer, 1);
            close(thepipe[0]);
            break;
    }
    return 0;
}