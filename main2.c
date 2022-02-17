#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define DEFAULT_PATH "/bin"
#define BUFFER_SIZE 256
#define MAX_ARGC_NUM 64
#define DELIMETER " \n\t"
#define REDIRECTION_SYMBOL '>'
#define PARALLEL_SYMBOL '&'

static char *allPaths[MAX_ARGC_NUM];
int isHaveDirection = 0;
sig_atomic_t isInBackGround = 0;

void background_job_handler(int signum)
{
    int old_errno = errno;
    int status;
    const char *correct_msg = "background process terminated\n";
    const char *child_error_msg = "Child Terminated Abnormally\n";
    const char *waitpid_error_msg = "waitpid() Abnormally\n";
    while (waitpid(-1, &status, WNOHANG) >= 0)
    {
        if (WIFEXITED(status))
        {
            write(1, correct_msg, strlen(correct_msg));
        }else{
            write(2, child_error_msg, strlen(child_error_msg));
        }
    }
    if (errno != ECHILD)
    {
        write(2, waitpid_error_msg, strlen(waitpid_error_msg));
    }
    isInBackGround = 0;
    errno = old_errno;
}

char getLastChar(char *str)
{
    return str[strlen(str) - 1];
}

int isLastCharSayBackground(char *str)
{
    char *locationOfbackgroundSymbol = strchr(str, '&');
    if (locationOfbackgroundSymbol)
    {
        char* restStr = (locationOfbackgroundSymbol + 1);
        while(*restStr != '\0'){
            char currentChar = *restStr;
            if(currentChar!=' ' && currentChar != '\n' && currentChar != '\t'){
                return 0;
            }
            restStr++;
        }
        return 1;
    }
    else
    {

        return 0;
    }
}

void freeAllPaths()
{
    for (int i = 0; i < sizeof(allPaths) / sizeof(allPaths[0]); i++)
    {
        if (allPaths[i])
        {
            free(allPaths[i]);
        }
    }
}

void jumpToExit(int exitState)
{
    freeAllPaths();
    exit(exitState);
}

void addPathToAllPaths(char *paths)
{
    int i = 0;
    char *newPath = malloc(strlen(paths) + 1);
    strcpy(newPath, paths);
    while (allPaths[i] != NULL)
    {
        i++;
    }

    allPaths[i] = newPath;
    allPaths[++i] = NULL;
}

int getIndexOfLastItemInArr(char *arr[])
{
    int i = 0;
    while (arr[i])
    {
        i++;
    }
    return i - 1;
}

void removeRedirectionSymbolAndDestFileNameFromInput(char *input[], size_t length)
{
    int indexOfRedirectionSymbol = 0;
    int tempLength = length;

    while (strcmp(input[indexOfRedirectionSymbol], ">") != 0)
    {
        ++indexOfRedirectionSymbol;
    }

    int indexOfFileName = length - 1;
    int indexToDelete[] = {indexOfRedirectionSymbol, indexOfFileName};

    for (int i = 0; i < 2; i++)
    {
        for (int z = indexToDelete[i]; z < length; z++)
        {
            input[z] = input[z + 1];
        }
        tempLength--;
    }
    input[tempLength] = NULL;
}

void copyFileNameFromArrToFileDest(char *fileDest, char *arr[], int indexOfFileName)
{
    strcpy(fileDest, arr[indexOfFileName]);
}

void printWrongArgumentMessage()
{
    char *error_message = "Invalid Argument. Please Try Again\n";
    write(2, error_message, strlen(error_message));
}

int getCharFreqInString(char *string, char theChar)
{
    int counter = 0;
    int len = strlen(string);
    for (int i = 0; i < len; i++)
    {
        char currnetChar = string[i];
        if (currnetChar == theChar)
        {
            counter++;
        }
    }
    return counter;
}

int getRedirectionSymbolNum(char *input)
{
    return getCharFreqInString(input, REDIRECTION_SYMBOL);
}



