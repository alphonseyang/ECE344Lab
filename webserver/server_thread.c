#include "request.h"
#include "server_thread.h"
#include "common.h"
#include <pthread.h>

struct entry {
    int count;
    struct file_data *data;
    struct entry *next;
};

struct hashtable {
    int size;
    int cache_size;
    int available_cache_size;
    pthread_mutex_t *ht_lock;
    struct entry *LRU;
    struct entry **table; //a array of the entry,table[i], i will be the hash number
};

unsigned long hash(char* str) {
    unsigned long hash = 5381;
    int c = *str;
    while (c) {
        hash = ((hash << 5) + hash) + c;
        str++;
        c = *str;
    }
    return hash % 10000000;
}

struct hashtable *ht;

struct server {
    int nr_threads;
    int max_requests;
    int max_cache_size;
    pthread_cond_t *cv_full;
    pthread_cond_t *cv_empty;
    pthread_mutex_t *lock;
    int *buf;
    int in;
    int out;
};

/* static functions */

/* initialize file data */
static struct file_data *
file_data_init(void) {
    struct file_data *data;

    data = Malloc(sizeof (struct file_data));
    data->file_name = NULL;
    data->file_buf = NULL;
    data->file_size = 0;
    return data;
}

/* free all file data */
static void
file_data_free(struct file_data *data) {
    free(data->file_name);
    free(data->file_buf);
    free(data);
}

void update_LRU(struct file_data *file) {
    if (!ht->LRU) { //LRU is empty
        struct entry *new_entry = malloc(sizeof (struct entry));
        new_entry->data = file;
        new_entry->next = NULL;
        new_entry->count = 0;
        ht->LRU = new_entry;
    } else if (!ht->LRU->next) { //only one element in LRU
        if (ht->LRU->data != file) {
            struct entry *new_entry = malloc(sizeof (struct entry));
            new_entry->data = file;
            new_entry->next = NULL;
            new_entry->count = 0;
            ht->LRU->next = new_entry;
        }
    } else {
        struct entry *temp = ht->LRU;
        struct entry *prev = ht->LRU;
        while (temp) {
            if (temp->data == file) //find the same in the LRU already
                break;
            prev = temp;
            temp = temp->next;
        }
        if (temp) { //if find the same entry
            if (temp->data == ht->LRU->data) {
                ht->LRU = ht->LRU->next;
                temp->next = NULL;
                struct entry *temp2 = ht->LRU;
                while (temp2) {
                    prev = temp2;
                    temp2 = temp2->next;
                }
                prev->next = temp;
            } else {
                prev->next = temp->next;
                temp->next = NULL;
                while (prev->next)
                    prev = prev->next;
                prev->next = temp;
            }
        } else {
            struct entry *new_entry = malloc(sizeof (struct entry));
            new_entry->data = file;
            new_entry->next = NULL;
            new_entry->count = 0;
            prev->next = new_entry;
        }
    }
}

struct file_data *cache_lookup(struct file_data *file, int time) {
    unsigned long hash_num = hash(file->file_name);
    if (!ht->table[hash_num]) {
        return NULL;
    }
    struct entry *temp = ht->table[hash_num];
    while (temp) {
        if (strcmp(temp->data->file_name, file->file_name) == 0) {
            if (!time)
                update_LRU(temp->data);
            return temp->data;
        }
        temp = temp->next;
    }
    return NULL;
}

void cache_insert_ht(struct file_data *file) {
    unsigned long hash_num = hash(file->file_name);
    struct entry *new_entry = malloc(sizeof (struct entry));
    new_entry->data = file;
    new_entry->next = NULL;
    new_entry->count = 0;
    if (ht->table[hash_num] == NULL) {
        ht->table[hash_num] = new_entry;
        ht->available_cache_size -= file->file_size;
    } else {
        struct entry *temp = ht->table[hash_num];
        while (temp->next)
            temp = temp->next;
        temp->next = new_entry;
        ht->available_cache_size -= file->file_size;
    }
    update_LRU(file);
}

void delete_entry(struct file_data *file) {
    unsigned long hash_num = hash(file->file_name);
    struct entry *temp = ht->table[hash_num];
    struct entry *prev = ht->table[hash_num];
    if (!temp->next) {
        free(temp);
        ht->table[hash_num] = NULL;
        return;
    }
    while (temp->data != file) {
        prev = temp;
        temp = temp->next;
    }
    prev->next = temp->next;
    free(temp);
}

void cache_evict(int size_to_evict) {
    while (ht->available_cache_size < size_to_evict) {
        struct entry *to_evict = ht->LRU;
        ht->available_cache_size += to_evict->data->file_size;
        ht->LRU = ht->LRU->next;
        delete_entry(to_evict->data);
        free(to_evict);
    }
}

