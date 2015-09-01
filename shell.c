#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>

#define FALSE 0
#define TRUE 1
#define INPUT_STRING_SIZE 80

#include "io.h"
#include "parse.h"
#include "process.h"
#include "shell.h"


int cmd_quit(tok_t arg[]) {
  printf("Bye\n");
  exit(0);
  return 1;
}

int cmd_help(tok_t arg[]);
int cmd_cd(tok_t arg[]);

static int is_bground = 0;
// Command Lookup table 
typedef int cmd_fun_t (tok_t args[]); /* cmd functions take token array and return int */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_quit, "quit", "quit the command shell"},
  {cmd_cd, "cd", "change the directory"},
};

int cmd_help(tok_t arg[]) {
  int i;
  for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
    printf("%s - %s\n",cmd_table[i].cmd, cmd_table[i].doc);
  }
  return 1;
}

int cmd_cd(tok_t arg[]){
	if (chdir(arg[0])>=0){
	fprintf(stdout, "chdir successfull\n");
	}else
	fprintf(stderr, "chdir failed\n");

	return 1;
}


int lookup(char cmd[]) {
  int i;
  for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0)) return i;
  }
  return -1;
}

void init_shell()
{
  /* Check if we are running interactively */
  shell_terminal = STDIN_FILENO;

  /** Note that we cannot take control of the terminal if the shell
      is not interactive */
  shell_is_interactive = isatty(shell_terminal);

  if(shell_is_interactive){

    /* force into foreground */
    while(tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp()))
      kill( - shell_pgid, SIGTTIN);

    shell_pgid = getpid();
    /* Put shell in its own process group */
    if(setpgid(shell_pgid, shell_pgid) < 0){
      perror("Couldn't put the shell in its own process group");
      exit(1);
    }

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);
    tcgetattr(shell_terminal, &shell_tmodes);
  }
  /** YOUR CODE HERE */
}

/**
 * Add a process to our process list
 */
void add_process(process* p)
{
 process *q = first_process;;
  while (q->next) {
    q=q->next;
  }
  q->next = p;
  p->prev = q;
}

/**
 * Creates a process given the inputString from stdin
 */
//process* create_process(char* inputString)
process* create_process(tok_t *t)
{

process* procInfo = (process *)malloc(sizeof(process));
int i=0;
int total_toks=0;

for (i = MAXTOKS - 2; i > 0; i--) {
    if (t[i] && t[i + 1] && strcmp(t[i], "<") == 0) {
      FILE *inputFile;
      if ((inputFile = fopen(t[i + 1], "r")) != NULL) {
        int j = 0;
        procInfo->stdin = fileno(inputFile);
        for (j = i; j < MAXTOKS - 2; j++)
          t[j] = t[j + 2];
        t[j] = NULL;
        total_toks-=2;
        break;
      }
    }
  }

  for (i = MAXTOKS - 2; i > 0; i--) {
    if (t[i] && t[i + 1] && strcmp(t[i], ">") == 0) {
      FILE *outputFile;
      if ((outputFile = fopen(t[i + 1], "w")) != NULL) {
        int j = 0;
        procInfo->stdout = fileno(outputFile);
        for (j = i; j < MAXTOKS - 2; j++)
          t[j] = t[j + 2];
        t[j] = NULL;
        total_toks-=2;
        break;
      }
    }
  }

  /* support for background sign */
  if (strcmp(t[total_toks - 1], "&") == 0) {
    procInfo->background = 1;
    t[total_toks - 1] = NULL;
    total_toks--;
  }
  else if (t[0][strlen(t[0]) - 1] == '&') {
    procInfo->background = 1;
    t[0][strlen(t[0]) - 1] = 0;
  }

  procInfo->argc = total_toks;

  return procInfo;
}




int shell (int argc, char *argv[]) {
  char cwd[2018];
  char *s = malloc(INPUT_STRING_SIZE+1);			/*user input string */
  tok_t *t;			/* tokens parsed from input */
  int lineNum = 0;
  int fundex = -1;
  pid_t pid = getpid();		/* get current processes PID */
  pid_t ppid = getppid();	/* get parents PID */
  pid_t cpid, tcpid, cpgid;
 
  char *path = getenv("PATH");

    int in_redirect_flag = 0;
    int out_redirect_flag = 0;

    int i, j,k;

    int stdin_bak;
    int stdout_bak;

  printf("%s running as PID %d under %d\n",argv[0],pid,ppid);
  
  //getcwd(cwd, sizeof(cwd));
  //fprintf(stdout, "%s\n%d:",cwd,lineNum);
  
  init_shell();

  lineNum=0;
  
  getcwd(cwd, sizeof(cwd));
 fprintf(stdout, "%d %s: ", lineNum++, cwd);
 // fprintf(stdout, "%s\n%d:",cwd,lineNum);
  while ((s = freadln(stdin))){
    	
	t = getToks(s); /* break the line into tokens */
	is_bground=0;
fundex = lookup(t[0]); /* Is first token a shell literal*/
                if(fundex >= 0) {

    	cmd_table[fundex].fun(&t[1]);
    }
    else {
    	int flag =0;
    	cpid = fork();

    	pid_t mypid;
        
        if( cpid > 0 ) 
        { // parent process
          mypid = getpid();
          int status;
          pid_t tcpid = wait(&status);

          if(flag == 0 )
          {
          	printf( "%s\n","Parent Process of fork - Done with child");
          }
          
        }

      if( cpid == 0 ){ // child process
      	//char [t]
	int wordcounter=0;
      	int r=0;
      	int j=0;
     

       while (t[r] != NULL)
       {
          wordcounter++;
          r++;
       }

       char *toks [wordcounter];

       	for(j; j<=wordcounter;j++)
       	{
       		toks[j] = t[j];
       	}
      char *path =  getenv ("PATH");
//find path

      printf("%s\n", path);
      
      mypid = getpid();
      flag = execv(t[0], t);//executes program

      char *pathName;
      char *token=  strtok(path, ":");
      int meh=0;

          if(flag==-1)
          {
          	while(token !=NULL)
          	{
          		//pathName=token;
				pathName = malloc(strlen(t[0])+ strlen(token+50)); /* make space for the new string (should check the return value ...) */
				strcpy(pathName, token); 
				strcat(pathName, "/");
				strcat(pathName,t[0]);
				

          		printf("%s\n", pathName);
          		meh = execv(pathName, toks);
          		token = strtok(NULL,":");
          	}
          	
          }
      }

    }

 if (in_redirect_flag)
        {
            dup2(stdin_bak, STDIN_FILENO);
            close(stdin_bak);
        }
        if (out_redirect_flag)
        {
            dup2(stdout_bak, STDOUT_FILENO);
            close(stdout_bak);
        }
        
        in_redirect_flag = 0;
        out_redirect_flag = 0;
    	getcwd(cwd, sizeof(cwd));
    	lineNum++;
    	fprintf(stdout, "%s\n%d:",cwd,lineNum);
  }
  return 0;
}
