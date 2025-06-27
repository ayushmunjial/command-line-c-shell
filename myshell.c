/************************************************************************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <assert.h>

/************************************************************************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

/************************************************************************************************************************************/

#include <sys/wait.h> // Declares the functions used for holding processes / waiting.
#include <sys/types.h>
#include <sys/stat.h>

/************************************************************************************************************************************/

#ifndef BUFFER_SIZE 
#define BUFFER_SIZE 1024 
#endif // To define amount of memory allocated for the temporary storage of data.

/************************************************************************************************************************************/

#ifndef NUMOFTOKENS 
#define NUMOFTOKENS 256
#define SIZEOFTOKEN 512
#endif // To define the length and number of tokens, to create array where all tokens will be stored.

/************************************************************************************************************************************/

char alltokens[NUMOFTOKENS][SIZEOFTOKEN]; // Creating an array to store all the tokens in the given input.
char arguments[NUMOFTOKENS][SIZEOFTOKEN]; // Creating an array to store all normal/wildcard('*') arguments.

/************************************************************************************************************************************/

int parse_command(char *buffer, char alltokens[NUMOFTOKENS][SIZEOFTOKEN], int *tokenPos, int *charPos);
int execute_Command(int NumOfTokens);
int execute_Process(int fd_in, int fd_out, int firstPos, int lastPos);
int storeArguments(char *given_token, char arguments[NUMOFTOKENS][SIZEOFTOKEN], int *argument_Pos);

/************************************************************************************************************************************/

int main(int argc, char **argv) {
    if (argc > 2) { printf("Too many arguments given to the shell\n"); exit(1); } // More than one file as input.
    FILE *fd = stdin; // fd is the file descriptor which is initialized to be set to the standard input.

    int last_command_status = 0; // To check the exit status of last command.
    int running_mode = 0; // If 0, interactive mode, else if 1, batch mode.

    if (argc == 2) { running_mode = 1; fd = fopen(argv[1], "r"); // To check if shell is in batch mode.
        if (!fd) {  perror("open"); exit(1); } // Opening specified file.
    }
    else { running_mode = 0; write(STDOUT_FILENO, "Welcome to my shell!\n", 21); } // If in the interactive mode.

    size_t buffer_size = BUFFER_SIZE; char *buffer = malloc(buffer_size); // To store the input given.
    int currTokenPos = 0, currCharacterPos = 0; // Variables to keep track of the current token and character positions.

    // Infinite loop that reads input from set fd, tokenizes it, and executes commands until the program is terminated.

    while (1) { // One input loop and command parsing algorithm that works for both the batch and interactive modes.

        if (running_mode == 0) { // Only works if the shell is in the interactive mode.
            if (last_command_status != 0) { write(STDOUT_FILENO, "!mysh> ", 7); } 
            else { write(STDOUT_FILENO, "mysh> ", 6); }
        }

        ssize_t bytes_read = getline(&buffer, &buffer_size, fd); if (bytes_read < 0) break;

        /* Parsing through the input buffer to tokenize it and store it into an array of tokens called alltokens.
        The function updates currTokPos and currCharPos to keep track of the current position in the input buffer. */

        int NumOfTokens = parse_command(buffer, alltokens, &currTokenPos, &currCharacterPos);

        if (NumOfTokens == -1) { continue; } /* If parse_command returns -1, it means that the last character in the 
        input buffer was a backslash character, and the newline character still needs to be read to complete the 
        command. In this case, the loop continues to read the newline character in the next iteration. */
        
        last_command_status = execute_Command(NumOfTokens); // Then, finally we execute the input command.
    }
    free(buffer); return 0;
}

/************************************************************************************************************************************/

/* The function 'parse_command' is used to parse the given input buffer and tokenize it into individual components or tokens and 
store them in the alltokens array. It also takes as input tokenPos (a pointer to an integer that tracks the current position of 
the token being processed), charPos (a pointer to an integer that tracks the current position of the character being processed 
within a token) and returns an integer representing the total number of tokens in the alltokens array. */

/************************************************************************************************************************************/

