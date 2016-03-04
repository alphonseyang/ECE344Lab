/*
 * dir.c
 *
 * defines operations on the dirent structure
 *
 */

#include "testfs.h"
#include "dir.h"
#include "inode.h"
#include "read_write.h"

/* get next dirent, updates offset to next dirent in directory.
 * allocates memory for a dirent structure that caller should free.
 * returns NULL on error. */
struct dirent *
testfs_next_dirent(struct inode *dir, off_t * offset)
{
	int ret;
	struct dirent d, *dp;

	assert(dir);
	assert(testfs_inode_get_type(dir) == I_DIR);
	if (*offset >= testfs_inode_get_size(dir))
		return NULL;
	ret = testfs_read_data(dir, (char *)&d, *offset, sizeof(struct dirent));
	if (ret < 0)
		return NULL;
	assert(d.d_name_len > 0);
	dp = malloc(sizeof(struct dirent) + d.d_name_len);
	if (!dp)
		return NULL;
	*dp = d;
	*offset += sizeof(struct dirent);
	ret = testfs_read_data(dir, D_NAME(dp), *offset, d.d_name_len);
	if (ret < 0) {
		free(dp);
		return NULL;
	}
	*offset += d.d_name_len;
	return dp;
}

/* returns dirent associated with inode_nr in dir.
 * returns NULL on error.
 * allocates memory, caller should free. */
static struct dirent *
testfs_find_dirent(struct inode *dir, int inode_nr)
{
	struct dirent *d;
	off_t offset = 0;

	assert(dir);
	assert(testfs_inode_get_type(dir) == I_DIR);
	assert(inode_nr >= 0);
	for (; (d = testfs_next_dirent(dir, &offset)); free(d)) {
		if (d->d_inode_nr == inode_nr)
			return d;
	}
	return NULL;
}

/* write a dirent structure in the directory
 * return 0 on success. return negative value on error. */
static int
testfs_write_dirent(struct inode *dir, const char *name, int len, int inode_nr,
		    int offset)
{
	int ret;
	struct dirent *d = malloc(sizeof(struct dirent) + len);

	if (!d)
		return -ENOMEM;
	assert(inode_nr >= 0);
	d->d_name_len = len;
	d->d_inode_nr = inode_nr;
	strcpy(D_NAME(d), name);
	ret = testfs_write_data(dir, (char *)d, offset,
				sizeof(struct dirent) + len);
	free(d);
	return ret;
}

/* add a dirent structure in the directory.
 * return 0 on success. return negative value on error. */
static int
testfs_add_dirent(struct inode *dir, const char *name, int inode_nr)
{
	struct dirent *d;
	off_t p_offset = 0, offset = 0;
	int found = 0;
	int ret = 0;
	int len = strlen(name) + 1;

	assert(dir);
	assert(testfs_inode_get_type(dir) == I_DIR);
	assert(name);
	for (; ret == 0 && found == 0; free(d)) {
		p_offset = offset;
		if ((d = testfs_next_dirent(dir, &offset)) == NULL)
			break;
		if ((d->d_inode_nr >= 0) && (strcmp(D_NAME(d), name) == 0)) {
			ret = -EEXIST;
			continue;
		}
		if ((d->d_inode_nr >= 0) || (d->d_name_len != len))
			continue;
		found = 1;
	}
	if (ret < 0)
		return ret;
	assert(found || (p_offset == testfs_inode_get_size(dir)));
	return testfs_write_dirent(dir, name, len, inode_nr, p_offset);
}

/* can a directory associated with inode_nr be removed?
 * returns negative value if the directory is not empty. */
static int
testfs_remove_dirent_allowed(struct super_block *sb, int inode_nr)
{
	struct inode *dir;
	off_t offset = 0;
	struct dirent *d;
	int ret = 0;

	dir = testfs_get_inode(sb, inode_nr);
	if (testfs_inode_get_type(dir) != I_DIR)
		goto out;
	for (; ret == 0 && (d = testfs_next_dirent(dir, &offset)); free(d)) {
		if ((d->d_inode_nr < 0) || (strcmp(D_NAME(d), ".") == 0) ||
		    (strcmp(D_NAME(d), "..") == 0))
			continue;
		ret = -ENOTEMPTY;
	}
out:
	testfs_put_inode(dir);
	return ret;
}

#define INVALID_INODE_NR -1

/* remove dirent associated with name in directory dir.
   returns inode_nr of dirent removed.
   returns negative value if name is not found */
