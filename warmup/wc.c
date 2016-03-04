#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#include "wc.h"
#define HASHTABLE_SIZE 100000000

struct entry {
    char *word; //this is the key
    int num_seen; //this is the value that needs to be set down
    struct entry *next;
};

struct wc {
    /* you can define this struct to have whatever fields you want. */
    int size;
    struct entry **table; //a array of the entry,table[i], i will be the hash number
};

unsigned long hash(char* str){
	unsigned long hash = 5381;
	int c = *str;
	while(c){
		hash = ((hash<<5) + hash) +c;
		str++;
		c = *str;
	}
	return hash%HASHTABLE_SIZE;
}


//change the hash function
//int hash(char *str) {
//    int result = 0;
//    int i = 0;
//    for (; i < strlen(str); i++) {
//        result = result + (int)str[i];
//    }
//    return result % HASHTABLE_SIZE;
//}

struct entry *entry_init(char *word) {
    struct entry *new_entry = (struct entry *) malloc(sizeof (struct entry));
    new_entry->next = NULL;
    new_entry->word = word;
    new_entry->num_seen = 1;
    return new_entry;
}

int insertEntry(int hash_num, char *single_word, struct wc *hashtable) {
    if (hashtable->table[hash_num] == NULL) { //create a new entry head
        struct entry *new_entry = entry_init(single_word);
        hashtable->table[hash_num] = new_entry; //set the new head;
        return 1;
    }
    struct entry *head = hashtable->table[hash_num];
    struct entry *temp = head;
    while (temp->next != NULL && strcmp(single_word, temp->word) != 0)
        temp = temp->next;
    if (temp->next != NULL || (temp->next == NULL && strcmp(single_word, temp->word) == 0)) { //find the same word again
        temp->num_seen = temp->num_seen + 1;
        free(single_word);
        single_word = NULL;
        return 1;
    }
    struct entry *new_entry = entry_init(single_word);
    temp->next = new_entry;
    return 1;
}

struct wc *
wc_init(char *word_array, long size) {
    struct wc *wc;
    wc = (struct wc *) malloc(sizeof (struct wc));
    assert(wc);
    wc->size = HASHTABLE_SIZE;
    wc->table = (struct entry **) malloc(sizeof (struct entry *) * wc->size); //allocate memory for the entry pointers
    int i = 0;
    for (; i < wc->size; i++) {
        wc->table[i] = NULL;
    }
    i = 0;
    int char_count = 0;
    int more_space = 0;
    char *single_word;
    char *moving_word_head = word_array;
    for (; i < size; i++) {
        if (char_count != 0 && (isspace(word_array[i]) || i == (size - 1))) { //when space or when come to the end of array
            single_word = (char *) malloc(sizeof (char) * (char_count + 1));
            strncpy(single_word, moving_word_head + more_space, char_count); //get the word
            single_word[char_count] = '\0';
            moving_word_head += (char_count + 1 + more_space); //move the resource
            char_count = 0;
            more_space = 0;
            int hash_num = hash(single_word);
            insertEntry(hash_num, single_word, wc);
        } else if (char_count == 0 && isspace(word_array[i])) {
            more_space++;
        } else
            char_count++;
    }
    return wc;
}

void
wc_output(struct wc *wc) {
    int i = 0;
    for (; i < HASHTABLE_SIZE; i++) {
        if (wc->table[i] != NULL) {
            struct entry *temp = wc->table[i];
            while (temp != NULL) {
                printf("%s:%d\n", temp->word, temp->num_seen);
                temp = temp->next;
            }
        }
    }
}

void
wc_destroy(struct wc *wc) {
    int i = 0;
    for (; i < HASHTABLE_SIZE; i++) {
        if (wc->table[i] != NULL) {
            struct entry *temp = wc->table[i];
            while (temp != NULL) {
                struct entry *to_delete = temp;
                temp = temp->next;
                free(to_delete->word);
                to_delete->word = NULL;
                free(to_delete);
                to_delete = NULL;
            }
            wc->table[i] = NULL;
        }
    }
    free(wc->table);
    wc->table = NULL;
    free(wc);
}