int parse_command(char *buffer, char alltokens[NUMOFTOKENS][SIZEOFTOKEN], int *tokenPos, int *charPos) {

    // The loop will terminate when it reaches either the end of the string '\0' or the end of the current command '\n'.
    for (int bufferPos = 0; buffer[bufferPos] && buffer[bufferPos] != '\n'; ++bufferPos) { char chr = buffer[bufferPos];

        if (chr == '\\') { char nextchr = buffer[bufferPos+1]; // Extension: 3.1 Escape sequences : escaping of special characters.
            // To represent the backslash character in a  character literal in C, you need to use the two-character sequence '\\'.

            if (nextchr == '\n' || !nextchr) { return -1; } // To check if the next character is a newline or the end of the buffer.
            // If not it returns -1, indicating that the input line was not complete. 

            alltokens[*tokenPos][(*charPos)++] = nextchr; // If yes, next character is added to current token in the tokens array.
            ++bufferPos; // Counter is incremented to skip over the next character.
        }

        else if (chr == '*') { alltokens[*tokenPos][(*charPos)++] = -chr; } // Wildcards are added to the current token as negative.
            
        else if ((chr == '|' || chr == '<' || chr == '>' || chr == ' ')) {  // To check if the current character is one of these.

            // If not space, it sets token as the specified delimiter and then adds the null character to the end to complete it.
            if (chr != ' ') { alltokens[(*tokenPos)][0] = chr; alltokens[(*tokenPos)++][1] = '\0'; }

            if (*charPos == 0 && chr == ' ') continue;  // If it is a space and the current token is empty, continue.

            // To check if there are any characters in the current token. If yes, adds the null character to the end to complete it.
            if (*charPos) { alltokens[(*tokenPos)++][*charPos] = '\0'; *charPos = 0; } 
        }
        else { alltokens[*tokenPos][(*charPos)++] = chr; } // Otherwise, if it is a normal character, simply add to current token.
    }

    if (*charPos != 0) { alltokens[(*tokenPos)++][*charPos] = '\0'; *charPos = 0; }
    int NumOfTokens = *tokenPos; *tokenPos = 0; return NumOfTokens;
}

/************************************************************************************************************************************/

/* The function 'execute_Command' takes as input the number of tokens in the input command and returns an integer representing the 
status of the last command executed. */

/************************************************************************************************************************************/

int execute_Command(int NumOfTokens) {
    int fd_in = STDIN_FILENO, fd_out = STDOUT_FILENO; // File descriptors which are set to standard input/output.
    int last_command_status = 0; int p[2]; // Pipe object used for inter-process communication during pipe.    

    for (int i = 0; i < NumOfTokens; ) { // Loop to iterate over each token in the input stored in alltokens.
        
        int j = i; last_command_status = 0; //  // To track the exit status of last command. 

        while (j < NumOfTokens) { /* Nested 'while' loop that searches for special characters such as <, >, |, and / 
        in the input string and breaks if it finds a pipe '|'. */

            if (strcmp(alltokens[j], "exit") == 0) { write(STDOUT_FILENO, "Exiting from my shell!\n", 23); exit(EXIT_SUCCESS); }
            else if (strcmp(alltokens[j], "|") == 0) { break; } // If we find a pipe we will come out of while loop.

            else if (strcmp(alltokens[j], "<") == 0) { fd_in = open(alltokens[j+1], O_RDONLY); // Input redirection.
                if (fd_in < 0) { last_command_status = -1; perror("open"); }
            } 

            else if (strcmp(alltokens[j], ">") == 0) { fd_out = open(alltokens[j+1], O_CREAT | O_TRUNC | O_WRONLY, 0640); 
                if (fd_out < 0) { last_command_status = -1;  perror("open"); } // Output redirection.
            } 

            else if (strcmp(alltokens[j], "/") == 0) { if (alltokens[j][0] != '~' || alltokens[j][1] != '/') return 1;
                char path[SIZEOFTOKEN]; strcpy(path, getenv("HOME")); // Extension: 3.2 Home directory
                strcat(path, alltokens[j]+1); strcpy(alltokens[j], path);
            } ++j;
        }
        
        if (strcmp(alltokens[j], "|") == 0) { // Check if the last token encountered was a pipe '|'. If yes, create pipe.
            if (pipe(p) < 0) { perror("pipe"); last_command_status = -1; } // If pipe fails during its creation.

            if (fd_out == STDOUT_FILENO) { fd_out = p[1]; } // Updating fd_out to the write end of the pipe (p[1]).
            // This is done because output of current command will be written to the pipe, which is read by the next command.
        }

        // Executing the subcommand of the pipe which is the sequence of tokens from i to j-1 in the alltokens array.
        if (last_command_status >= 0) { last_command_status = execute_Process(fd_in, fd_out, i, j-1); }

        if ((strcmp(alltokens[j], "|") == 0)) { fd_in = p[0]; } // Check if the previous token encountered was a pipe '|'.
        // If yes, then the read end of the pipe is set as the input file descriptor (fd_in) for the next command.

        else { if (fd_in != STDIN_FILENO) { close(fd_in); } fd_in = STDIN_FILENO; } // If not pipe, and fd != standard input.

        if (fd_out != STDOUT_FILENO) { close(fd_out); } fd_out = STDOUT_FILENO; i = j+1;
    }

    if (fd_in != STDIN_FILENO) { close(fd_in); } if (fd_out != STDOUT_FILENO) { close(fd_out); } // Set back fd_in & set_out.
    return last_command_status; // Finally return the exit status of the current command to print prompt accordingly.
}

