#include "smallshell.h"

static const char* prompt = "Command>"; //"smallshell:";

int main() {
	/* sigaction struct (linux) */
	struct sigaction sa;
	sa.sa_handler = handle_int;
	sigfillset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	/* Cauht signal (^C, ^\, ^Z) */
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGTSTP, &sa, NULL);

	/* Get command */
	while (userin(prompt) != EOF) {
		procline();
	}
	return 0;
}
