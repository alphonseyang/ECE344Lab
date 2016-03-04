#ifndef _INODE_H
#define _INODE_H

#include "list.h"

typedef enum { I_NONE, I_FILE, I_DIR } inode_type;

#define NR_DIRECT_BLOCKS 10
#define NR_INDIRECT_BLOCKS (BLOCK_SIZE/(int)sizeof(int))

/* inode structure on disk */
struct dinode {
	s32 i_type;
	s32 i_block_nr[NR_DIRECT_BLOCKS];
	s32 i_indirect;
	s32 i_dindirect;
	s32 block_count; /* number of blocks allocated to this file */
	off_t i_size;
};

/* inode is dirty */
#define I_FLAGS_DIRTY     0x1

/* inode structure in memory */
struct inode {
	struct dinode in;	/* disk inode structure */
	int i_flags;		/* we only use the dirty flag above */
	int i_nr;		/* inode number */
	int i_count;		/* ref. count, so we free the inode only when
				 * it is not in use. */
	struct hlist_node hnode; /* keep these structures in a hash table */
	struct super_block *sb;
};

#define INODES_PER_BLOCK (u32)(BLOCK_SIZE/(sizeof(struct dinode)))

/* initialize a hash table for storing inodes */
void inode_hash_init(void);
void inode_hash_destroy(void);

/* retrieve inode from disk and store it in hash table */
struct inode *testfs_get_inode(struct super_block *sb, int inode_nr);

/* copy the updated (dirty) in-memory inode to the disk */
void testfs_sync_inode(struct inode *in);

/* remove inode from hash table if no one is using it */
void testfs_put_inode(struct inode *in);

/* functions to access inode information */
int testfs_inode_get_nr(struct inode *in);
inode_type testfs_inode_get_type(struct inode *in);
off_t testfs_inode_get_size(struct inode *in);
int testfs_inode_get_block_count(struct inode *in);

/* create an inode in the file system */
int testfs_create_inode(struct super_block *sb, inode_type type,
			struct inode **inp);

/* remove an inode from the file system */
void testfs_remove_inode(struct inode *in);

/* allocates a new physical block for the inode.
 * returns physical block number or error. */
int testfs_alloc_block_for_inode(struct inode *in);


/* deallocates a physical block that was previously allocated to this inode */
void testfs_free_block_from_inode(struct inode *in, int phy_block_nr);

#endif /* _INODE_H */
