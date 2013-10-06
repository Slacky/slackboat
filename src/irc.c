#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "irc.h"
#include "ipc.h"
#include "io.h"

void irc_notice_event(char *sender, char *argument, char *content) {
	/* TODO: create savestate to load info at startup instead of compile-time */
	if(strstr(content, "*** Looking up your hostname") != NULL) {
		irc_send("NICK slackboat\r\n");
		irc_send("USER slackboat 8 * :Slack the Boat\r\n");
	}
	if(!strncmp(sender, "NickServ", 8) && strstr(content, "please choose a different nick") != NULL) {
		char out[256];
		memset(out, 0, 256);
		snprintf(out, 12 + strlen(PASSWORD), "IDENTIFY %s\r\n", PASSWORD);
		irc_privmsg("NickServ", out);
	}
}

void irc_welcome_event(void) {
	irc_join_channel("#pharmaceuticals");
}

void irc_privmsg_event(char *sender, char *argument, char *content) {
	if(!strncmp(content, ".", 1) && !strncmp(sender, "sasha", 5)) {
		char **argv, command[128], args[256];
		int argc;
		if(strstr(content, " ") != NULL) {
			argc = 1;
			sscanf(content, ".%127s %255[^\r\n]", command, args);
			for (int i = 0; args[i]; i++)
				argc += (args[i] == ' ');
		} else {
			argc = 0;
			sscanf(content, ".%127s[^\r\n]", command);
		}
		argv = (char **) malloc(sizeof(char *) * argc);
		if(argc > 0) {
			argv[0] = strtok(args, " ");
			for(int i = 1; i < argc; i++)
				argv[i] = strtok(NULL, " ");
		}
		if(strlen(command) > 0) {
			ipc_send(content);
		} else if(!strncmp(command, "load", 4) && argc > 0)
			init_module(strdup(argv[0]));
		free(argv);
	}
}

void irc_privmsg(const char *recipient, const char *message) {
	char out[256];
	memset(out, 0, 256);
	snprintf(out, 13 + strlen(recipient) + strlen(message), "PRIVMSG %s :%s\r\n", recipient, message);
	irc_send(out);
	return;
}

void irc_join_channel(const char *channel) {
	char out[BUFFER_SIZE];
	memset(out, 0, BUFFER_SIZE);
	snprintf(out, 8 + strlen(channel), "JOIN %s\r\n", channel);
	irc_send(out);
	return;
}