/************************************************************************************************************************************/

/* The function 'execute_Process' is responsible for executing a subcommand that occurs before the pipe token in the input line.
It take in as input the file descriptors for input and output source already set accordingly in the 'execute_command'. Therefore, 
we do not have to consider the tokens '>' and '<' again, cause they have been dealt with earlier already. It returns an integer 
representing the status of the last command executed. */

/************************************************************************************************************************************/

int execute_Process(int fd_in, int fd_out, int firstPos, int lastPos) { 
    
    int last_command_status; int argument_Pos = 0; // Variable to keep track of the position of the arguments in the arguments array.

    for (int i = firstPos; i <= lastPos; ++i) { // To skip over any pipe, input redirection, output redirection, or whitespace tokens. 
        if (alltokens[i][0] == '|' || alltokens[i][0] == '<' || alltokens[i][0] == '>' || alltokens[i][0] == ' ') { ++i; continue; }
        storeArguments(alltokens[i], arguments, &argument_Pos); // To store each non-skipped token arguments in the arguments array.
    }
    arguments[argument_Pos][0] = '\0'; // After the for loop, the arguments array is terminated with a null pointer.

    // After the argument list has been created, we now prepare for the execution of arguments.

    if ((strcmp(arguments[0], "cd") == 0)) { // To check if the first argument in the array is 'cd'.

        if (firstPos == lastPos) { return chdir(getenv("HOME")); } // This checks whether there is any argument after 'cd' or not.

        // Otherwise, the directory is changed to the directory specified by the second argument by doing [firstPos+1].
        last_command_status = chdir(alltokens[firstPos+1]); if (last_command_status < 0) { perror("cd"); }
        return last_command_status;
    }

    if (strcmp(arguments[0], "/") != 0) { struct stat stat_ob;  // To check if the first argument is not an absolute path.

        /* If not, the code searches for the file in several system directories. If the program is found in any of these 
        directories, the path to the program is added prior to the first argument in the arguments array.*/

        char *systemdirs[] = { "/usr/local/sbin/", "/usr/local/bin/", "/usr/sbin/", "/usr/bin/", "/sbin/", "/bin/" };
        char store[SIZEOFTOKEN];

        for (int dname = 0; dname < 6; ++dname) { strcpy(store, systemdirs[dname]); strcat(store, arguments[0]);
            int check = stat(store, &stat_ob); if (check < 0) { continue; }
            strcpy(arguments[0], store); break; // Stores the directory in arguments.
        }
    }
    char *execArguments[argument_Pos+1]; // An array to hold the arguments that are passed to the execv function.
    for (int i = 0; i < argument_Pos; ++i) { execArguments[i] = arguments[i]; } execArguments[argument_Pos] = NULL;
    
    pid_t pid = fork(); if (pid == -1) { perror("fork"); return 1; } // Forking a child process.

    if (pid == 0) { // If pid is 0, then the current process is the child process. Now, setting up file redirection.
        if (dup2(fd_in, STDIN_FILENO) < 0) { perror("dup2"); exit(1); }
        if (dup2(fd_out, STDOUT_FILENO) < 0) { perror("dup2"); exit(1); }

        last_command_status = execv(execArguments[0], execArguments); // Executing with the arguments in the execArguments array.

        if (last_command_status == -1) { perror("execv"); exit(last_command_status); }
    }
    else { wait(&last_command_status); } // Parent process : wait for child process to complete.
    return last_command_status; // Finally return the exit status of the current command to print prompt accordingly.
}

