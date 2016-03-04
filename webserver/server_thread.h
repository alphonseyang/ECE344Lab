#ifndef __SERVER_THREAD_H__
#define __SERVER_THREAD_H__

struct server;

struct server *server_init(int nr_threads, int max_requests, 
			   int max_cache_size);
void server_request(struct server *sv, int connfd);

#endif /* __SERVER_THREAD_H__ */
