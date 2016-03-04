/*
 * file.c
 *
 * operations for regular files
 *
 */

#include "testfs.h"
#include "inode.h"
#include "dir.h"
#include "read_write.h"

/* get inode for a file or directory called name located within directory dir.
 * the inode is returned in inptr. */
static int
testfs_get_file_inode(struct super_block *sb, struct inode *dir,
		      const char *name, struct inode **inptr)
{
	int inode_nr = testfs_dir_name_to_inode_nr(dir, name);
	struct inode *in;

	if (inode_nr < 0)
		return inode_nr;
	in = testfs_get_inode(sb, inode_nr);
	if (testfs_inode_get_type(in) == I_DIR) {
		testfs_put_inode(in);
		return -EISDIR;
	}

	*inptr = in;
	return 0;
}

/* read a file called filename. 
 * read the file starting at offset, and read size bytes. */
int
cmd_read(struct super_block *sb, struct context *c)
{
	int ret;
	const char *filename;
	off_t offset;
	size_t size;
	char *buf;
	struct inode *in;
	off_t file_size;

	if (c->nargs != 4) {
		return -EINVAL;
	}

	filename = c->cmd[1];
	ret = str_to_offset(c->cmd[2], &offset);
	if (ret < 0)
		return ret;
	if (offset < 0)
		return -EINVAL;
	ret = str_to_size(c->cmd[3], &size);
	if (ret < 0)
		return ret;

	ret = testfs_get_file_inode(sb, c->cur_dir, filename, &in);
	if (ret < 0)
		return ret;

	file_size = testfs_inode_get_size(in);
	if (offset + (off_t)size > file_size) {
		ret = -EINVAL;
		goto out;
	}
	buf = malloc(size + 1);
	if (!buf) {
		ret = -ENOMEM;
		goto out;
	}
	ret = testfs_read_data(in, buf, offset, size);
	if (ret >= 0) {
		buf[size] = 0;
		printf("%s\n", buf);
	}
	free(buf);
out:
	testfs_put_inode(in);
	return ret;
}

/* write a file called filename. 
 * write the file starting at offset, and the bytes in buf. */
int
cmd_write(struct super_block *sb, struct context *c)
{
	int ret;
	struct inode *in;
	const char *filename;
	off_t offset;
	size_t size;
	const char *buf;

	if (c->nargs != 4) {
		return -EINVAL;
	}

	filename = c->cmd[1];
	ret = str_to_offset(c->cmd[2], &offset);
	if (ret < 0)
		return ret;
	if (offset < 0)
		return -EINVAL;
	buf = c->cmd[3];

	ret = testfs_get_file_inode(sb, c->cur_dir, filename, &in);
	if (ret < 0)
		return ret;

	size = strlen(buf);
	ret = testfs_write_data(in, buf, offset, size);
	testfs_sync_inode(in);
	testfs_put_inode(in);
	return ret;
}