static int
testfs_remove_dirent(struct super_block *sb, struct inode *dir,
		     const char *name)
{
	struct dirent *d;
	off_t p_offset, offset = 0;
	int inode_nr = INVALID_INODE_NR;
	int ret = -ENOENT;

	assert(dir);
	assert(name);
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
		return -EINVAL;
	}
	for (; inode_nr == INVALID_INODE_NR; free(d)) {
		p_offset = offset;
		if ((d = testfs_next_dirent(dir, &offset)) == NULL)
			break;
		if ((d->d_inode_nr < 0) || (strcmp(D_NAME(d), name) != 0))
			continue;
		/* found the dirent */
		inode_nr = d->d_inode_nr;
		if ((ret = testfs_remove_dirent_allowed(sb, inode_nr)) < 0)
			continue;	/* this will break out of the loop */
		d->d_inode_nr = INVALID_INODE_NR;
		ret = testfs_write_data(dir, (char *)d, p_offset,
					sizeof(struct dirent) + d->d_name_len);
		if (ret >= 0)
			ret = inode_nr;
	}
	return ret;
}

/* this creates a new empty directory.
 * p_inode_nr is the inode number of the parent. 
 * cdir is the inode of the directory being created.  */
static int
testfs_create_empty_dir(struct super_block *sb, int p_inode_nr,
			struct inode *cdir)
{
	int ret;

	assert(testfs_inode_get_type(cdir) == I_DIR);
	ret = testfs_add_dirent(cdir, ".", testfs_inode_get_nr(cdir));
	if (ret < 0)
		return ret;
	ret = testfs_add_dirent(cdir, "..", p_inode_nr);
	if (ret < 0) {
		testfs_remove_dirent(sb, cdir, ".");
		return ret;
	}
	return 0;
}

/* create a new file or directory called name in the directory dir */
static int
testfs_create_file_or_dir(struct super_block *sb, struct inode *dir,
			  inode_type type, const char *name)
{
	int ret = 0;
	struct inode *in;
	int inode_nr;

	if (dir) {
		inode_nr = testfs_dir_name_to_inode_nr(dir, name);
		if (inode_nr >= 0)
			return -EEXIST;
	}
	/* first create inode */
	ret = testfs_create_inode(sb, type, &in);
	if (ret < 0) {
		goto fail;
	}
	inode_nr = testfs_inode_get_nr(in);
	if (type == I_DIR) {	/* create directory */
		int p_inode_nr = dir ? testfs_inode_get_nr(dir) : inode_nr;
		ret = testfs_create_empty_dir(sb, p_inode_nr, in);
		if (ret < 0)
			goto out;
	}
	/* then add directory entry in dir for the newly created inode */
	if (dir) {
		if ((ret = testfs_add_dirent(dir, name, inode_nr)) < 0)
			goto out;
		testfs_sync_inode(dir);
	}
	testfs_sync_inode(in);
	testfs_put_inode(in);
	return 0;
out:
	testfs_remove_inode(in);
fail:
	return ret;
}

/* print out the current working directory */
static int
testfs_pwd(struct super_block *sb, struct inode *in)
{
	int p_inode_nr;
	struct inode *p_in;
	struct dirent *d;
	int ret;

	assert(in);
	assert(testfs_inode_get_nr(in) >= 0);
	p_inode_nr = testfs_dir_name_to_inode_nr(in, "..");
	assert(p_inode_nr >= 0);
	if (p_inode_nr == testfs_inode_get_nr(in)) {
		printf("/");
		return 1;
	}
	p_in = testfs_get_inode(sb, p_inode_nr);
	d = testfs_find_dirent(p_in, testfs_inode_get_nr(in));
	assert(d);
	ret = testfs_pwd(sb, p_in);
	testfs_put_inode(p_in);
	printf("%s%s", ret == 1 ? "" : "/", D_NAME(d));
	free(d);
	return 0;
}

/* list files or directories in the directory associated with inode in */
static int
testfs_ls(struct super_block *sb, struct inode *in, int recursive)
{
	off_t offset = 0;
	struct dirent *d;

	for (; (d = testfs_next_dirent(in, &offset)); free(d)) {
		struct inode *cin;

		if (d->d_inode_nr < 0)
			continue;
		cin = testfs_get_inode(sb, d->d_inode_nr);
		printf("%s%s\n", D_NAME(d),
		       (testfs_inode_get_type(cin) == I_DIR) ? "/" : "");
		if (recursive && testfs_inode_get_type(cin) == I_DIR &&
		    (strcmp(D_NAME(d), ".") != 0) &&
		    (strcmp(D_NAME(d), "..") != 0)) {
			testfs_ls(sb, cin, recursive);
		}
		testfs_put_inode(cin);
	}
	return 0;
}

