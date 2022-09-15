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

#define TRUE 1
#define FALSE 0

void RunCommand(int, Command *);
void DebugPrintCommand(int, Command *);
void runCommand(int, Pgm *, int);
void PrintPgm(Pgm *);
void stripwhite(char *);


//This is enough to kill all processes
void handle_signal(int sig){
  printf("\n"); 
}


int main(void)
{
  Command cmd;
  int parse_result;

  struct sigaction sa;
  sa.sa_handler = &handle_signal;
  sigaction(SIGINT, &sa, NULL);


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



/* Execute the given command(s).

 * Note: The function currently only prints the command(s).
 * 
 * TODO: 
 * 1. Implement this function so that it executes the given command(s).
 * 2. Remove the debug printing before the final submission.
 */
void RunCommand(int parse_result, Command *cmd)
{
  // DebugPrintCommand(parse_result, cmd);
  
  // check for built-in commands
  // check for background? 

  char* input = cmd->rstdin;
  char* output = cmd->rstdout;

  int fdin = STDIN_FILENO;
  int fdout = STDOUT_FILENO;

  if(input != 0) {
    fdin = open(input, O_RDONLY);
    if (fdin == -1) {printf("Error when opening file");}
  }

  if (output != 0) {
    fdout = open(output, O_CREAT|O_WRONLY|O_TRUNC);
    if (fdout == -1) {printf("Error when creatingm file");}
  }

  
  runCommand(fdin, cmd->pgm, fdout);
  return;
}


//run in backround: create fork and don't wait
//signal dont listen to CTRL-C
//not working...
void BackgroundCommand(int fdin, Command *cmd, int fdout){
  pid_t processidOfChild;
  processidOfChild = fork();
  if (processidOfChild == -1) { printf("Failed to fork child\n"); } 
  else if (processidOfChild == 0) {
    signal(SIGINT, SIG_IGN);
    
    runCommand(fdin, cmd->pgm, fdout);
  }

}

void runCommand(int from, Pgm *p, int to) {
  if (p == NULL) {
    if (from != STDIN_FILENO) {
      // change std input;
      dup2(from, STDIN_FILENO);
      close(from);
    }
    return; 
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

    if (to != STDOUT_FILENO) {
      // change std outout
      dup2(to, STDOUT_FILENO);
      close(to);
    }

    runCommand(from, p->next, pipefd[1]);

    // change std input;
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[1]);
    close(pipefd[0]);

    int err = execvp(p->pgmlist[0], p->pgmlist);
    exit(0);

  } else {              //Does it need to be an else here?
    close(pipefd[0]);   //Should we close the pipes after wait? To make sure everything gets received
    close(pipefd[1]);
    
    wait(NULL); 
  }

  return;
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
