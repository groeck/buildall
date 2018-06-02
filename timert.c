#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

unsigned int start_time, stop_time;

int digits = 2;

static void print_time(void)
{
	int s = time(0) - start_time;
	int minutes = s / 60;
	int i;

	printf("\b\b\b\b\b");
	for (i = 0; i < digits; i++)
		printf("\b");

	printf("[%0*d:%02d]", digits, minutes, s % 60);

	minutes /= 100;
	digits = 2;
	while (minutes) {
	    digits++;
	    minutes /= 10;
	}
}

static void error(const char *s)
{
	perror(s);
	exit(1);
}

static void sigchld_handler(int signo, siginfo_t *si, void *uc)
{
//	fprintf(stderr, "code %d, status %d\n", si->si_code, si->si_status);
	switch (si->si_code) {
		case CLD_STOPPED:
			fprintf(stderr, "stopped\n");
			stop_time = time(0);
			break;
		case CLD_CONTINUED:
			fprintf(stderr, "continued\n");
			start_time += time(0) - stop_time;
			break;
		default:
			wait(0);
			print_time();
			if (si->si_status > 0xff)
				exit(si->si_status >> 8);
			else
				exit(si->si_status);
	}
}

static void init_signals(void)
{
	struct sigaction sa;

	sa.sa_sigaction = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	if (sigaction(SIGCHLD, &sa, 0) < 0)
		error("sigaction()");
}

int main(int argc, char **argv)
{
	int pid, fd;

	if (argc < 4) {
		fprintf(stderr, "Usage: %s <label> <logfile> <cmd> <args>\n",
		        argv[0]);
		return 1;
	}

	init_signals();

	setbuf(stdout, 0);

	start_time = time(0);

	printf(" %s        ", argv[1]);
	print_time();

	switch (pid = fork()) {
		case -1:
			error("fork()");

		case 0:
			fd = creat(argv[2], 0666);
			if (fd < 0)
				error("creat()");

			if (dup2(fd, 1) < 0)
				error("dup2()");

			if (dup2(fd, 2) < 0)
				error("dup2()");

			if (close(fd) < 0)
				error("close()");

			if (execvp(argv[3], argv+3))
				error("execvp()");

		default:
			for (;;) {
				print_time();
				sleep(1);
			}
	}

	return 0;
}
