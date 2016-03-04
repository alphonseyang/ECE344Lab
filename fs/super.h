#ifndef _SUPER_H
#define _SUPER_H

/* super block on disk */
struct dsuper_block {
	u32 inode_freemap_start; /* starting block number of inode freemap */
	u32 block_freemap_start; /* starting block number of block freemap */
	u32 inode_blocks_start;	 /* starting block number of inode blocks */
	u32 data_blocks_start;	 /* starting block number of data blocks */
	u32 used_inode_count;	 /* total number of allocated inodes */
	u32 used_block_count;	 /* total number of allocated blocks */
	u64 max_fs_blocks;	 /* max number of blocks in the file system */
};

/* in-memory version of super block */
struct super_block {
	struct dsuper_block sb;		/* disk super block structure */
	FILE *dev;			/* device associated with file system */
	struct bitmap *inode_freemap;	/* in-memory version of inode freemap */
	struct bitmap *block_freemap;	/* in-memory version of block freemap */
};

/* get the device associated with this file system */
FILE *testfs_get_dev(struct super_block *sb);
/* create a new super block on device.
 * max_fs_blocks is the maximum number of blocks in the file system. */
struct super_block *testfs_make_super_block(const char *dev, u64 max_fs_blocks);
/* create a new inode freemap on disk */
void testfs_make_inode_freemap(struct super_block *sb);
/* create a new block freemap on disk */
void testfs_make_block_freemap(struct super_block *sb);
/* create the inode blocks on disk */
void testfs_make_inode_blocks(struct super_block *sb);

/* initialize the in-memory version of the super block */
int testfs_init_super_block(const char *file, struct super_block **sbp);
/* destroy the in-memory version of the super block */
void testfs_close_super_block(struct super_block *sb);

/* allocate an inode in the inode freemap and return its inode number. */
int testfs_get_inode_freemap(struct super_block *sb);
/* release an allocated inode. */
void testfs_put_inode_freemap(struct super_block *sb, int inode_nr);

/* allocate a block in the block freemap and return its block number. */
int testfs_alloc_block(struct super_block *sb);
/* release an allocated block. */
int testfs_free_block(struct super_block *sb, int block_nr);
/* print file system statistics */
int cmd_fsstat(struct super_block *sb, struct context *c);
#endif /* _SUPER_H */
