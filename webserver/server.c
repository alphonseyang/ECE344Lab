#include "common.h"
#include "request.h"
#include "server_thread.h"

/* 
 * server.c: A very, very simple web server
 *
 * To run:
 *  server portnum nr_threads max_requests max_cache_size
 *
 * Repeatedly handles HTTP requests sent to this port number. Most of the work
 * is done within routines written in server_thread.c and request.c
 */

void
usage(char *program)
{
	fprintf(stderr, "Usage: %s port nr_threads max_requests "
		"max_cache_size\n", program);
	exit(1);
}

int
main(int argc, char *argv[])
{
	int port, nr_threads, max_requests, max_cache_size;
	int listenfd, connfd, clientlen;
	struct sockaddr_in clientaddr;
	struct server *sv;

	if (argc != 5)
		usage(argv[0]);
	port = atoi(argv[1]);
	nr_threads = atoi(argv[2]);
	max_requests = atoi(argv[3]);
	max_cache_size = atoi(argv[4]);
	if (port < 1024) {
		fprintf(stderr, "port = %d, should be >= 1024\n", port);
		usage(argv[0]);
	}
	if (nr_threads < 0 || max_requests < 0 || max_cache_size < 0) {
		fprintf(stderr, "arguments should be > 0\n");
		usage(argv[0]);
	}

	sv = server_init(nr_threads, max_requests, max_cache_size);

	listenfd = open_listenfd(port);
	while (1) {
		clientlen = sizeof(clientaddr);
		SYS(connfd = accept(listenfd, (struct sockaddr *)&clientaddr,
				    (socklen_t *) & clientlen));

		/* serve the request */
		server_request(sv, connfd);
	}
	exit(0);
}
