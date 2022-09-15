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

#include<time.h>

#define TRUE 1
#define FALSE 0

void RunCommand(int, Command *);
void DebugPrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);

int main(void)
{
  Command cmd;
  int parse_result;

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



void DateCommand(){
  time_t tm;
  time(&tm);
  printf("%s", ctime(&tm));
}
//TODO Show login time, not sure how
void WhoCommand(){
  printf("%s\n", getenv("USERNAME"));
  
}
void ExitCommand(){
  exit(0);
}
void lsCommand(){
  
}
void cdCommand(){
  
}
void pwdCommand(){
  char cwd[1024];
  getcwd(cwd, sizeof(cwd));
  printf("%s\n", cwd);

}



void RunCommand(int parse_result, Command *cmd)
{
  DebugPrintCommand(parse_result, cmd);

  //Creates a char array with the input text
  char *cmd_text = *cmd->pgm->pgmlist;
  
  if(strcmp(cmd_text, "who") == 0){
    WhoCommand();
  }
  else if(strcmp(cmd_text, "date") == 0){
    DateCommand();
  }
  else if(strcmp(cmd_text, "exit") == 0){
    ExitCommand();
  }
  else if(strcmp(cmd_text, "ls") == 0){
    lsCommand();
  }
  else if(strcmp(cmd_text, "cd") == 0){
    cdCommand();
  }
  else if(strcmp(cmd_text, "pwd") == 0){
    pwdCommand();
  }


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