void getUserLine(char **dest)
{
    size_t buffer_size = 0;
    int isReadLineSuccess;
    isReadLineSuccess = getline(dest, &buffer_size, stdin);
    if (isReadLineSuccess == -1)
    {
        free(*dest);
        if (feof(stdin))
        {
            printf("\nExit Wish Shell\n");
            jumpToExit(EXIT_SUCCESS);
        }
        else
        {
            perror("Read Line failed");
            jumpToExit(EXIT_FAILURE);
        }
    }
}

void splitALineToArr(char *inputLine, char *splitArr[], char *delim)
{
    int i = 0;
    char *p = strtok(inputLine, delim);
    while (p != NULL)
    {
        splitArr[i] = p;
        p = strtok(NULL, delim);
        i++;
    }
}

void splitInputLineWithParallelSym(char *inputLine, char *splitArr[])
{
    splitALineToArr(inputLine, splitArr, "&\n");
}

void splitInputLine(char *userInputLineSplitArr[], char *inputLine)
{
    splitALineToArr(inputLine, userInputLineSplitArr, DELIMETER);
}

void copyFileNameFromArrToFileDestAndRemoveItFromInput(char *fileDest, char *inputLineArr[])
{
    int lastIndexInArr = getIndexOfLastItemInArr(inputLineArr);
    copyFileNameFromArrToFileDest(fileDest, inputLineArr, lastIndexInArr);
    removeRedirectionSymbolAndDestFileNameFromInput(inputLineArr, lastIndexInArr);
}

int didUserWantToExit(char *input)
{
    for (int i = 0; input[i]; i++)
    {
        input[i] = tolower(input[i]);
    }
    return (strcmp(input, "exit") == 0);
}

