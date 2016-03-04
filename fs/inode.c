/*
 * inode.c
 *
 * defines operations on the inode structure
 *
 */

#include "testfs.h"
#include "super.h"
#include "block.h"
#include "inode.h"
#include "read_write.h"

static struct hlist_head *inode_hash_table = NULL;

#define INODE_HASH_SHIFT 8

#define inode_hashfn(nr)	\
	hash_int((unsigned int)nr, INODE_HASH_SHIFT)

static const int inode_hash_size = (1 << INODE_HASH_SHIFT);

void
inode_hash_init(void)
{
	int i;

	inode_hash_table = malloc(inode_hash_size * sizeof(struct hlist_head));
	if (!inode_hash_table) {
		EXIT("malloc");
	}
	for (i = 0; i < inode_hash_size; i++) {
		INIT_HLIST_HEAD(&inode_hash_table[i]);
	}
}

void
inode_hash_destroy(void)
{
	int i;
	assert(inode_hash_table);
	for (i = 0; i < inode_hash_size; i++) {
		assert(hlist_empty(&inode_hash_table[i]));
	}
	free(inode_hash_table);
}

static struct inode *
inode_hash_find(struct super_block *sb, int inode_nr)
{
	struct hlist_node *elem;
	struct inode *in;

	hlist_for_each_entry(in, elem,
			     &inode_hash_table[inode_hashfn(inode_nr)], hnode) {
		if ((in->sb == sb) && (in->i_nr == inode_nr)) {
			return in;
		}
	}
	return NULL;
}

static void
inode_hash_insert(struct inode *in)
{
	INIT_HLIST_NODE(&in->hnode);
	hlist_add_head(&in->hnode, &inode_hash_table[inode_hashfn(in->i_nr)]);
}

static void
inode_hash_remove(struct inode *in)
{
	hlist_del(&in->hnode);
}

/* get the relative block number in which this inode is stored on disk. 
 * this block number is relative to the starting inode block. */
static int
testfs_inode_to_block_nr(struct inode *in)
{
	int block_nr = in->i_nr / INODES_PER_BLOCK;
	assert(block_nr >= 0);
	assert(block_nr < NR_INODE_BLOCKS);
	return block_nr;
}

/* get the offset within a block in which this inode is stored */
static int
testfs_inode_to_block_offset(struct inode *in)
{
	int block_offset = (in->i_nr % INODES_PER_BLOCK) *
		sizeof(struct dinode);
	assert(block_offset >= 0);
	assert(block_offset < BLOCK_SIZE);
	return block_offset;
}

/* read the inode block in which inode in is located */
static void
testfs_read_inode_block(struct inode *in, char *block)
{
	int block_nr = testfs_inode_to_block_nr(in);
	read_blocks(in->sb, block, in->sb->sb.inode_blocks_start + block_nr, 1);
}

/* write the inode block in which inode in is located */
static void
testfs_write_inode_block(struct inode *in, char *block)
{
	int block_nr = testfs_inode_to_block_nr(in);
	write_blocks(in->sb, block, in->sb->sb.inode_blocks_start + block_nr,
		     1);
}

/* reads an inode from disk and creates an in-memory inode structure. the
 * in-memory inode is hashed and reference counted. */
struct inode *
testfs_get_inode(struct super_block *sb, int inode_nr)
{
	char block[BLOCK_SIZE];
	int block_offset;
	struct inode *in;

	in = inode_hash_find(sb, inode_nr);
	if (in) {
		/* increment ref count */
		in->i_count++;
		return in;
	}
	/* allocate a new in-memory inode structure */
	if ((in = calloc(1, sizeof(struct inode))) == NULL) {
		EXIT("calloc");
	}
	in->i_flags = 0;
	in->i_nr = inode_nr;
	in->sb = sb;
	in->i_count = 1;
	/* read inode from disk into memory */
	testfs_read_inode_block(in, block);
	block_offset = testfs_inode_to_block_offset(in);
	memcpy(&in->in, block + block_offset, sizeof(struct dinode));
	inode_hash_insert(in);
	return in;
}

/* synchronizes the inode in memory to disk */
void
testfs_sync_inode(struct inode *in)
{
	char block[BLOCK_SIZE];
	int block_offset;

	if ((in->i_flags & I_FLAGS_DIRTY) == 0)
		return;
	testfs_read_inode_block(in, block);
	block_offset = testfs_inode_to_block_offset(in);
	memcpy(block + block_offset, &in->in, sizeof(struct dinode));
	testfs_write_inode_block(in, block);
	in->i_flags &= ~I_FLAGS_DIRTY;
}

/* free the in-memory inode structure if no one is using it */
void
testfs_put_inode(struct inode *in)
{
	assert((in->i_flags & I_FLAGS_DIRTY) == 0);
	if (--in->i_count == 0) {
		inode_hash_remove(in);
		free(in);
	}
}

int
testfs_inode_get_nr(struct inode *in)
{
	return in->i_nr;
}

inode_type
testfs_inode_get_type(struct inode * in)
{
	return in->in.i_type;
}

off_t
testfs_inode_get_size(struct inode * in)
{
	return in->in.i_size;
}

int
testfs_inode_get_block_count(struct inode *in)
{
	return in->in.block_count;
}

/* creates a new inode for a file or directory */
int
testfs_create_inode(struct super_block *sb, inode_type type, struct inode **inp)
{
	struct inode *in;
	int inode_nr = testfs_get_inode_freemap(sb);

	if (inode_nr < 0) {
		return inode_nr;
	} else if (inode_nr / INODES_PER_BLOCK >= NR_INODE_BLOCKS) {
		/* undo the freemap and return error */
		testfs_put_inode_freemap(sb, inode_nr);
		return -ENOSPC;
	}
	in = testfs_get_inode(sb, inode_nr);
	in->in.i_type = type;
	in->i_flags |= I_FLAGS_DIRTY;
	*inp = in;
	return 0;
}

/* removes inode and all blocks associated with inode */
void
testfs_remove_inode(struct inode *in)
{
	/* free all the data blocks */
	testfs_free_blocks(in);
	assert(in->in.block_count == 0);
	/* zero the inode */
	bzero(&in->in, sizeof(struct dinode));
	in->i_flags |= I_FLAGS_DIRTY;
	testfs_put_inode_freemap(in->sb, in->i_nr);
	testfs_sync_inode(in);
	testfs_put_inode(in);
}

/* allocates a new physical block for the inode.
 * returns physical block number or error. */
int
testfs_alloc_block_for_inode(struct inode *in)
{
	int phy_block_nr;

	phy_block_nr = testfs_alloc_block(in->sb);
	if (phy_block_nr >= 0) {
		in->in.block_count++;
		in->i_flags |= I_FLAGS_DIRTY;
	}
	return phy_block_nr;
}

/* deallocates a physical block that was previously allocated to this inode */
void
testfs_free_block_from_inode(struct inode *in, int phy_block_nr)
{
	if (phy_block_nr == 0)
		return;
	testfs_free_block(in->sb, phy_block_nr);
	in->in.block_count--;
	in->i_flags |= I_FLAGS_DIRTY;
}
