#include "server.h"

void request_handler(void *vargs)
{
	args_ *args = vargs;
	int connection = args->connection;
	int offset = 0, num = 0;
	char *buffer = NULL;
	char c, cToStr[2];

	// Copies socket and opens orig and copy in read and write mode respectively
	FILE *f_recv = fdopen(dup(connection), "r");
	FILE *f_send = fdopen(connection, "w");

	do {
		c = getc(f_recv);
		if(c != EOF) {
			if (            c == 'G' &&
				 getc(f_recv) == 'E' &&
				 getc(f_recv) == 'T' &&
				 getc(f_recv) == '\n')
			{
				// Parse length
				c = getc(f_recv);
				while( '0' <= c && '9' >= c) {
					num *= 10;
					num += c - '0';
					c = getc(f_recv);
				}

			} else if (            c == 'S' &&
						getc(f_recv) == 'E' &&
					    getc(f_recv) == 'T' &&
					    getc(f_recv) == '\n')
			{

			} else if (            c == 'D' &&
						getc(f_recv) == 'E' &&
						getc(f_recv) == 'L' &&
						getc(f_recv) == '\n')
			{

			} else {
				// malformed request
				fprintf(f_send, "ERR BAD");
			}
		}
	} while(c != EOF);


	fclose(f_recv);
	fclose(f_send);
	free(args);
}

int main(int argc, char *argv[])
{
	if(argc != 2) {
		// Use ports 5000-65536
		printf("Usage: %s [port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	struct addrinfo hints, *info;
	struct sockaddr_storage remote_addr;
	socklen_t remote_addrlen;
	int listener;
	int connection;
	int error;
	volatile bool run;

	// Provide additional info about what type of connection we want to open
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;        // Use IPv4 or IPv6 addresses
	hints.ai_socktype = SOCK_STREAM;    // Create a streaming socket
	hints.ai_flags = AI_PASSIVE;        // Listen for incoming connection

	// Get port information
	error = getaddrinfo(NULL, argv[1], &hints, &info);
	if(error != 0) {
		eprintf("%s\n", gai_strerror(error));
		exit(EXIT_FAILURE);
	}

	// Creates listener socket
	listener = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if(listener < 0 || connect(listener, info->ai_addr, info->ai_addrlen) == 0) {
		eprintf("Failed to setup listener port\n");
		exit(EXIT_FAILURE);
	}

	// Binds the listener socket to the port
	error = bind(listener, info->ai_addr, info->ai_addrlen);
	if(error != 0) {
		eprintf("Failed to bind port\n");
		exit(EXIT_FAILURE);
	}

	// Create queue for accepting incoming requests
	error = listen(listener, QUEUE_SIZE);
	if(error != 0) {
		eprintf("Failed to initialize connection queue\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(info);

	// Initialize database
	hashtable_ *database = hashtable_init();

	// TODO finish args struct in server.h
	args_ *args;
	unsigned long worker_id;
	while(run) { //TODO EXTRA handle SIGs
		// Wait for incoming requests
		connection = accept(listener, (struct sockaddr *) &remote_addr, &remote_addrlen);
		// TODO Should we do any extra errorhandling?
		// Failed to accept incoming connection
		if(connection < 0) continue;

		// TODO Use getnameinfo() to convert remote_addr back to human-readable strings
		args = args_init(connection, database, remote_addr, remote_addrlen);
		// TODO Setup and pass required args

		// Start thread
		// TODO do we need to pass worker_id to the worker thread?
		error = pthread_create(&worker_id, NULL, (void *)request_handler, (void *)args);
		if(error) {
			eprintf("main(): Failed to create pthread\n");
			exit(EXIT_FAILURE);
		}

		// Will not wait for threads to finish
		pthread_detach(worker_id);
	}

	hashtable_destroy(database);
	return EXIT_SUCCESS;
}