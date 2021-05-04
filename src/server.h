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

#include "macros.h"
#include "database.h"

#ifndef HASH
#define HASH 10
#endif

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 128
#endif

#ifndef QUEUE_SIZE
#define QUEUE_SIZE 20
#endif

typedef struct args {
	int connection;
	hashtable_ *database;
	socklen_t remote_addrlen;
	struct sockaddr_storage remote_addr;
} args_;

args_* args_init( int connection,
                  hashtable_ *database,
                  struct sockaddr_storage remote_addr,
		          socklen_t remote_addrlen)
{
	args_ *args = safe_malloc(__func__, sizeof(args_));
	args->connection = connection;
	args->database = database;
	args->remote_addrlen = remote_addrlen;
	args->remote_addr = remote_addr;

	return args;
}

__thread FILE *f_recv = NULL;
__thread FILE *f_send = NULL;

#endif //P3_SERVER_H