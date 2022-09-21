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


int saved_output = 1;

void handle_sigchld(int sig) {
  //WHOHANG returns immediately. THis is good since we only want to collect the status of the dead and move on.
  waitpid(-1, NULL, WNOHANG);
}
void handle_sigint(int sig) {
  printf("\n");
}

int main(void)
{
  Command cmd;
  int parse_result;

  //signal(SIGINT, SIG_IGN);

/*
  struct sigaction sa;
  sa.sa_handler = &handle_sigchld;
  sigaction(SIGCHLD, &sa, NULL);
  
  struct sigaction sab;
  sab.sa_handler = &handle_sigchld;
  sigaction(SIGINT, &sab, NULL);
*/

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
  if (BuiltinCommands(cmd->pgm) == 1) {
    return;
  }

  char* input = cmd->rstdin;
  char* output = cmd->rstdout;

  int fdin = STDIN_FILENO;
  int fdout = STDOUT_FILENO;

  if(input != NULL) {
    fdin = open(input, O_RDONLY);
    if (fdin == -1) {printf("Error when opening file");}
  }

  if (output != NULL) {
    fdout = open(output, O_CREAT|O_WRONLY|O_TRUNC, 0666);
    if (fdout == -1) {printf("Error when creating file");}
  }

  pid_t processidOfChild;
  processidOfChild = fork();

  if (processidOfChild == -1) { printf("Failed to fork child\n"); } 
  else if (processidOfChild == 0) {

    if (cmd->background) { signal(SIGINT, SIG_IGN); 
    }
    runCommand(fdin, cmd->pgm, fdout);
    exit(0);

  } 
  else {
    if (!cmd->background) { wait(NULL); }
  }
  
  return;
}

void runCommand(int from, Pgm *p, int to) {
  
  if (to != STDOUT_FILENO) {
    dup2(to, STDOUT_FILENO);
    close(to);
  }

  if (!p->next) {
    
    if (from != STDIN_FILENO) {
      dup2(from, STDIN_FILENO);
      close(from);
    }

    int err = execvp(p->pgmlist[0], p->pgmlist); 

    dup2(saved_output, 1);
    printf("Something went wrong when running: %s\n", p->pgmlist[0]);
    close(saved_output);

    exit(1);
  }

  int pipefd[2];
  int pipe_result = pipe(pipefd);

  if (pipe_result < 0) {
    printf("Could not create pipe\n");
    return;
  }

  pid_t processidOfChild;
  processidOfChild = fork();
      
  if (processidOfChild == -1) { printf("Failed to fork child\n"); } 
  else if (processidOfChild == 0) {

    //signal(SIGINT, SIG_DFL);

    close(pipefd[0]);
    runCommand(from, p->next, pipefd[1]);
    exit(0);

  } else {
    close(pipefd[1]);
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);

    wait(NULL);

    int err = execvp(p->pgmlist[0], p->pgmlist);
    dup2(saved_output, 1);
    printf("Something went wrong when running: %s\n", p->pgmlist[0]);
    close(saved_output);
    exit(1);
  }
}

// Handles the bultin commands: "exit" and "cd"
// This checks if 1st word is exit/cd
// We should check nr of arguments
int BuiltinCommands(Pgm *p){
  if(strcmp(*p->pgmlist, "exit") == 0){
    exit(0);
  }
  else if(strcmp(*p->pgmlist, "cd") == 0) {
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
