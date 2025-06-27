all : myshell.c
			gcc -g -Wall -o mysh myshell.c

clean:
			rm -f mysh -r buildTesting
