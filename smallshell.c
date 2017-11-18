#include "smallshell.h"

/* Program buffers & Work pointers */
static char inpbuf[MAXBUF], tokbuf[2 * MAXBUF];
static char *ptr = inpbuf, *tok = tokbuf;

static int intr_p = 0;
static int fg_pid = 0;


/* Signal handlers */
void handle_int(int signo) {
	int c;

	/* If the signal was generated on shell prompt */
	if (fg_pid == 0) {
		if (signo == 2) { /* SIGINT (^C) */
			printf("Cauht SIGINT\n");
		} else if (signo == 3) { /* SIGQUIT (^\) */
			printf("Cauht SIGQUIT\n");
		} else if (signo == 20) { /* SIGTSTP (^Z) */
			printf("Cauht SIGTSTP\n");
		}
		return;
	}

	/* Signal was generated on processing command */
	if (intr_p == 0) { /* Ignore first interrupt(signal) */
		printf("\nignoring, signal again to interrupt\n");
		intr_p = 1;
	} else { /* Take repeated signal */
		printf("\ngot it, signalling\n");
		kill(fg_pid, SIGTERM);
		fg_pid = 0;
	}
}


/* Private Functions */
static char special[] = {' ', '\t', '&', ';', '\n', '|', '<', '>', '\0'};
static int inarg(char c) { /* Discriminate the special character */
	char *wrk;
	for (wrk = special; *wrk != '\0'; wrk++) {
		if (c == *wrk) {
			/* is special character */
			return 0;
		}
	}
	/* is plain character */
	return 1;
}

static int gettok(char **outptr) { /* Get token & Place into tokbuf */
	int type;

	*outptr = tok;

	/* strip white space */
	for (; *ptr == ' ' || *ptr == '\t'; ptr++) {
		// empty
	}

	/* Push first character of token */
	*tok++ = *ptr;

	switch (*ptr++) { /* Each token is separated by special characters (except ' ', '\t') */
	case '\n':
		type = EOL;
		break;
	case '&':
		type = AMPERSAND;
		break;
	case ';':
		type = SEMICOLON;
		break;
	case '|':
		type = PIPELINE;
		break;
	case '<':
		type = LESS;
		break;
	case '>':
		type = GREATER;
		break;
	default:
		type = ARG;

		/* Push remaining characters of token */
		while(inarg(*ptr)) {
			*tok++ = *ptr++;
		}
	}
	*tok++ = '\0';
	return type;
}

static int runcommand(char **cline, int where, int in, int out) { /* Execute a command with optional wait */
	int pid, exitstat, ret;

	/* Implement "cd" command (change directory) */
	if (strcmp(cline[0], "cd") == 0) { 
		if (chdir(cline[1]) == -1) {
			perror("smallshell");
			return -1;
		}
		return 0;
	}

	/* Implement "exit" command (exit the shell) */
	if (strcmp(cline[0], "exit") == 0) {
		exit(0);
	}

	/* Ignore signal (linux) */
	struct sigaction sa_ign;
	sa_ign.sa_handler = SIG_IGN;
	sigemptyset(&sa_ign.sa_mask);
	sa_ign.sa_flags = SA_RESTART;

	/* Make new(child) process */
	if ((pid = fork()) < 0) {
		perror("smallshell");
		return -1;
	}

	if (pid == 0) { /* Child */
		sigaction(SIGINT, &sa_ign, NULL);
		sigaction(SIGQUIT, &sa_ign, NULL);
		sigaction(SIGTSTP, &sa_ign, NULL);

		/* Change standard input/output if needed */
		if (in != -1) { 
			dup2(in, 0);
			close(in);
		}
		if (out != -1) {
			dup2(out, 1);
			close(out);
		}
		
		/* Run command */
		execvp(*cline, cline);

		/* exec error */
		perror(*cline);
		exit(127);
	}

	/* Parent code */

	fg_pid = pid;

	/* If child is background process, print pid of child and exit */
	if (where == BACKGROUND) {
		fg_pid = 0;
		printf("[Process id %d]\n", pid);
		return 0;
	}

	/* Wait until child process exit */
	while ( ((ret = wait(&exitstat)) != pid) && (ret != -1) ) {
		// empty
	}

	fg_pid = 0;
	return (ret == -1) ? 1 : exitstat;	
}


/* Public Functions */
int userin(char *p) { /* Print prompt & Read a line */
	int c, count;
	char wd[MAXBUF];

	/* Init for later routines */
	ptr = inpbuf;
	tok = tokbuf;
	
	/* Display prompt */
	getcwd(wd, MAXBUF);
	printf("%s%s$ ", p, wd);
	
	/* Read a line */
	for (count = 0;;) {
		/* Get a character */
		if ((c = getchar()) == EOF) {
			return EOF;
		}
		if (count < MAXBUF) {
			inpbuf[count++] = c;
		}
		
		/* End of line */
		if (c == '\n' && count < MAXBUF) {
			inpbuf[count] = '\0';
			return count;
		}

		/* If line too long, then restart */
		if (c == '\n') {
			printf("smallshell :: input line is too long\n");
			count = 0;
			printf("%s ", p);
		}
	}
}

void procline() { /* Process input line */
	char *arg[MAXARG + 1];      /* Pointer array for runcommand   */
	char *filename;             /* File name of redirection       */
	int toktype;                /* Type of token in command       */
	int narg;                   /* Number of arguments so far     */
	int where;                  /* FOREGROUND or BACKGROUND ?     */
	int pipe_fd[2];             /* File descryptor for pipe       */
	int in = -1, out = -1;      /* File descryptor to be replaced */

	/* Reset intr flag */
	intr_p = 0;

	for (narg = 0;;) { /* loop until meet EOL */

		/* Take action according to token type */
		switch(toktype = gettok(&arg[narg])) {
		case ARG: 
			if (narg < MAXARG) {
				narg++;
			}
			break;
		
		/* Redirection ('<', '>') */
		case LESS:
			gettok(&filename);
			if ((in = open(filename, O_RDONLY)) == -1) {
				perror("smallshell");
				return;
			}
			break;
		case GREATER:
			gettok(&filename);
			if ((out = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0664)) == -1) {
				perror("smallshell");
				return;
			}
			break;

		/* Pipeline ('|') */
		case PIPELINE:
			if (pipe(pipe_fd) == -1) printf("pipe error\n");
			out = pipe_fd[1];

		/* End of command ('\n', ';', '&') */
		case EOL:
		case SEMICOLON:
		case AMPERSAND:
			/* Set process type (BG ? FG) */
			where = (toktype == AMPERSAND) ? BACKGROUND : FOREGROUND;

			/* Run command */
			if (narg != 0) {
				arg[narg] = NULL;
				runcommand(arg, where, in, out);
				
				/* If some files are opened, close it */
				if (in != -1) {
					close(in);
					in = -1;
				}
				if (out != -1) {
					close(out);
					out = -1;
				}
			}
			
			/* Return to shell prompt if the token is EOL('\n') */
			if (toktype == EOL) {
				return;
			}

			/* Pipelining */
			if (toktype == PIPELINE) {
				in = pipe_fd[0];
			}

			narg =0;
			break;
		}
	}
}
