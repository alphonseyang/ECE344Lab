/*
 * client_simple.c: A very, very primitive HTTP client.
 * 
 * To run, try: 
 *      client www.ece.toronto.edu 80 /
 *      client www.google.ca 80 /
 *
 * Sends one HTTP request to the specified HTTP server.
 * Prints out the HTTP response.
 */

#include "common.h"

/* send an HTTP request for the specified file */
static void
client_send(int fd, char *host, char *filename)
{
	char buf[MAXLINE];

	/* form and send the HTTP request */
	sprintf(buf, "GET %s HTTP/1.0\r\n", filename);
	sprintf(buf, "%shost: %s\r\n\r\n", buf, host);
	Rio_write(fd, buf, strlen(buf));
}

/* read the HTTP response and print it out */
static void
client_print(int fd)
{
	struct rio *rio;
	char buf[MAXBUF];
	int n;
	int length = 0;
	int length_received = 0;
	
	rio = Rio_init(fd);

	/* read and display the HTTP header */
	n = Rio_readlineb(rio, buf, MAXBUF);
	while (strcmp(buf, "\r\n") && (n > 0)) {
		printf("Header: %s", buf);
		n = Rio_readlineb(rio, buf, MAXBUF);

		/* look for certain HTTP tags... */
		if (sscanf(buf, "Content-Length: %d ", &length) == 1) {
			/* found length tag */
		}
	}

	fflush(stdout);
	/* read and display the HTTP body */
	do {
		n = Rio_readlineb(rio, buf, MAXBUF);
		Rio_write(STDOUT_FILENO, buf, n);
		length_received += n;
	} while (n > 0);
		
	if (length != 0)
		assert(length == length_received);
	Rio_destroy(rio);
}

int
main(int argc, char *argv[])
{
	int i = 1;
	char *filename;
	int clientfd;
	char *host;
	int port;

	if (argc != 4) {
		fprintf(stderr, "Usage: %s host port filename\n", argv[0]);
		exit(1);
	}
	host = argv[i++];
	port = atoi(argv[i++]);
	filename = argv[i++];
	clientfd = open_clientfd(host, port);
	client_send(clientfd, host, filename);
	client_print(clientfd);
	SYS(close(clientfd));
	exit(0);
}
