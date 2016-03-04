/*
 * mktestfs.c
 *
 * make a new testfs file system
 *
 */

#include <popt.h>
#include "testfs.h"
#include "super.h"
#include "dir.h"
#include "inode.h"

static poptContext pc;	/* context for parsing command-line options */

static void
usage()
{
	poptSetOtherOptionHelp(pc, "device");
	poptPrintUsage(pc, stderr, 0);
	exit(1);
}

int
main(int argc, const char *argv[])
{
	char c;
	struct super_block *sb;
	int ret;
	u64 max_fs_blocks = (u64)BLOCK_FREEMAP_SIZE * BLOCK_SIZE * 8;
	u64 max_fs_size_mb = (max_fs_blocks * BLOCK_SIZE) >> 20;
	u64 fs_size_mb = max_fs_size_mb;
	u64 max_file_blocks;
	const char **args;

	struct poptOption options_table[] = {
		{NULL, 's', POPT_ARG_INT, &fs_size_mb, 's',
		 "maximum file system size in MB", 0},
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
	if (fs_size_mb < 1) {
		fprintf(stderr, "minimum file system size is 1 MB\n");
		usage();
	}
	if (fs_size_mb > max_fs_size_mb) {
		fprintf(stderr, "maximum file system size is %" PRId64 "\n",
			max_fs_size_mb);
		usage();
	}
	args = poptGetArgs(pc);
	if (!args || !args[0] || args[1]) {
		usage();
	}
	/* args[0] is the device */
	max_fs_blocks = (fs_size_mb << 20) / BLOCK_SIZE;
	sb = testfs_make_super_block(args[0], max_fs_blocks);
	testfs_make_inode_freemap(sb);
	testfs_make_block_freemap(sb);
	testfs_make_inode_blocks(sb);
	testfs_close_super_block(sb);

	ret = testfs_init_super_block(args[0], &sb);
	if (ret) {
		EXIT("testfs_init_super_block");
	}
	poptFreeContext(pc);

	testfs_make_root_dir(sb);
	testfs_close_super_block(sb);

	printf("block size: %d\n", BLOCK_SIZE);
	printf("max file system blocks: %" PRId64 "\n", max_fs_blocks);
	printf("max file system size: %" PRId64 "\n",
	       max_fs_blocks * BLOCK_SIZE);

	printf("\ninode size: %zu\n", sizeof(struct dinode));
	printf("max number of inodes: %u\n",
	       NR_INODE_BLOCKS * INODES_PER_BLOCK);

	/* direct + single indirect + double indirect */
	max_file_blocks = NR_DIRECT_BLOCKS + NR_INDIRECT_BLOCKS +
		NR_INDIRECT_BLOCKS * NR_INDIRECT_BLOCKS;
	printf("max file size: %" PRId64 " bytes\n\n",
	       max_file_blocks * BLOCK_SIZE);

	return 0;
}