static int
cmd_ls_internal(struct super_block *sb, struct context *c, int recursive)
{
	int inode_nr;
	struct inode *in;
	const char *cdir;

	if (c->nargs != 2) {
		return -EINVAL;
	}
	cdir = c->cmd[1];
	assert(c->cur_dir);
	inode_nr = testfs_dir_name_to_inode_nr(c->cur_dir, cdir);
	if (inode_nr < 0)
		return inode_nr;
	in = testfs_get_inode(sb, inode_nr);
	if (testfs_inode_get_type(in) == I_DIR) {
		testfs_ls(sb, in, recursive);
	} else {
		return -ENOTDIR;
	}
	testfs_put_inode(in);
	return 0;
}

int
testfs_make_root_dir(struct super_block *sb)
{
	return testfs_create_file_or_dir(sb, NULL, I_DIR, NULL);
}

/* returns negative value if name is not found */
int
testfs_dir_name_to_inode_nr(struct inode *dir, const char *name)
{
	struct dirent *d;
	off_t offset = 0;
	int ret = -ENOENT;

	assert(dir);
	assert(name);
	assert(testfs_inode_get_type(dir) == I_DIR);
	for (; ret < 0 && (d = testfs_next_dirent(dir, &offset)); free(d)) {
		if ((d->d_inode_nr < 0) || (strcmp(D_NAME(d), name) != 0))
			continue;
		ret = d->d_inode_nr;
	}
	return ret;
}

int
cmd_cd(struct super_block *sb, struct context *c)
{
	int inode_nr;
	struct inode *dir_inode;

	if (c->nargs != 2) {
		return -EINVAL;
	}
	inode_nr = testfs_dir_name_to_inode_nr(c->cur_dir, c->cmd[1]);
	if (inode_nr < 0)
		return inode_nr;
	dir_inode = testfs_get_inode(sb, inode_nr);
	if (testfs_inode_get_type(dir_inode) != I_DIR) {
		testfs_put_inode(dir_inode);
		return -ENOTDIR;
	}
	testfs_put_inode(c->cur_dir);
	c->cur_dir = dir_inode;
	return 0;
}

int
cmd_pwd(struct super_block *sb, struct context *c)
{
	if (c->nargs != 1) {
		return -EINVAL;
	}
	testfs_pwd(sb, c->cur_dir);
	printf("\n");
	return 0;
}

int
cmd_ls(struct super_block *sb, struct context *c)
{
	return cmd_ls_internal(sb, c, 0);
}

int
cmd_lsr(struct super_block *sb, struct context *c)
{
	return cmd_ls_internal(sb, c, 1);
}

/* create an empty file */
int
cmd_create(struct super_block *sb, struct context *c)
{
	if (c->nargs != 2) {
		return -EINVAL;
	}
	return testfs_create_file_or_dir(sb, c->cur_dir, I_FILE, c->cmd[1]);
}

int
cmd_mkdir(struct super_block *sb, struct context *c)
{
	if (c->nargs != 2) {
		return -EINVAL;
	}
	return testfs_create_file_or_dir(sb, c->cur_dir, I_DIR, c->cmd[1]);
}

/* remove a file */
int
cmd_rm(struct super_block *sb, struct context *c)
{
	int inode_nr;
	struct inode *in;

	if (c->nargs != 2) {
		return -EINVAL;
	}
	inode_nr = testfs_remove_dirent(sb, c->cur_dir, c->cmd[1]);
	if (inode_nr < 0) {
		return inode_nr;
	}
	in = testfs_get_inode(sb, inode_nr);
	testfs_remove_inode(in);
	testfs_sync_inode(c->cur_dir);
	return 0;
}

/* print stats for a file or directory */
int
cmd_stat(struct super_block *sb, struct context *c)
{
	int inode_nr;
	struct inode *in;

	if (c->nargs != 2) {
		return -EINVAL;
	}
	inode_nr = testfs_dir_name_to_inode_nr(c->cur_dir, c->cmd[1]);
	if (inode_nr < 0)
		return inode_nr;
	in = testfs_get_inode(sb, inode_nr);
	printf("%s: i_nr = %d, i_type = %d, i_size = %ju, block_count = %d\n",
	       c->cmd[1], testfs_inode_get_nr(in), testfs_inode_get_type(in),
	       testfs_inode_get_size(in), testfs_inode_get_block_count(in));
	testfs_put_inode(in);
	return 0;
}
