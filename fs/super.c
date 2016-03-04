/*
 * super.c
 *
 * defines operations on the super block structure
 *
 */

#include "testfs.h"
#include "bitmap.h"
#include "super.h"
#include "inode.h"
#include "block.h"

static void
testfs_write_super_block(struct super_block *sb)
{
	char block[BLOCK_SIZE] = { 0 };

	assert(sizeof(struct dsuper_block) <= BLOCK_SIZE);
	memcpy(block, &sb->sb, sizeof(struct dsuper_block));
	write_blocks(sb, block, 0, 1);
}

static void
testfs_write_inode_freemap(struct super_block *sb, int inode_nr)
{
	char *freemap;
	int nr;

	assert(sb->inode_freemap);
	freemap = bitmap_getdata(sb->inode_freemap);
	nr = inode_nr / (BLOCK_SIZE * BITS_PER_WORD);
	write_blocks(sb, freemap + (nr * BLOCK_SIZE),
		     sb->sb.inode_freemap_start + nr, 1);
}

static void
testfs_write_block_freemap(struct super_block *sb, int block_nr)
{
	char *freemap;
	int nr;

	assert(sb->block_freemap);
	freemap = bitmap_getdata(sb->block_freemap);
	nr = block_nr / (BLOCK_SIZE * BITS_PER_WORD);
	write_blocks(sb, freemap + (nr * BLOCK_SIZE),
		     sb->sb.block_freemap_start + nr, 1);
}

/* return free block number or negative value */
static int
testfs_get_block_freemap(struct super_block *sb)
{
	u_int32_t index;
	int ret;

	assert(sb->block_freemap);
	ret = bitmap_alloc(sb->block_freemap, &index);
	if (ret < 0)
		return ret;
	testfs_write_block_freemap(sb, index);
	return index;
}

/* release allocated block */
static void
testfs_put_block_freemap(struct super_block *sb, int block_nr)
{
	assert(sb->block_freemap);
	bitmap_unmark(sb->block_freemap, block_nr);
	testfs_write_block_freemap(sb, block_nr);
}

FILE *
testfs_get_dev(struct super_block *sb)
{
	return sb->dev;
}

unsigned int
testfs_inode_blocks_start(struct super_block *sb)
{
	return sb->sb.inode_blocks_start;
}

struct super_block *
testfs_make_super_block(const char *dev, u64 max_fs_blocks)
{
	struct super_block *sb = calloc(1, sizeof(struct super_block));

	if (!sb) {
		EXIT("malloc");
	}
	if ((sb->dev = fopen(dev, "w")) == NULL) {
		EXIT(dev);
	}
	sb->sb.inode_freemap_start = SUPER_BLOCK_SIZE;
	sb->sb.block_freemap_start = sb->sb.inode_freemap_start +
		INODE_FREEMAP_SIZE;
	sb->sb.inode_blocks_start = sb->sb.block_freemap_start +
		BLOCK_FREEMAP_SIZE;
	sb->sb.data_blocks_start = sb->sb.inode_blocks_start + NR_INODE_BLOCKS;
	sb->sb.used_inode_count = 0;
	sb->sb.used_block_count = 0;
	sb->sb.max_fs_blocks = max_fs_blocks;
	testfs_write_super_block(sb);
	inode_hash_init();
	return sb;
}

void
testfs_make_inode_freemap(struct super_block *sb)
{
	zero_blocks(sb, sb->sb.inode_freemap_start, INODE_FREEMAP_SIZE);
}

void
testfs_make_block_freemap(struct super_block *sb)
{
	zero_blocks(sb, sb->sb.block_freemap_start, BLOCK_FREEMAP_SIZE);
}

void
testfs_make_inode_blocks(struct super_block *sb)
{
	const size_t num_bits_in_freemap = BLOCK_SIZE * INODE_FREEMAP_SIZE * 8;
	if (num_bits_in_freemap < NR_INODE_BLOCKS * INODES_PER_BLOCK) {
		EXIT("not enough inode freemap to support "
		     STR(NR_INODE_BLOCKS) " inode blocks");
	}
	/* dinodes should not span blocks */
	assert((BLOCK_SIZE % sizeof(struct dinode)) == 0);
	zero_blocks(sb, sb->sb.inode_blocks_start, NR_INODE_BLOCKS);
}

