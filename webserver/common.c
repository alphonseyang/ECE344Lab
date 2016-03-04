#include "common.h"

/************************** 
 * Error-handling functions
 **************************/
void
unix_error(char *msg)
{	/* unix-style error */
	fprintf(stderr, "%s: %s\n", msg, strerror(errno));
	exit(1);
}

/*********************************************
 * Wrappers for memory management functions
 ********************************************/
void *
Malloc(size_t size)
{
	void *rc;
	rc = malloc(size);
	if (!rc) {
		unix_error("malloc");
	}
	return rc;
}

/*********************************************************************
 * The Rio package - robust I/O functions
 **********************************************************************/

/* Persistent state for the robust I/O (Rio) package */

#define RIO_BUFSIZE 8192

struct rio {
	int rio_fd;	/* descriptor for this internal buf */
	int rio_cnt;	/* unread bytes in internal buf */
	char *rio_bufptr;	/* next unread byte in internal buf */
	char rio_buf[RIO_BUFSIZE];	/* internal buffer */
};

/*
 * rio_init - Associate a descriptor with a read buffer and reset buffer
 */
static struct rio *
rio_init(int fd)
{
	struct rio *rp = malloc(sizeof(struct rio));
	if (rp) {
		rp->rio_fd = fd;
		rp->rio_cnt = 0;
		rp->rio_bufptr = rp->rio_buf;
	}
	return rp;
}

static void
rio_destroy(struct rio *rp)
{
	free(rp);
}

/* rio_read - robustly read n bytes (unbuffered) */
static ssize_t
rio_read(int fd, void *usrbuf, size_t n)
{
	size_t nleft = n;
	ssize_t nread;
	char *bufp = usrbuf;

	while (nleft > 0) {
		if ((nread = read(fd, bufp, nleft)) < 0) {
			if (errno == EINTR)	/* interrupted by sig handler return */
				nread = 0;	/* and call read() again */
			else
				return -1;	/* errno set by read() */
		} else if (nread == 0)
			break;	/* EOF */
		nleft -= nread;
		bufp += nread;
	}
	return (n - nleft);	/* return >= 0 */
}

/* rio_write - robustly write n bytes (unbuffered) */
static ssize_t
rio_write(int fd, void *usrbuf, size_t n)
{
	size_t nleft = n;
	ssize_t nwritten;
	char *bufp = usrbuf;

	while (nleft > 0) {
		if ((nwritten = write(fd, bufp, nleft)) <= 0) {
			if (errno == EINTR)	/* interrupted by sig handler return */
				nwritten = 0;	/* and call write() again */
			else
				return -1;	/* errorno set by write() */
		}
		nleft -= nwritten;
		bufp += nwritten;
	}
	return n;
}

/* 
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
static ssize_t
rio_readb(struct rio *rp, char *usrbuf, size_t n)
{
	int cnt;

	while (rp->rio_cnt <= 0) {	/* refill if buf is empty */
		rp->rio_cnt = read(rp->rio_fd, rp->rio_buf,
				   sizeof(rp->rio_buf));
		if (rp->rio_cnt < 0) {
			if (errno != EINTR)	/* interrupted by sig handler return */
				return -1;
		} else if (rp->rio_cnt == 0)	/* EOF */
			return 0;
		else
			rp->rio_bufptr = rp->rio_buf;	/* reset buffer ptr */
	}

	/* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
	cnt = n;
	if (rp->rio_cnt < n)
		cnt = rp->rio_cnt;
	memcpy(usrbuf, rp->rio_bufptr, cnt);
	rp->rio_bufptr += cnt;
	rp->rio_cnt -= cnt;
	return cnt;
}

/* rio_readlineb - robustly read a text line (buffered) */
static ssize_t
rio_readlineb(struct rio *rp, void *usrbuf, size_t maxlen)
{
	int n, rc;
	char c, *bufp = usrbuf;

	for (n = 0; n < maxlen; n++) {
		if ((rc = rio_readb(rp, &c, 1)) == 1) {
			*bufp++ = c;
			if (c == '\n') {
				n++;
				break;
			}
		} else if (rc == 0) {
			if (n == 0)
				return 0;	/* EOF, no data read */
			else
				break;	/* EOF, some data was read */
		} else
			return -1;	/* error */
	}
	*bufp = 0;
	return n;
}

