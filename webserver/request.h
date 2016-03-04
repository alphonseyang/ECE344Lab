#ifndef __REQUEST_H__
#define __REQUEST_H__

struct file_data {
	char *file_name; /* name of file being requested */
	char *file_buf;	 /* file is read into this buffer in memory */
	int file_size;	 /* file size */
};

struct request *request_init(int connfd, struct file_data *data);
int request_readfile(struct request *rq);
void request_set_data(struct request *rq, struct file_data *data);
void request_sendfile(struct request *rq);
void request_destroy(struct request *rq);

#endif