void setOutputToFileDest(char *fileDest)
{
    printf("Has Redirection\n");
    int fd = open(fileDest, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    dup2(fd, 1);
    close(fd);
}

void execute2(char **argv, char *fileDest)
{
    pid_t pid;
    int status;

    if ((pid = fork()) < 0)
    { /* fork a child process           */
        printf("*** ERROR: forking child process failed\n");
        exit(1);
    }
    else if (pid == 0)
    { /* for the child process:         */
        printf("argv[0] is now %s\n", argv[0]);
        if (isHaveDirection == 1)
        {
            setOutputToFileDest(fileDest);
        }

        if (execv(argv[0], argv) < 0)
        { /* execute the command  */
            printf("*** ERROR: exec failed\n");
            exit(1);
        }
    }
    else
    {
        if (!isInBackGround)
        {

            waitpid(pid, NULL, 0);
        } /* for the parent:      */
    }
}

void custome_exit()
{
    jumpToExit(0);
}

int custome_cd(char *argv[])
{

    // If we write no path (only 'cd'), then go to the home directory
    if (argv[1] == NULL)
    {
        chdir(getenv("HOME"));
        return 0;
    }
    // Else we change the directory to the one specified by the
    // argument, if possible
    else
    {
        if (chdir(argv[1]) != 0)
        {
            printf(" %s: no such directory\n", argv[1]);
            return -1;
        }
    }

    return 0;
}

void clearAllPath()
{
    for (int i = 0; i < sizeof(allPaths) / sizeof(allPaths[0]); i++)
    {
        if (allPaths[i])
        {
            free(allPaths[i]);
        }
        allPaths[i] = NULL;
    }
}

void printAllPaths()
{
    int z = 0;
    for (int i = 0; i < sizeof(allPaths) / sizeof(allPaths[0]); i++)
    {
        if (allPaths[i])
        {
            printf("| path %d is %s |\n", i, allPaths[i]);
        }
        //putchar('\n');
    }
}

void custome_path(char *argv[])
{
    clearAllPath();
    if (argv[1] == NULL)
    {
        printf("clear all path\n");
    }
    else
    {
        printf("first argumetn in path: %s\n", argv[1]);
        for (int i = 1; argv[i]; i++)
        {
            addPathToAllPaths(argv[i]);
        }
        printAllPaths();
    }
}

char *addPathToCommandName(char *commandName, char *path)
{
    int newCommandLength = strlen(path) + strlen(commandName) + 2;
    char *pathBeforeCommand = malloc(newCommandLength);
    strcpy(pathBeforeCommand, path);
    strcat(pathBeforeCommand, "/");
    strcat(pathBeforeCommand, commandName);
    return pathBeforeCommand;
}

char *addDefaultPathToCommandName(char *commandName)
{
    return addPathToCommandName(commandName, DEFAULT_PATH);
}

int findPathOfCommand(char *commandName)
{
    char *pathBeforeCommand;
    int pathIndex = -1;
    char *currentPath;
    int allPathSize = sizeof(allPaths) / sizeof(allPaths[0]);
    for (int i = 0; i < allPathSize; i++)
    {

        if (allPaths[i] != NULL)
        {
            pathBeforeCommand = addPathToCommandName(commandName, allPaths[i]);
            //printf("full path: %s\n", pathBeforeCommand);
            if (access(pathBeforeCommand, X_OK) == 0)
            {
                free(pathBeforeCommand);
                return i;
            }
            free(pathBeforeCommand);
        }
    }
    //free(pathBeforeCommand);
    return pathIndex;
}

void decideIfEnterBuidinCommand(char *argv[], char *fileDest)
{
    char *enteredCommand = argv[0];
    //execute custome function for those builtin functions
    if (strcmp(enteredCommand, "cd") == 0)
    {
        custome_cd(argv);
    }
    else if (strcmp(enteredCommand, "path") == 0)
    {

        custome_path(argv);
    }
    else if (strcmp(enteredCommand, "exit") == 0)
    {
        custome_exit();
    }
    else //Below is command executed by OS
    {

        int pathIndex = findPathOfCommand(argv[0]);
        if (pathIndex == -1)
        {
            printf("Can't find match command among all existing path\n");
        }
        else
        {
            argv[0] = addPathToCommandName(argv[0], allPaths[pathIndex]);
            //printf("Full path is now %s\n", argv[0]);
            execute2(argv, fileDest);
            free(argv[0]);
        }
    }
}

void inputLineSplitArrInialize(char *arr[], size_t size)
{
    for (int i = 0; i < size; i++)
    {
        arr[i] = NULL;
    }
}

int interActiveMode(int argc, char *argv[])
{
    char *inputLine = NULL; //in heap
    char *inputLineSplitArr[MAX_ARGC_NUM] = {NULL};
    char fileDest[MAX_ARGC_NUM];
    int lastIndexInArr = -1;
    int numOfRedirectionSymbol = 0;
    int numOfParallelSymbol = 0;
    while (1)
    {
        isHaveDirection = 0;
        numOfRedirectionSymbol = 0;
        lastIndexInArr = -1;

        //printAllPaths();
        printf("wish> ");
        getUserLine(&inputLine);
        numOfRedirectionSymbol = getRedirectionSymbolNum(inputLine);
        if (isLastCharSayBackground(inputLine))
        {
            isInBackGround = 1;
            char* locationnOfBackgroundSym = strchr(inputLine, '&');
            *locationnOfBackgroundSym = ' ';
        }
        
        splitInputLine(inputLineSplitArr, inputLine);
        if (numOfRedirectionSymbol == 1)
        {
            copyFileNameFromArrToFileDestAndRemoveItFromInput(fileDest, inputLineSplitArr);
            isHaveDirection = 1;
        }
        else if (numOfRedirectionSymbol > 1)
        {
            printf("Can't have more than one '>'! \n");
            continue;
        }

        decideIfEnterBuidinCommand(inputLineSplitArr, fileDest);
        free(inputLine);
    }
}

int main(int argc, char *argv[])
{
    signal(SIGCHLD, background_job_handler);
    clearAllPath();
    addPathToAllPaths(DEFAULT_PATH);

    if (argc == 2)
    { //* Batch mode
        FILE *readedFile;
    }
    else if (argc == 1)
    { //* Interactive mode
        interActiveMode(argc, argv);
        jumpToExit(0);
    }
    else
    {
        printWrongArgumentMessage();
        jumpToExit(1);
    }
}