/* 
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file 
 * you will need to modify Makefile to compile
 * your additional functions.
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Submit the entire lab1 folder as a tar archive (.tgz).
 * Command to create submission archive: 
      $> tar cvf lab1.tgz lab1/
 *
 * All the best 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>

#define TRUE 1
#define FALSE 0

void RunCommand(int, Command *);
void DebugPrintCommand(int, Command *);
void runCommand(int, Pgm *, int);
void PrintPgm(Pgm *);
void stripwhite(char *);
int BuiltinCommands(Pgm *);
void handle_sigchld(int);
void handle_sigint(int);

// Saves fd of std output
int saved_output = STDOUT_FILENO; // 1
int status;

void handle_sigchld(int sig) {
  //WHOHANG returns immediately. 
  // THis is good since we only want to collect the status of the dead 
  // and move on.
  waitpid(-1, NULL, WNOHANG);
}

// Handles Ctrl+C
void handle_sigint(int sig) {
  printf("\n");
}

int main(void)
{
  Command cmd;
  int parse_result;

  // Assigns custom signal handlers for the main thread.
  signal(SIGCHLD, &handle_sigchld);
  signal(SIGINT, &handle_sigint);

  while (TRUE)
  {
    char *line;
    line = readline("> ");

    /* If EOF encountered, exit shell */
    if (!line)
    {
      break;
    }

    /* Remove leading and trailing whitespace from the line */
    stripwhite(line);

    /* If stripped line not blank */
    if (*line)
    {
      add_history(line);
      parse_result = parse(line, &cmd);
      RunCommand(parse_result, &cmd);
    }

    /* Clear memory */
    free(line);
  }
  return 0;
}

void RunCommand(int parse_result, Command *cmd)
{

  // Check if user typed a built-in command
  // If so, return. 
  if (BuiltinCommands(cmd->pgm) == 1) { // returns 1 if user typed built-in command
    return;
  }

  char* input = cmd->rstdin;
  char* output = cmd->rstdout;

  // default fd is stdio
  int fdin = STDIN_FILENO;
  int fdout = STDOUT_FILENO;

  // Open and/or create files is user wants to redirect I/O
  if(input != NULL) {
    fdin = open(input, O_RDONLY);
    if (fdin == -1) {printf("Error when opening file");}
  }

  if (output != NULL) {
    fdout = open(output, O_CREAT|O_WRONLY|O_TRUNC, 0666);
    if (fdout == -1) {printf("Error when creating file");}
  }

  // Fork child to run command
  pid_t processidOfChild;
  processidOfChild = fork();

  if (processidOfChild == -1) { printf("Failed to fork child\n"); } 
  else if (processidOfChild == 0) {

    // Ignore Ctrl+C if the command is  set to run in the background
    if (cmd->background) { 
      signal(SIGINT, SIG_IGN); 
    }

    // Run command with fd for in and out
    runCommand(fdin, cmd->pgm, fdout);
    exit(0);
  } 

  else {
    // if the command runs in the foreground, wait for it to finish
    // Before displaying prompt and taking more input
    if (!cmd->background) {
      // Wait for the active child.
      waitpid(processidOfChild, &status, 0);
    }
  }
  return;
}

// run command p with input from and output to
void runCommand(int from, Pgm *p, int to) {
  
  // redirect output to ''to'', always close pipes. 
  if (to != STDOUT_FILENO) {
    dup2(to, STDOUT_FILENO);
    close(to);
  }

  // Base case, if there are no more commands to come
  if (!p->next) {
    
    // Redirect standard input
    if (from != STDIN_FILENO) {
      dup2(from, STDIN_FILENO);
      close(from);
    }

    // Execute command
    int err = execvp(p->pgmlist[0], p->pgmlist);

    // This only runs if there was a problem with execvp
    // Prints to terminal using saved_output.
    dup2(saved_output, 1);
    printf("Something went wrong when running: %s\n", p->pgmlist[0]);
    close(saved_output);

    // Terminates child
    exit(1);
  }

  // Case: there are more commands to follow ---> We need pipes!
  int pipefd[2];
  int pipe_result = pipe(pipefd);

  if (pipe_result < 0) {
    printf("Could not create pipe\n");
    return;
  }

  // More commands to follow ---> We need to fork!
  pid_t processidOfChild;
  processidOfChild = fork();
      
  if (processidOfChild == -1) { printf("Failed to fork child\n"); } 
  else if (processidOfChild == 0) {

    // Close read end of pipe.
    close(pipefd[0]);

    // Recursive call, same fd from
    // but writes to pipefd[0] to send output to parent
    // pipefd[1] is closed in the first lines of the function call
    runCommand(from, p->next, pipefd[1]);
    exit(0);

  } else {
    // Close write end of pipe
    close(pipefd[1]);

    // Redirect input to read end of pipe
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);

    // Wait for any (only) child
    wait(NULL);

    // Execute command using newly redirected pipe as input
    int err = execvp(p->pgmlist[0], p->pgmlist);

    // Only prints if execvp fails
    dup2(saved_output, 1);
    printf("Something went wrong when running: %s\n", p->pgmlist[0]);
    close(saved_output);
    exit(1);
  }
}

// Handles the bultin commands: "exit" and "cd"
int BuiltinCommands(Pgm *p){
  if(strcmp(*p->pgmlist, "exit") == 0){
    exit(0);
  }
  else if(strcmp(*p->pgmlist, "cd") == 0) {
    // Special case: User wants home directory
    if(chdir(p->pgmlist[1] ? p->pgmlist[1] : getenv("HOME")) == -1){
        printf("%s: %s\n", p->pgmlist[1], strerror(errno));  
    }
    return 1;
  }
  return 0;
}

/* 
 * Print a Command structure as returned by parse on stdout. 
 * 
 * Helper function, no need to change. Might be useful to study as inpsiration.
 */
void DebugPrintCommand(int parse_result, Command *cmd)
{
  if (parse_result != 1) {
    printf("Parse ERROR\n");
    return;
  }
  printf("------------------------------\n");
  printf("Parse OK\n");
  printf("stdin:      %s\n", cmd->rstdin ? cmd->rstdin : "<none>");
  printf("stdout:     %s\n", cmd->rstdout ? cmd->rstdout : "<none>");
  printf("background: %s\n", cmd->background ? "true" : "false");
  printf("Pgms:\n");
  PrintPgm(cmd->pgm);
  printf("------------------------------\n");
}


/* Print a (linked) list of Pgm:s.
 * 
 * Helper function, no need to change. Might be useful to study as inpsiration.
 */
void PrintPgm(Pgm *p)
{
  if (p == NULL)
  {
    return;
  }
  else
  {
    char **pl = p->pgmlist;

    /* The list is in reversed order so print
     * it reversed to get right
     */
    PrintPgm(p->next);
    printf("            * [ ");
    while (*pl)
    {
      printf("%s ", *pl++);
    }
    printf("]\n");
  }
}


/* Strip whitespace from the start and end of a string. 
 *
 * Helper function, no need to change.
 */
void stripwhite(char *string)
{
  register int i = 0;

  while (isspace(string[i]))
  {
    i++;
  }

  if (i)
  {
    memmove(string, string + i, strlen(string + i) + 1);
  }

  i = strlen(string) - 1;
  while (i > 0 && isspace(string[i]))
  {
    i--;
  }

  string[++i] = '\0';
}
