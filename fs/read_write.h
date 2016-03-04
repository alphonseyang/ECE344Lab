#ifndef _READ_WRITE_H
#define _READ_WRITE_H

/* read data from file associated with inode in. read up to len bytes of data
 * into buffer buf, starting from offset off in file. on success, returns the
 * number of bytes read. on error, returns a negative value. */
int testfs_read_data(struct inode *in, char *buf, off_t off, size_t len);

/* write data to file associated with inode in. write len bytes of data from
 * buffer buf, starting from offset off in file. on success, returns the number
 * of bytes written. on error, returns a negative value. */
int testfs_write_data(struct inode *in, const char *buf, off_t off, size_t len);

/* free all the blocks of a file. on error, returns a negative value. */
int testfs_free_blocks(struct inode *in);

#endif /* _READ_WRITE_H */
