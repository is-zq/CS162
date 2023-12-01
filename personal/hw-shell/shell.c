#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens* tokens);
int cmd_help(struct tokens* tokens);
int cmd_pwd(struct tokens* tokens);
int cmd_cd(struct tokens* tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens* tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t* fun;
  char* cmd;
  char* doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
    {cmd_help, "?", "show this help menu"},
    {cmd_exit, "exit", "exit the command shell"},
	{cmd_pwd, "pwd", "prints the current working directory"},
	{cmd_cd, "cd", "changes the current working directory"},
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens* tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens* tokens) { exit(0); }

/* Prints the current working directory */
int cmd_pwd(unused struct tokens* tokens)
{
	char buff[4096];
	if(getcwd(buff,sizeof(buff)) == NULL)
	{
		perror("getcwd");
		return 0;
	}
	printf("%s\n",buff);
	return 1;
}

/* Changes the current working directory */
int cmd_cd(struct tokens* tokens)
{
	if(chdir(tokens_get_token(tokens,0)) == -1)
	{
		perror("chdir");
		return 0;
	}
	return 1;
}

/* run a program */
void run_program(char* prog)
{
	char* tokens[34] = {NULL};
	char* argv[32] = {NULL};
	size_t i;
	size_t tok_len = 0;
	char* saveptr1;
	for(char* token = strtok_r(prog," ",&saveptr1);token != NULL;token = strtok_r(NULL," ",&saveptr1))
	{
		tokens[tok_len++] = token;
	}
	char* path = tokens[0];

	/* get argv */
	argv[0] = path;
	if(tok_len >= 3 && strcmp(tokens[tok_len-2],"<") == 0)
	{
		for(i=1;i<tok_len-2;i++)
			argv[i] = tokens[i];
		freopen(tokens[tok_len-1],"r",stdin);
	}
	else if(tok_len >= 3 && strcmp(tokens[tok_len-2],">") == 0)
	{
		for(i=1;i<tok_len-2;i++)
			argv[i] = tokens[i];
		freopen(tokens[tok_len-1],"w",stdout);
	}
	else
	{
		for(i=1;i<tok_len;i++)
			argv[i] = tokens[i];
	}
	argv[i] = NULL;

	/* execute */
	if(path[0] == '/')
	{
		execv(path,argv);
		perror("exec");
		exit(-1);
	}
	else
	{
		char* PATH = getenv("PATH");
		char rpath[4096];
		char* saveptr2;
		for(char* token = strtok_r(PATH,":",&saveptr2);token != NULL;token = strtok_r(NULL,":",&saveptr2))
		{
			strncpy(rpath,token,sizeof(rpath));
			strcat(rpath,"/");
			strcat(rpath,path);
			if(access(rpath,F_OK) == 0)
				execv(rpath,argv);
		}
		perror("exec");
		exit(-1);
	}
}

/* start session */
void start_session(struct tokens* tokens)
{
	size_t tok_len = tokens_get_length(tokens);
	char* prog_arr[512];
	int pipe_arr[512][2];
	size_t i;
	size_t count=0;

	/* split by '|' */
	for(i=0;i<tok_len;i++)
	{
		char* token = tokens_get_token(tokens,i);
		if(strcmp(token,"|") == 0)
		{
			int pip[2];
			if(pipe(pip) == -1)
			{
				perror("pipe");
				return;
			}
			pipe_arr[count][0] = pip[0];
			pipe_arr[count][1] = pip[1];
			count++;
		}
		else
		{
			if(prog_arr[count] == NULL)
			{
				prog_arr[count] = (char*)malloc(256*sizeof(char));
				strncpy(prog_arr[count],token,256);
			}
			else
			{
				char* prog = prog_arr[count];
				if(strlen(prog) + strlen(token) >= 256)
				{
					printf("too long !\n");
					return;
				}
				prog[strlen(prog)] = ' ';
				prog[strlen(prog)] = '\0';
				strcat(prog_arr[count],token);
			}
		}
	}
	count++;

	/* run programs */
	pid_t pid;
	for(i=0;i<count;i++)
	{
		pid = fork();
		if(pid == -1)
		{
			perror("fork");
			return;
		}
		else if(pid == 0)
		{
			if(setpgrp() == -1)		//set as its own process group
			{
				perror("setpgrp");
				return;
			}
			if(tcsetpgrp(STDIN_FILENO,getpid()) == -1)	//place in the foreground
			{
				perror("tcsetpgrp");
				return;
			}

			for(int sig=1;sig<=64;sig++)		//reset signal handler
				signal(sig,SIG_DFL);

			if(count >= 2)
			{
				if(i == 0)
					dup2(pipe_arr[i][1],1);
				else if(i == count-1)
					dup2(pipe_arr[i-1][0],0);
				else
				{
					dup2(pipe_arr[i-1][0],0);
					dup2(pipe_arr[i][1],1);
				}
				for(int j=0;j<count-1;j++)
				{
					close(pipe_arr[j][0]);
					close(pipe_arr[j][1]);
				}
			}
			run_program(prog_arr[i]);
		}
		if(i != 0)
			close(pipe_arr[i-1][0]);
		if(i != count - 1)
			close(pipe_arr[i][1]);
		if(wait(NULL) == -1)
		{
			perror("wait");
			return;
		}
	}

	/* release resources */
	for(i=0;i<count;i++)
	{
		free(prog_arr[i]);
		prog_arr[i] = NULL;
	}

	if(tcsetpgrp(STDIN_FILENO,getpid()) == -1)	//reset shell to foreground
	{
		perror("tcsetpgrp");
		return;
	}
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
		return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

int main(unused int argc, unused char* argv[]) {
  init_shell();

  for(int i=1;i<=64;i++)	//ignore signals
	  signal(i,SIG_IGN);
  signal(17,SIG_DFL);

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens* tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
	  start_session(tokens);
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
