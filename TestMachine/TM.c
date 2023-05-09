#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

//TM WILL BE HOSTING ON http://192.168.220.118/

typedef struct curUser {
    char username[32];
    char password[32];
    int score; // Max of 30
}curUser;

int main(int argc, char* argv[]) {

}

int login(char username[], char password[]) {
    
}