/************************************************************************************************************************************/

int storeArguments(char *given_token, char arguments[NUMOFTOKENS][SIZEOFTOKEN], int *argument_Pos) {
    
    int i, j; int containsAsterik = 0; // Variable to flag the token if it contains a asterik wildcard.

    /* Iterates backwards through the characters of the given token until it finds a forward slash (/) or reaches the beginning of 
    the string. This loop also checks for the presence of any wildcards in the given token. A wildcard is any character whose ASCII 
    value is less than zero. */

    for (i = strlen(given_token)-1; i >= 0 && given_token[i] != '/'; --i) { containsAsterik |= given_token[i] < 0; }

    if (!containsAsterik) { strcpy(arguments[(*argument_Pos)++], given_token); return 1; } //  If no wildcard, it stores the token.

    DIR *dp; char path_Directory[SIZEOFTOKEN]; // Array to hold the directory path that contains the files matching the wildcard.
    
    if (i < 0) { strcpy(path_Directory, ""); dp = opendir("."); }  // Checking if the wildcard is at the beginning of given token.
    // This is done by testing whether the index 'i' has reached the beginning of the string.

    else { memcpy(path_Directory, given_token, i+1); dp = opendir(path_Directory); }

    char before_Wildcard[SIZEOFTOKEN]; int before_length; // Array to store the part of the wildcard that occurred before it. 
    char after_Wildcard[SIZEOFTOKEN]; int after_length; // Array to store the part of the wildcard that occurred after it. 

    for (++i, j = 0; given_token[i] > 0; ++i, ++j) { before_Wildcard[j] = given_token[i]; } // Storing part before wildcard occurs.
    before_length = j; before_Wildcard[j] = '\0'; // To add the null terminating character to the end to complete the string.

    given_token[i] = given_token[i] * (-1); //  This is used to mark the end of the before part and the start of the after part.
    for (++i, j = 0; given_token[i] > 0; ++i, ++j) { after_Wildcard[j] = given_token[i]; } // Storing part after wildcard occurs.
    after_length = j; after_Wildcard[j] = '\0'; // To add the null terminating character to the end to complete the string.

    struct dirent *de; int NumOfMatches = 0; // To track the number of filenames whose names matched with the wildcard patterns.

    while ((de = readdir(dp))) { // This reads the directory contents one by one and assigns the value to the de pointer. 
        
        char *dname = de->d_name; if (dname[0] == '.') { continue; } // To check if the file name starts with a dot. 
        // If yes, the loop skips to the next iteration using the continue statement, and does not process the current file.

        int equals; int dname_length = strlen(dname); // The variable equals stores result of the pattern matching.

        if (dname_length < before_length + after_length) continue; // This means that this isn't the desired file, so it continues.

        if (before_length != 0) { equals = memcmp(before_Wildcard, dname, before_length); if (equals != 0) continue; } 

        if (after_length != 0) { equals = memcmp(after_Wildcard, dname + (dname_length - after_length), after_length); 
        if (equals != 0) continue; } // Comparison is done by using pointer arithmetic to skip over the characters.

        ++NumOfMatches; // If the filename matched with the wildcard patterns, we increment the total number of matches.
        strcpy(arguments[*argument_Pos], path_Directory); strcat(arguments[(*argument_Pos)++], dname);
        // Finally, we add the matched filename to the arguments array.
    }

    if (!NumOfMatches) { // If the total number of matches is none, we just add the argument without changing it.
        strcpy(arguments[*argument_Pos], path_Directory); strcat(arguments[(*argument_Pos)++], given_token);
    }
    return NumOfMatches;
}

/************************************************************************************************************************************/