#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include "ipc.h"
#include "io.h"
#include "irc.h"
#include "util.h"

int pipe_fd[2];
int ipc_index = 0;
char *ipc_names[256];

void init_ipc(void) {
	pthread_t ipc_thread;
	pthread_create(&ipc_thread, NULL, handle_ipc_calls, NULL);

	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = child_handler;
	sigaction(SIGCHLD, &sa, NULL);
}

void init_module(char *name) {
	if(pipe(pipe_fd)) {
		perror("Error creating IPC pipes");
		exit(1);
	}

	pid_t child = fork();

	if(child < 0) {
		perror("Error starting module");
		return;
	}

	/* TODO: Handle errors starting modules without killing IRC I/O */

	if(!child) {
		ipc_names[ipc_index++] = name;
		char s[256];
		snprintf(s, 255, "Module ‘%s’ starting...", name);
		irc_privmsg("#pharmaceuticals", s);
		strprepend(name, "./");
		close(1);
		if(dup(pipe_fd[1]) < 0) {
			perror("Error starting module.");
			exit(1);
		}
		close(pipe_fd[0]);
		execl(name, name, NULL);
	}

	return;
}

int ipc_read(char *in_buffer) {
	ssize_t nread = 0;
	size_t tread = 0;
	char c;

	if (in_buffer == NULL) {
		errno = EINVAL;
		return -1;
	}

	for (;;) {
		nread = read(pipe_fd[0], &c, 1);

		if (nread == -1) {
			if (errno == EINTR)
				continue;
			else
				return -1;
		} else if (nread == 0) {
			if (tread == 0)
				return 0;
			else
				break;

		} else {
			if (tread < BUFFER_SIZE - 1) {
				tread++;
				*in_buffer++ = c;
			}

			if (c == '\n' || c == EOF)
				break;
		}
	}

	*in_buffer = '\0';
	return tread;
}

void *handle_ipc_calls() {
	for(;;) {
		char in_buffer[BUFFER_SIZE];
		int tread = ipc_read(in_buffer);
		if(tread > 0) {
			if(DEBUG)
				printf("IPC IN: %s", in_buffer);
			char msg[256];
			snprintf(msg, 255, "From a running module: %s", in_buffer);
			irc_privmsg("#pharmaceuticals", msg);
		}
		usleep(50 * 1000);
	}

	return NULL;
}

void child_handler(int sig) {
	pid_t pid;
	int status;

	while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		char *name = ipc_names[--ipc_index];
		char s[256];
		snprintf(s, 255, "Module ‘%s’ has stopped.", name);
		irc_privmsg("#pharmaceuticals", s);
	}
}