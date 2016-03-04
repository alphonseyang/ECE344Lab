/*
 * testfs.c
 *
 * program for accessing a testfs filesystem
 *
 */

#include <stdbool.h>
#include <popt.h>
#include <malloc.h>
#include "testfs.h"
#include "super.h"
#include "inode.h"

static poptContext pc;	/* context for parsing command-line options */

static const char *disk;	/* name of disk */
static int hide_prompt = 0;	/* by default, show prompt */
static int verbose = 0;		/* show verbose output */

static bool can_quit = false;

static int cmd_help(struct super_block *, struct context *c);
static int cmd_quit(struct super_block *, struct context *c);

static const struct cmd {
	const char *name;
	int (*func) (struct super_block * sb, struct context * c);
	int max_args;
	const char *help;
	const char *usage;
} cmdtable[] = {
	/* menus */
	{ "help", cmd_help, 1, "prints this help message.", "usage: help"},
	{ "cd", cmd_cd, 2, "changes to directory DIR.", "usage: cd DIR"},
	{ "pwd", cmd_pwd, 1, "prints current directory.", "usage: pwd"},
	{ "ls", cmd_ls, 2, "lists files and directories in directory DIR.",
	  "usage: ls DIR"},
	{ "lsr", cmd_lsr, 2, "recursively lists files and directories "
	  "in directory DIR.", "usage: lsr DIR"},
	{ "create", cmd_create, 2, "creates a new file FILE in the "
	  "current directory.", "usage: create FILE"},
	{ "stat", cmd_stat, 2, "prints information on file or dir named NAME.",
	  "usage: stat NAME"},
	{ "rm", cmd_rm, 2, "removes a file or directory with name NAME.",
	  "usage: rm NAME"},
	{ "mkdir", cmd_mkdir, 2, "creates a new directory DIR in the"
	  " current directory.", "usage: mkdir DIR"},
	{ "read", cmd_read, 4, "reads SIZE bytes of ascii "
	  "data from file FILE at offset OFFSET.",
	  "usage: read FILE OFFSET SIZE"},
	{ "write", cmd_write, 4, "write ascii data DATA to file FILE at "
	  "offset OFFSET.", "usage: write FILE OFFSET DATA"},
	{ "fsstat", cmd_fsstat, 1, "print inode and block usage.",
	  "usage: fsstat"},
	{ "quit", cmd_quit, 1, "quits this program.", "usage: quit"},
	{ NULL, NULL, 0, NULL, NULL},
	/* when adding new commands, you must make sure MAX_ARGS >= max_args 
	 * for your new command */
};

static int
cmd_help(struct super_block *sb, struct context *c)
{
	int i;

	(void)sb;
	(void)c;

	printf("%s", "Commands:\n");
	for (i = 0; cmdtable[i].name; i++) {
		printf("%8s - %s %s\n", cmdtable[i].name, cmdtable[i].help,
		       cmdtable[i].usage);
	}
	fflush(stdout);
	return 0;
}

static int
cmd_quit(struct super_block *sb, struct context *c)
{
	(void)sb;
	(void)c;

	printf("Bye!\n");
	fflush(stdout);
	can_quit = true;

	return 0;
}

static void
parse_command(const struct cmd *cmd, struct context *c, const char *name,
	      char *args)
{
	char *save = NULL;
	int j = 0;

	assert(cmd->func);

	c->cmd[j++] = name;
	while (j < cmd->max_args &&
	       (c->cmd[j] = strtok_r(args, " \t\r\n", &save)) != NULL) {
		j++;
		args = NULL;
	}
	if ((c->cmd[j] = strtok_r(args, "\r\n", &save)) != NULL) {
		j++;
	}
	for (c->nargs = j++; j <= MAX_ARGS; j++) {
		c->cmd[j] = NULL;
	}
}

static void
handle_command(struct super_block *sb, struct context *c, const char *name,
	       char *args)
{
	int i;
	int ret;
	
	if (name == NULL)
		return;

	for (i = 0; cmdtable[i].name; i++) {
		if (strcmp(name, cmdtable[i].name) == 0) {
			parse_command(&cmdtable[i], c, name, args);
			ret = cmdtable[i].func(sb, c);
			if (ret < 0) {
				fprintf(stderr, "%s: %s\n", c->cmd[0],
					strerror(-ret));
				fprintf(stderr, "%s\n", cmdtable[i].usage);
			}
			return;
		}
	}
	fprintf(stderr, "%s: command not found. type \"help\"\n", name);
}

static void
usage()
{
	poptSetOtherOptionHelp(pc, "device");
	poptPrintUsage(pc, stderr, 0);
	exit(1);
}

void
parse_arguments(int argc, const char *argv[])
{
	const char **args;
	int c;
	struct poptOption options_table[] = {
		{NULL, 'n', POPT_ARG_NONE, &hide_prompt, 'n',
		 "don't print testfs prompt", 0},
		{NULL, 'v', POPT_ARG_NONE, &verbose, 'v',
		 "print verbose output", 0},
		POPT_AUTOHELP
		{NULL, 0, 0, NULL, 0, NULL, NULL}
	};

	pc = poptGetContext(NULL, argc, argv, options_table, 0);
	while ((c = poptGetNextOpt(pc)) >= 0);
	if (c < -1) {	/* an error occurred during option processing */
		fprintf(stderr, "%s: %s\n",
			poptBadOption(pc, POPT_BADOPTION_NOALIAS),
			poptStrerror(c));
		usage();
	}
	args = poptGetArgs(pc);
	if (!args || !args[0] || args[1]) {
		usage();
	}
	disk = args[0];
	poptFreeContext(pc);
}

int
main(int argc, const char *argv[])
{
	struct super_block *sb;
	int ret;
	struct context c;
	struct mallinfo minfo;
	char *line = NULL;
	size_t line_size = 0;

	parse_arguments(argc, argv);
	ret = testfs_init_super_block(disk, &sb);
	free((char *)disk);
	if (ret) {
		EXIT("testfs_init_super_block");
	}
	c.cur_dir = testfs_get_inode(sb, 0);	/* root dir */
	do {
		char *cname, *pargs, *save = NULL;

		if (!hide_prompt) {
			printf("%s", "% ");
			fflush(stdout);
		}
		if (getline(&line, &line_size, stdin) == EOF) {
			break;
		}
		if (line[0] == '#') {
			/* input files can have comments. we ignore them */
			continue;
		}
		if (verbose) {
			printf("%s", line);
			fflush(stdout);
		}
		cname = strtok_r(line, " \t\r\n", &save);
		pargs = strtok_r(NULL, "\r\n", &save);
		handle_command(sb, &c, cname, pargs);
		fflush(stdout);
		fflush(stderr);
	} while (can_quit == false);

	free(line);
	testfs_put_inode(c.cur_dir);
	testfs_close_super_block(sb);

	/* check for memory leaks */
	minfo = mallinfo();
	assert(minfo.uordblks == 0);
	assert(minfo.hblks == 0);
	return 0;
}
