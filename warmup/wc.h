#ifndef _WC_H_
#define _WC_H_

/* DO NOT CHANGE THIS FILE */

/* Forward declaration of structure for the function declarations below. */
struct wc;

/* You will be writing these functions */

/* Initialize wc data structure, returning pointer to it. The input to this
 * function is an array of characters. The length of this array is size.
 * The array contains a sequence of words, separated by spaces. You need to
 * parse this array for words, and initialize your data structure with the words
 * in the array. You can use the isspace() function to look for spaces between
 * words. Note that the array is read only and cannot be modified. */
struct wc *wc_init(char *word_array, long size);

/* wc_output produces output, consisting of unique words that have been inserted
 * in wc (in wc_init), and a count of the number of times each word has been
 * seen.
 *
 * The output should be sent to standard output, i.e., using the standard printf
 * function.
 *
 * The output should be in the format shown below. The words do not have to be
  * sorted in any order. Do not add any extra whitespace.
word1:5
word2:10
word3:30
word4:1
 */
void wc_output(struct wc *wc);

/* Destroy all the data structures you have created, so you have no memory
 * loss. */
void wc_destroy(struct wc *wc);

#endif /* _WC_H_ */
