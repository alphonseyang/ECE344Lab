#ifndef _BLOCK_H
#define _BLOCK_H

/* start at block number start and write nr blocks */
void write_blocks(struct super_block *sb, const char *blocks, off_t start,
		  size_t nr);
/* start at block number start and zero out nr blocks */
void zero_blocks(struct super_block *sb, off_t start, size_t nr);
/* start at block number start and read nr blocks */
void read_blocks(struct super_block *sb, char *blocks, off_t start, size_t nr);

#endif /* _BLOCK_H */
