#include "smallshell.h"

static const char* prompt = "smallshell:";

int main() {
	/* sigaction struct (linux) */
	struct sigaction sa;
	sa.sa_handler = handle_int;
	sigfillset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	while (userin(prompt) != EOF) {
		sigaction(SIGINT, &sa, NULL);
		sigaction(SIGQUIT, &sa, NULL);
		sigaction(SIGTSTP, &sa, NULL);

		procline();
	}
	return 0;
}