void cache_insert(struct file_data *file) {

    if (file->file_size > 0.5*ht->cache_size) {
        return;
    } else if (file->file_size > ht->available_cache_size) {
        cache_evict(file->file_size);
        cache_insert_ht(file);
    } else {
        cache_insert_ht(file);
    }

}

static void
do_server_request(struct server *sv, int connfd) {
    struct request *rq;
    struct file_data *data;

    data = file_data_init();

    /* fills data->file_name with name of the file being requested */
    rq = request_init(connfd, data);
    if (!rq) {
        file_data_free(data);
        return;
    }
    struct file_data *cached_data = NULL;
    if (sv->max_cache_size > 0) {
        pthread_mutex_lock(ht->ht_lock);
        cached_data = cache_lookup(data, 0);
        pthread_mutex_unlock(ht->ht_lock);
    }
    if (cached_data) {
        request_set_data(rq, cached_data);
        request_sendfile(rq);
    } else {
        request_readfile(rq);
        request_sendfile(rq);
        if (sv->max_cache_size > 0) {
            pthread_mutex_lock(ht->ht_lock);
            struct file_data *returned_data = cache_lookup(data, 1);
            if (!returned_data) {
                cache_insert(data);
            }
            pthread_mutex_unlock(ht->ht_lock);
        }
    }
    /* reads file, 
     * fills data->file_buf with the file contents,
     * data->file_size with file size. */

    /* sends file to client */
    request_destroy(rq);
}

/* entry point functions */

void *stub_func(void* sv) {
    struct server *request_sv = (struct server *) sv;
    while (1) {
        pthread_mutex_lock(request_sv->lock);
        while (request_sv->in == request_sv->out) {
            pthread_cond_wait(request_sv->cv_empty, request_sv->lock);
        }
        int msg = request_sv->buf[request_sv->out];
        if ((request_sv->in - request_sv->out + (request_sv->max_requests + 1)) % (request_sv->max_requests + 1) == request_sv->max_requests)
            pthread_cond_signal(request_sv->cv_full);
        request_sv->out = (request_sv->out + 1) % (request_sv->max_requests + 1);
        pthread_mutex_unlock(request_sv->lock);
        do_server_request(request_sv, msg);
    }
    return NULL;
}

struct server *
server_init(int nr_threads, int max_requests, int max_cache_size) {
    struct server *sv;

    sv = Malloc(sizeof (struct server));
    sv->nr_threads = nr_threads;
    sv->max_requests = max_requests;
    sv->max_cache_size = max_cache_size;

    if (nr_threads > 0 || max_requests > 0 || max_cache_size > 0) {
        if (max_cache_size > 0) {
            ht = malloc(sizeof (struct hashtable));
            ht->size = 10000000;
            ht->table = malloc(sizeof (struct entry *) * ht->size);
            ht->LRU = NULL;
            ht->cache_size = max_cache_size;
            ht->available_cache_size = max_cache_size;
            ht->ht_lock = malloc(sizeof (pthread_mutex_t));
            pthread_mutex_init(ht->ht_lock, NULL);
            int i = 0;
            for (; i < ht->size; ++i) {
                ht->table[i] = NULL;
            }
        }
        sv->cv_full = malloc(sizeof (pthread_cond_t));
        sv->cv_empty = malloc(sizeof (pthread_cond_t));
        sv->lock = malloc(sizeof (pthread_mutex_t));
        sv->buf = malloc((max_requests + 1) * sizeof (int));
        sv->in = 0;
        sv->out = 0;
        pthread_cond_init(sv->cv_full, NULL);
        pthread_cond_init(sv->cv_empty, NULL);
        pthread_mutex_init(sv->lock, NULL);
        pthread_t *worker_threads = malloc(nr_threads * sizeof (pthread_t));
        int i = 0;
        for (; i < nr_threads; i++) {
            pthread_create(&worker_threads[i], NULL, stub_func, (void *) sv);
        }
    }

    /* Lab 4: create queue of max_request size when max_requests > 0 */

    /* Lab 5: init server cache and limit its size to max_cache_size */

    /* Lab 4: create worker threads when nr_threads > 0 */

    return sv;
}

void
server_request(struct server *sv, int connfd) {
    if (sv->nr_threads == 0) { /* no worker threads */
        do_server_request(sv, connfd);
    } else {
        pthread_mutex_lock(sv->lock);
        while ((sv->in - sv->out + (sv->max_requests + 1)) % (sv->max_requests + 1) == (sv->max_requests)) {
            pthread_cond_wait(sv->cv_full, sv->lock);
        }
        sv->buf[sv->in] = connfd;
        if (sv->in == sv->out)
            pthread_cond_signal(sv->cv_empty);
        sv->in = (sv->in + 1) % (sv->max_requests + 1);
        pthread_mutex_unlock(sv->lock);
    }
}

