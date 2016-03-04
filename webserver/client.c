/*
 * client.c: A very, very primitive HTTP client.
 * 
 * To run, try: 
 *      client www.cs.wisc.edu 80 /
 *
 * Sends one HTTP request to the specified HTTP server.
 * Prints out the HTTP response.
 *
 * For testing your server, you will want to modify this client.  
 * For example:
 * 
 * You may want to make this multi-threaded so that you can 
 * send many requests simultaneously to the server.
 *
 * You may also want to be able to request different URIs; 
 * you may want to get more URIs from the command line 
 * or read the list from a file. 
 *
 * When we test your server, we will be using modifications to this client.
 *
 */

#include "common.h"

/* send an HTTP request for the specified file */
static void
client_send(int fd, char *host, char *filename)
{
	char buf[MAXLINE];

	/* create the request line */
	sprintf(buf, "GET %s HTTP/1.0\r\n", filename);
	/* create one request header line for the server host, 
	   and then the empty line */
	sprintf(buf, "%shost: %s\r\n\r\n", buf, host);
	Rio_write(fd, buf, strlen(buf));
}

/* read the HTTP response and print it out */
static void
client_print(int fd, unsigned int orig_csum, int orig_length, int print)
{
	struct rio *rio;
	char buf[MAXBUF];
	int i, n;
	int length = 0;
	int length_received = 0;
	unsigned int csum = 0;
	unsigned int csum_received = 0;
	
	rio = Rio_init(fd);

	/* read and display the HTTP header */
	n = Rio_readlineb(rio, buf, MAXBUF);
	while (strcmp(buf, "\r\n") && (n > 0)) {
		if (print) {
			printf("Header: %s", buf);
		}
		n = Rio_readlineb(rio, buf, MAXBUF);

		/* look for certain HTTP tags... */
		if (sscanf(buf, "Content-Length: %d ", &length) == 1) {
			/* found length tag */
		}
		if (sscanf(buf, "Content-Csum: %u ", &csum) == 1) {
			/* found csum tag */
		}
	}

	fflush(stdout);
	/* read and display the HTTP body */
	do {
		n = Rio_readlineb(rio, buf, MAXBUF);
		if (print) {
			Rio_write(STDOUT_FILENO, buf, n);
		}
		length_received += n;
		for (i = 0; i < n; i++) {
			csum_received += (unsigned char)buf[i];
		}
	} while (n > 0);

	assert(orig_csum == csum);
	assert(orig_length == length);

	assert(length == length_received);
	assert(csum == csum_received);
	Rio_destroy(rio);
}

struct fileinfo {
	char *name;
	unsigned int csum;
	int len;
};

struct client {
	char *host;
	int port;
	int nr_times;
	int nr_threads;
	struct fileinfo *fileset;
	int nr_files;
	int timing_mode;
};

/* open a single connection to the specified host and port */
static void *
client_request(void *arg)
{
	struct client *cl = (struct client *)arg;
	int clientfd;
	int i;

	for (i = 0; i < cl->nr_times; i++) {
		int fnr;

		clientfd = open_clientfd(cl->host, cl->port);
		/* get a random file from the file set */
		fnr = rand_self_similar_int(0.2, cl->nr_files);
		fnr--;
		/* for debugging */
		// fprintf(stderr, "requesting file: %s\n", 
		// cl->fileset[fnr].name);
		client_send(clientfd, cl->host, cl->fileset[fnr].name);
		/* when timing_mode is 1, then don't print anything */
		client_print(clientfd, cl->fileset[fnr].csum, 
			     cl->fileset[fnr].len, (cl->timing_mode == 0));
		SYS(close(clientfd));
	}
	return NULL;
}

static void
usage(char *program)
{
	fprintf(stderr, "Usage: %s [-t] host port nr_times nr_threads fileset\n",
		program);
	exit(1);
}

/* filename should have a list of files to be requested, one per line */
static void
init_fileset(char *filename, struct client *cl)
{
	int i, n;
	int fd;
	struct rio *rio;
	char buf[MAXLINE];

	/* read the index file for the fileset */
	SYS(fd = open(filename, O_RDONLY, 0));
	rio = Rio_init(fd);
	i = 0;
	while (1) {
		struct fileinfo *fi;
		n = Rio_readlineb(rio, buf, MAXLINE);
		if (n == 0) {
			assert(cl->nr_files > 0);
			assert(i == cl->nr_files);
			break;
		}
		if (buf[n - 1] == '\n') {
			n--;
		}
		if (cl->nr_files == 0) {
			cl->nr_files = atoi(buf);
			assert(cl->nr_files > 0);
			cl->fileset = Malloc(sizeof(struct fileinfo) * 
					     cl->nr_files);
			continue;
		}
		assert(i < cl->nr_files);
		fi = &cl->fileset[i];
		fi->name = Malloc(n + 1);
		sscanf(buf, "%s %u %d", fi->name, &fi->csum, &fi->len);
		i++;
	}
	Rio_destroy(rio);
	SYS(close(fd));
}

int
main(int argc, char *argv[])
{
	int i;
	char *filename;
	pthread_t *threads;
	struct client cl;
	struct timeval start, end, diff;

	if (argc != 6 && argc != 7) {
		usage(argv[0]);
	}
	i = 1;
	if (strcmp(argv[i], "-t") == 0) {
		cl.timing_mode = 1;
		i++;
	} else {
		cl.timing_mode = 0;
	}
	cl.host = argv[i++];
	cl.port = atoi(argv[i++]);
	cl.nr_times = atoi(argv[i++]);
	cl.nr_threads = atoi(argv[i++]);
	cl.nr_files = 0;
	filename = argv[i++];
	if (cl.port < 1024 || cl.nr_times <= 0 || cl.nr_threads <= 0) {
		usage(argv[0]);
	}

	init_fileset(filename, &cl);

	if (cl.timing_mode)
		gettimeofday(&start, NULL);

	init_random();

	threads = Malloc(sizeof(pthread_t) * cl.nr_threads);
	for (i = 0; i < cl.nr_threads; i++) {
		SYS(pthread_create(&threads[i], NULL, client_request,
				   (void *)&cl));
	}
	for (i = 0; i < cl.nr_threads; i++) {
		pthread_join(threads[i], NULL);
	}

	if (cl.timing_mode) {
		gettimeofday(&end, NULL);
		timersub(&end, &start, &diff);
		printf("client runtime = %.6f seconds\n",
			(float)diff.tv_sec + (float)diff.tv_usec / 1000000);
	}
	exit(0);
}
