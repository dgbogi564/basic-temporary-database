//////////////////////////////////////////////////////////////////////
//	filename: 	server.h
//
//	purpose:    Hosts a server that takes multiple connections and
//                      their commands to maintain a database for the
//                      duration of it's runtime.
/////////////////////////////////////////////////////////////////////


#ifndef P3_SERVER_H
#define P3_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include<string.h>

#include <sys/socket.h>
#include <netdb.h>

#ifndef REQUEST_HANDLING_INCLUDED
#define REQUEST_HANDLING_INCLUDED
extern __thread FILE *f_recv;
extern __thread FILE *f_send;
#endif

#ifndef HASH
#define HASH 10
#endif

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 128
#endif

#ifndef QUEUE_SIZE
#define QUEUE_SIZE 20
#endif

#include "macros.h"
#include "database.h"

typedef struct args {
	bool terminate;
	int connection;
	hashtable_ *database;
	socklen_t remote_addrlen;
	struct sockaddr_storage remote_addr;
} args_;

#endif //P3_SERVER_H