#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netdb.h>

#include<string.h>


#include "macros.h"
#include "database.h"



int main(int argc, char *argv[])
{
	if(argc != 2) {
		printf("Usage: %s [port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	struct addrinfo hints, *info;
	struct sockaddr_storage remote_addr;
	socklen_t remote_addrlen;
	int listener;
	int connection;
	int error;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	error = getaddrinfo(NULL, argv[1], &hints, &info);
	if(error != 0) {
		eprintf("%s\n", gai_strerror(error));
		exit(EXIT_FAILURE);
	}

	listener = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if(listener < 0 || connect(listener, info->ai_addr, info->ai_addrlen) == 0) {
		eprintf("Failed to setup listener port\n");
		exit(EXIT_FAILURE);
	}

	error = bind(listener, info->ai_addr, info->ai_addrlen);
	if(error != 0) {
		eprintf("Failed to bind port\n");
		exit(EXIT_FAILURE);
	}

	error = listen(listener, QUEUESIZE);
	if(error != 0) {
		eprintf("Failed to initialize connection queue\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(info);

	while(1) {
		connection = accept(listener, (struct sockaddr *) &remote_addr, &remote_addrlen);
		// TODO Should we do any extra errorhandling?
		// Failed to accept incoming connection.
		if(connection < 0) continue;

		// TODO
		//  Use getnameinfo() to convert remote_addr back to human-readable strings
		//  Setup required info and setup threads here
	}

	// TODO use getnameinfo() to convert remote_addr back to human-readable strings

	return EXIT_SUCCESS;
}