int
testfs_init_super_block(const char *file, struct super_block **sbp)
{
	struct super_block *sb = malloc(sizeof(struct super_block));
	char block[BLOCK_SIZE];
	int ret, sock;

	if (!sb) {
		return -ENOMEM;
	}

	if ((sock = open(file, O_RDWR)) < 0) {
		return errno;
	} else if ((sb->dev = fdopen(sock, "r+")) == NULL) {
		return errno;
	}

	read_blocks(sb, block, 0, 1);
	memcpy(&sb->sb, block, sizeof(struct dsuper_block));

	ret = bitmap_create(BLOCK_SIZE * INODE_FREEMAP_SIZE * BITS_PER_WORD,
			    &sb->inode_freemap);
	if (ret < 0)
		return ret;
	read_blocks(sb, bitmap_getdata(sb->inode_freemap),
		    sb->sb.inode_freemap_start, INODE_FREEMAP_SIZE);

	ret = bitmap_create(BLOCK_SIZE * BLOCK_FREEMAP_SIZE * BITS_PER_WORD,
			    &sb->block_freemap);
	if (ret < 0)
		return ret;
	read_blocks(sb, bitmap_getdata(sb->block_freemap),
		    sb->sb.block_freemap_start, BLOCK_FREEMAP_SIZE);
	inode_hash_init();
	*sbp = sb;

	return 0;
}

void
testfs_close_super_block(struct super_block *sb)
{
	testfs_write_super_block(sb);
	inode_hash_destroy();
	if (sb->inode_freemap) {
		write_blocks(sb, bitmap_getdata(sb->inode_freemap),
			     sb->sb.inode_freemap_start, INODE_FREEMAP_SIZE);
		bitmap_destroy(sb->inode_freemap);
		sb->inode_freemap = NULL;
	}
	if (sb->block_freemap) {
		write_blocks(sb, bitmap_getdata(sb->block_freemap),
			     sb->sb.block_freemap_start, BLOCK_FREEMAP_SIZE);
		bitmap_destroy(sb->block_freemap);
		sb->block_freemap = NULL;
	}
	fflush(sb->dev);
	fclose(sb->dev);
	sb->dev = NULL;
	free(sb);
}

int
testfs_get_inode_freemap(struct super_block *sb)
{
	u_int32_t index;
	int ret;

	assert(sb->inode_freemap);
	ret = bitmap_alloc(sb->inode_freemap, &index);
	if (ret < 0)
		return ret;
	testfs_write_inode_freemap(sb, index);
	sb->sb.used_inode_count++;
	testfs_write_super_block(sb);
	return index;
}

void
testfs_put_inode_freemap(struct super_block *sb, int inode_nr)
{
	assert(sb->inode_freemap);
	bitmap_unmark(sb->inode_freemap, inode_nr);
	testfs_write_inode_freemap(sb, inode_nr);
	assert(sb->sb.used_inode_count > 0);
	sb->sb.used_inode_count--;
	testfs_write_super_block(sb);
}

int
testfs_alloc_block(struct super_block *sb)
{
	int phy_block_nr;

	/* file system size is limited to max_fs_blocks */
	if (sb->sb.used_block_count >= sb->sb.max_fs_blocks)
		return -ENOSPC;
	phy_block_nr = testfs_get_block_freemap(sb);
	if (phy_block_nr < 0)
		return phy_block_nr;
	sb->sb.used_block_count++;
	testfs_write_super_block(sb);
	return sb->sb.data_blocks_start + phy_block_nr;
}

/* free a block.
 * returns negative value on error. */
int
testfs_free_block(struct super_block *sb, int block_nr)
{

	zero_blocks(sb, block_nr, 1);
	block_nr -= sb->sb.data_blocks_start;
	assert(block_nr >= 0);
	testfs_put_block_freemap(sb, block_nr);
	assert(sb->sb.used_block_count > 0);
	sb->sb.used_block_count--;
	testfs_write_super_block(sb);
	return 0;
}

int
cmd_fsstat(struct super_block *sb, struct context *c)
{
	if (c->nargs != 1) {
		return -EINVAL;
	}

	printf("nr of allocated inodes = %d\n", sb->sb.used_inode_count);
	printf("nr of allocated blocks = %d\n", sb->sb.used_block_count);
	return 0;
}
