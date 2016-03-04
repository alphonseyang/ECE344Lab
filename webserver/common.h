#ifndef __CSAPP_H__
#define __CSAPP_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

#define __STR(n) #n
#define STR(n) __STR(n)

#define TBD() do {							\
		printf("%s:%d: %s: please implement this functionality\n", \
		       __FILE__, __LINE__, __FUNCTION__);		\
		exit(1);						\
	} while (0)

/* use for system calls */
#define SYS(code)							\
	do {								\
		if ((code) < 0)	{					\
			fprintf(stderr, "%s: line %d: %s: %s\n",	\
				__FUNCTION__, __LINE__, STR(code),	\
				strerror(errno));			\
			exit(1);					\
		}							\
	} while (0)

/* use for gethostbyname/addr */
#define DNS(code)							\
	do {								\
		if ((code) == NULL) {					\
			fprintf(stderr, "%s: line %d: %s: DNS error %d\n", \
				__FUNCTION__, __LINE__, STR(code), h_errno); \
			exit(1);					\
		}							\
	} while (0)


/* Misc constants */
#define MAXLINE  8192	/* max text line length */
#define MAXBUF   8192	/* max I/O buffer size */
#define LISTENQ  1024	/* second argument to listen() */

/* Memory managment wrappers */
void *Malloc(size_t size);

/* Persistent state for the robust I/O (Rio) package */
struct rio;

struct rio *Rio_init(int fd);
void Rio_destroy(struct rio *rp);
ssize_t Rio_read(int fd, void *usrbuf, size_t n);
void Rio_write(int fd, void *usrbuf, size_t n);
ssize_t Rio_readlineb(struct rio *rp, void *usrbuf, size_t maxlen);

/* Wrappers for client/server helper functions */
int open_clientfd(char *hostname, int port);
int open_listenfd(int port);

/* Random functions */
void init_random();
double rand_pareto(double m, double a);
int rand_pareto_int(double m, double a);
double rand_self_similar(double a);
int rand_self_similar_int(double a, double high);

#endif /* __CSAPP_H__ */
