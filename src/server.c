#include "server.h"

/* ================== INITIALIZE LOCAL THREAD VARIABLES ================= */

__thread FILE *f_recv = NULL;
__thread FILE *f_send = NULL;

/* ================================ ARGS ================================ */


args_* args_init( int connection,
                  hashtable_ *database,
                  struct sockaddr_storage remote_addr,
                  socklen_t remote_addrlen)
{
	args_ *args = safe_malloc(__func__, sizeof(args_));
	args->terminate = 0;
	args->connection = connection;
	args->database = database;
	args->remote_addrlen = remote_addrlen;
	args->remote_addr = remote_addr;

	return args;
}


/* =========================== MAIN FUNCTIONS =========================== */


int parse_payload(char **key, char **data) {
	// Parse length of the payload
	int i, j;
	int len = 0;
	char c = getc(f_recv);

	while ('0' <= c && '9' >= c) {
		len *= 10;
		len += c - '0';
		c = getc(f_recv);
	}
	if(len == 0) return 1; // malformed request

	*key = safe_malloc(__func__, (sizeof(char)*len)+2);
	for (i = 0; i < len; ++i) {
		c = getc(f_recv);
		if(c == '\n') break;
		else if ('a' <= c && 'z' <= c || 'A' <= c && 'Z' <= c) {
			*key[i] = c;
			*key[i+1] = '\0';
		} else {
			//malformed request
			return 1;
		}
	}
	i++;
	*data = safe_malloc(__func__, (sizeof(char)*len-i)+2);
	for(j = 0; j+i < len; ++j) {
		c = getc(f_recv);
		if ('a' <= c && 'z' <= c || 'A' <= c && 'Z' <= c) {
			*data[j] = c;
			*data[j+1] = '\0';
		} else if(c == '\n') {
			// wrong length
			return 2;
		} else {
			//malformed request
			return 1;
		}
	}

	// wrong length
	if(c != '\n') return 2;

	return 0;
}

void request_handler(void *vargs) {
	args_ *args = vargs;
	int connection = args->connection;
	hashtable_ *database = args->database;
	int error;
	char *key, *data;
	char c;

	// Copies socket and opens orig and copy in read and write mode respectively
	 f_recv = fdopen(dup(connection), "r");
	 f_send = fdopen(connection, "w");

	do {
		c = getc(f_recv);
		if (c != EOF) {
			if (           c == 'G' &&
			    getc(f_recv) == 'E' &&
			    getc(f_recv) == 'T' &&
			    getc(f_recv) == '\n')
			{
				error = parse_payload(&key, &data);
				if(error) {
					if(error == 1) {
						fprintf(f_send, "ERR BAD\n");
					} else {
						fprintf(f_send, "ERR LEN\n");
					}
				}
				// GET data from key
				data = getData(database, key);
				if (data == NULL) {
					// Key doesn't exist
					fprintf(f_send,"KNF\n");
				} else {
					// Success
					fprintf(f_send, "OKG\n%lu\n%s\n", strlen(data), data);
				}
			}
			else if (             c == 'S' &&
			           getc(f_recv) == 'E' &&
			           getc(f_recv) == 'T' &&
			           getc(f_recv) == '\n')
			{
				error = parse_payload(&key, &data);
				if(error) {
					if(error == 1) {
						fprintf(f_send, "ERR BAD\n");
					} else {
						fprintf(f_send, "ERR LEN\n");
					}
				}
				// TODO whats the format for parsing values?
				// SET key to data
				// TODO insertData error checking?
				insertData(database, key, data);
				fprintf(f_send, "OKS\n");
			}
			else if (             c == 'D' &&
			           getc(f_recv) == 'E' &&
			           getc(f_recv) == 'L' &&
			           getc(f_recv) == '\n')
			{
				error = parse_payload(&key, &data);
				if(error) {
					if(error == 1) {
						// Malformed request
						fprintf(f_send, "ERR BAD\n");
					} else {
						// Wrong length
						fprintf(f_send, "ERR LEN\n");
					}
				}
				// TODO DEL key
				data = delData(database, key);
				if(data == NULL) {
					// Key doesn't exist
					fprintf(f_send,"KNF\n");
				} else {
					// Success
					fprintf(f_send, "OKD\n%lu\n%s\n", strlen(data), data);
				}
			}
			else {
				// Malformed request
				fprintf(f_send, "ERR BAD\n");
			}
		}
	} while (!args->terminate);

	fclose(f_recv);
	fclose(f_send);
	free(args);
}


/* =============================== DRIVER =============================== */


int main(int argc, char *argv[]) {
	if (argc != 2) {
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
	volatile bool run = 1;

	// Provide additional info about what type of connection we want to open
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;        // Use IPv4 or IPv6 addresses
	hints.ai_socktype = SOCK_STREAM;    // Create a streaming socket
	hints.ai_flags = AI_PASSIVE;        // Listen for incoming connection

	// Get port information
	error = getaddrinfo(NULL, argv[1], &hints, &info);
	if (error != 0) {
		eprintf("%s\n", gai_strerror(error));
		exit(EXIT_FAILURE);
	}

	// Creates listener socket
	listener = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (listener < 0 || connect(listener, info->ai_addr, info->ai_addrlen) == 0) {
		eprintf("Failed to setup listener port\n");
		exit(EXIT_FAILURE);
	}

	// Binds the listener socket to the port
	error = bind(listener, info->ai_addr, info->ai_addrlen);
	if (error != 0) {
		eprintf("Failed to bind port\n");
		exit(EXIT_FAILURE);
	}

	// Create queue for accepting incoming requests
	error = listen(listener, QUEUE_SIZE);
	if (error != 0) {
		eprintf("Failed to initialize connection queue\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(info);

	// Initialize database
	hashtable_ *database = hashtable_init();

	// TODO finish args struct in server.h
	args_ *args;
	unsigned long worker_id;
	while (run) { //TODO EXTRA handle SIGs
		// Wait for incoming requests
		connection = accept(listener, (struct sockaddr *) &remote_addr, &remote_addrlen);
		// TODO Should we do any extra errorhandling?
		// Failed to accept incoming connection
		if (connection < 0) continue;

		// TODO Use getnameinfo() to convert remote_addr back to human-readable strings
		args = args_init(connection, database, remote_addr, remote_addrlen);
		// TODO Setup and pass required args

		// Start thread
		// TODO do we need to pass worker_id to the worker thread?
		error = pthread_create(&worker_id, NULL, (void *) request_handler, (void *) args);
		if (error) {
			eprintf("main(): Failed to create pthread\n");
			exit(EXIT_FAILURE);
		}

		// Will not wait for threads to finish
		pthread_detach(worker_id);
	}

	hashtable_destroy(database);
	return EXIT_SUCCESS;
}