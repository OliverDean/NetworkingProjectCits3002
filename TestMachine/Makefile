# A Makefile for our project
C11 = cc -std=c11
CFLAGS = -Wall -pedantic -Werror

PROJECT = TM
HEADERS = $(PROJECT).h
OBJ = TM.o parse.o handleHTTP.o QuestionBanks.o user.o 

$(PROJECT) : $(OBJ)
	$(C11) $(CFLAGS) -o $(PROJECT) $(OBJ) -lm

%.o:%c $(HEADERS)
	$(C11) $(CFLAGS) -c $<

clean:
	rm -f $(PROJECT) $(OBJ)
	rm -f $(OBJ)