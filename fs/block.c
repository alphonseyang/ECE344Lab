/*
 * block.c
 *
 * defines block-level operations
 *
 */

#include "testfs.h"
#include "super.h"

static char zero[BLOCK_SIZE] = { 0 };

void
write_blocks(struct super_block *sb, const char *blocks, off_t start, size_t nr)
{
	long pos;
	FILE *dev = testfs_get_dev(sb);

	if ((pos = ftell(dev)) < 0) {
		EXIT("ftell");
	}
	if (fseek(dev, start * BLOCK_SIZE, SEEK_SET) < 0) {
		EXIT("fseek");
	}
	if (fwrite(blocks, BLOCK_SIZE, nr, dev) != nr) {
		EXIT("fwrite");
	}
	if (fseek(dev, pos, SEEK_SET) < 0) {
		EXIT("fseek");
	}
}

void
zero_blocks(struct super_block *sb, off_t start, size_t nr)
{
	size_t i;

	for (i = 0; i < nr; i++) {
		write_blocks(sb, zero, start + i, 1);
	}
}

void
read_blocks(struct super_block *sb, char *blocks, off_t start, size_t nr)
{
	long pos;
	FILE *dev = testfs_get_dev(sb);

	if ((pos = ftell(dev)) < 0) {
		EXIT("ftell");
	}
	if (fseek(dev, start * BLOCK_SIZE, SEEK_SET) < 0) {
		EXIT("fseek");
	}
	if (fread(blocks, BLOCK_SIZE, nr, dev) != nr) {
		EXIT("fread");
	}
	if (fseek(dev, pos, SEEK_SET) < 0) {
		EXIT("fseek");
	}
}
