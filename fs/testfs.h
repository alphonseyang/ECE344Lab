#ifndef _TESTFS_H
#define _TESTFS_H

#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "common.h"

#define BLOCK_SIZE 8192

#define SUPER_BLOCK_SIZE    1
#define INODE_FREEMAP_SIZE  1
#define BLOCK_FREEMAP_SIZE  16
#define NR_INODE_BLOCKS     128

struct super_block;
struct inode;

#define MAX_ARGS 6

struct context {
	int nargs;
	const char *cmd[MAX_ARGS + 1];	// +1 to keep the overflows
	struct inode *cur_dir;
};

// dir.c
int cmd_cd(struct super_block *, struct context *c);
int cmd_pwd(struct super_block *, struct context *c);
int cmd_ls(struct super_block *, struct context *c);
int cmd_lsr(struct super_block *, struct context *c);
int cmd_create(struct super_block *, struct context *c);
int cmd_stat(struct super_block *, struct context *c);
int cmd_rm(struct super_block *, struct context *c);
int cmd_mkdir(struct super_block *, struct context *c);

// file.c
int cmd_read(struct super_block *, struct context *c);
int cmd_write(struct super_block *, struct context *c);
#endif /* _TESTFS_H */