/**********************************
 * Wrappers for robust I/O routines
 **********************************/
ssize_t
Rio_read(int fd, void *ptr, size_t nbytes)
{
	ssize_t n;

	if ((n = rio_read(fd, ptr, nbytes)) < 0)
		unix_error("Rio_readn error");
	return n;
}

void
Rio_write(int fd, void *usrbuf, size_t n)
{
	if (rio_write(fd, usrbuf, n) != n)
		unix_error("Rio_writen error");
}

struct rio *
Rio_init(int fd)
{
	struct rio *rp = rio_init(fd);
	if (!rp)
		unix_error("Rio_init error");
	return rp;
}

void
Rio_destroy(struct rio *rp)
{
	rio_destroy(rp);
}

ssize_t
Rio_readlineb(struct rio * rp, void *usrbuf, size_t maxlen)
{
	ssize_t rc;

	if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
		unix_error("Rio_readlineb error");
	return rc;
}

/******************************** 
 * Client/server helper functions
 ********************************/
/* open connection to server at <hostname, port> and return a socket descriptor
 * ready for reading and writing. */
int
open_clientfd(char *hostname, int port)
{
	int clientfd;
	struct hostent *hp;
	struct sockaddr_in serveraddr;

	SYS(clientfd = socket(AF_INET, SOCK_STREAM, 0));

	/* Fill in the server's IP address and port */
	DNS(hp = gethostbyname(hostname));

	bzero((char *)&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *)hp->h_addr,
	      (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
	serveraddr.sin_port = htons(port);

	/* Establish a connection with the server */
	SYS(connect(clientfd, (struct sockaddr *)&serveraddr,
		    sizeof(serveraddr)));
	return clientfd;
}

/* open and return a listening socket on port */
int
open_listenfd(int port)
{
	int listenfd, optval = 1;
	struct sockaddr_in serveraddr;

	/* Create a socket descriptor */
	SYS(listenfd = socket(AF_INET, SOCK_STREAM, 0));

	/* Eliminates "Address already in use" error from bind. */
	SYS(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
		       (const void *)&optval, sizeof(int)));

	/* Listenfd will be an endpoint for all requests to port
	   on any IP address for this host */
	bzero((char *)&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short)port);
	SYS(bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)));

	/* Make it a listening socket ready to accept connection requests */
	SYS(listen(listenfd, LISTENQ));

	return listenfd;
}

/*********************************************************
 * Functions for generating long-tail random distributions
 *********************************************************/

#define RAND ((double)random())/RAND_MAX

void
init_random()
{
	int fd = open("/dev/urandom", O_RDONLY);
	int seed;

	if (fd < 0) {
		fprintf(stderr, "couldn't open /dev/random\n");
		exit(1);
	}
	if (read(fd, &seed, sizeof(int)) < sizeof(int)) {
		fprintf(stderr, "couldn't read /dev/random\n");
		exit(1);
	}
	srandom(seed);
}

 /* m: minimum value */
double
rand_pareto(double m, double a)
{
	double r = RAND;

	while (r <= 0 || r >= 1)
		r = RAND;

	return m * pow(r, -1 / a);
}

int
rand_pareto_int(double m, double a)
{
	return round(rand_pareto(m, a));
}

/*
 * Input: 0 < a < 1 
 * Return value: > 0 and <= 1
 *
 * The fraction of the values that will lie between 0 and a is (1-a).
 * E.g., 80% of the values generated will be between 0 and 0.2 when a = 0.2
 * Obtained from: http://research.microsoft.com/pubs/68246/syntheticdatagen.pdf
 *
 */
double
rand_self_similar(double a)
{
	double r;
	double exp;
	double ret;

	assert(a > 0 && a < 1);
	do {
		r = RAND;
	} while (r <= 0);
	exp = log(a) / log(1 - a);
	ret = pow(r, exp);
	assert(ret > 0 && ret <= 1);
	return ret;
}

/* return value: >= 1 and <= high */
int
rand_self_similar_int(double a, double high)
{
	int ret = ceil(high * rand_self_similar(a));
	assert(ret >= 1 && ret <= high);
	return ret;
}
