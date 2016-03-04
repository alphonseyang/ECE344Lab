#ifndef _DIR_H
#define _DIR_H

/* this is the directory entry structure */
struct dirent {
	s32 d_name_len;	/* length of the name in the directory entry */
	s32 d_inode_nr;	/* inode number associated with this directory entry */
};

/* the directory entry name (file or directory name) is located at the end of
 * the dirent structure */
#define D_NAME(d) ((char*)(d) + sizeof(struct dirent))

/* get next dirent in the directory */
struct dirent *testfs_next_dirent(struct inode *dir, off_t * offset);
/* create the root directory of the file system */
int testfs_make_root_dir(struct super_block *sb);
/* returns inode number of the file or directory called name in directory dir */
int testfs_dir_name_to_inode_nr(struct inode *dir, const char *name);

#endif /* _DIR_H */
