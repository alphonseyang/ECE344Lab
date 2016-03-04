#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <popt.h>
#include "common.h"

/* Generate a set of files for the webserver assignment */

poptContext context;	/* context for parsing command-line options */

static void
usage()
{
	poptPrintUsage(context, stderr, 0);
	exit(1);
}

/* mean file size is in terms of 4K, so it is 12k */
#define DEFAULT_MEAN_FILE_SZ 3
/* this will create roughly 12k * 256 = 3MB of files */
#define DEFAULT_NR_FILES 256
/* the directory in which to create the files */
#define DEFAULT_DIR fileset_dir

static int mean_file_sz = DEFAULT_MEAN_FILE_SZ;
static int nr_files = DEFAULT_NR_FILES;
static char *dir = STR(DEFAULT_DIR);

int
main(int argc, const char *argv[])
{
	char c;
	int i;
	DIR *d;
	char filename[1024];
	double total_file_sz = 0;
	int fd_idx;
	char buf_idx[4096];

	struct poptOption options_table[] = {
		{NULL, 'm', POPT_ARG_INT, &mean_file_sz, 'm',
		 "mean file size",
		 " default: " STR(DEFAULT_MEAN_FILE_SZ)},
		{NULL, 'n', POPT_ARG_INT, &nr_files, 'n',
		 "number of files",
		 " default: " STR(DEFAULT_NR_FILES)},
		{NULL, 'd', POPT_ARG_STRING, &dir, 'd',
		 "directory in which the files are created",
		 " default: " STR(DEFAULT_DIR)},
		POPT_AUTOHELP {NULL, 0, 0, NULL, 0}
	};

	context = poptGetContext(NULL, argc, argv, options_table, 0);
	while ((c = poptGetNextOpt(context)) >= 0);
	if (c < -1) {	/* an error occurred during option processing */
		fprintf(stderr, "%s: %s\n",
			poptBadOption(context, POPT_BADOPTION_NOALIAS),
			poptStrerror(c));
		exit(1);
	}
	if (mean_file_sz <= 1) {
		fprintf(stderr, "mean file size is too small\n");
		usage();
	}
	if (nr_files < 1 || nr_files > 99999) {
		fprintf(stderr, "nr of files is out of bounds\n");
		usage();
	}
	if (strlen(dir) > 1000) {
		fprintf(stderr, "dir name is too long\n");
		usage();
	}
	d = opendir(dir);
	if (d) { /* directory exists */
		closedir(d);
	} else {
		if (mkdir(dir, 0755) < 0) {
			fprintf(stderr, "mkdir: %s: %s\n", dir, 
				strerror(errno));
			exit(1);
		}
	}
	init_random();
	strcpy(filename, dir);
	strcat(filename, ".idx");
	SYS(fd_idx = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644));

	/* write the number of files in the index file */
	sprintf(buf_idx, "%d\n", nr_files);
	Rio_write(fd_idx, buf_idx, strlen(buf_idx));

	for (i = 0; i < nr_files; i++) {
		char name[10];
		int fd, file_sz, remaining;
		char buf[4096];
		double ms = mean_file_sz;
		unsigned int csum = 0;
		int j;

		strcpy(filename, dir);
		sprintf(name, "/%05d", i);
		strcat(filename, name);
		file_sz = rand_pareto(4096, ms/(ms - 1));
		total_file_sz += file_sz;
		SYS(fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644));
		remaining = file_sz;
		while (remaining > 0) {
			int sz = (remaining < 4096) ? remaining : 4096;
			for (j = 0; j < sz; j++) {
				/* printable characters lie between 0x20-0x73 */
				buf[j] = random() % (0x73 - 0x20) + 0x20;
				csum += (unsigned char)(buf[j]);
			}
			Rio_write(fd, buf, sz);
			remaining -= sz;
		}
		SYS(close(fd));
		printf("filename = %s, csum = %u, len = %d\n", filename, csum,
		       file_sz);
		sprintf(buf_idx, "%s %u %d\n", filename, csum, file_sz);
		Rio_write(fd_idx, buf_idx, strlen(buf_idx));
	}

	SYS(close(fd_idx));
	printf("mean file size = %d, expected mean file size = %d\n",
	       (int)(total_file_sz / nr_files), mean_file_sz * 4096);
	exit(0);
}
