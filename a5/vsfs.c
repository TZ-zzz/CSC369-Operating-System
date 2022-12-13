/*
 * This code is provided solely for the personal and private use of students
 * taking the CSC369H course at the University of Toronto. Copying for purposes
 * other than this use is expressly prohibited. All forms of distribution of
 * this code, including but not limited to public repositories on GitHub,
 * GitLab, Bitbucket, or any other online platform, whether as given or with
 * any changes, are expressly prohibited.
 *
 * Authors: Alexey Khrabrov, Karen Reid, Angela Demke Brown
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2022 Angela Demke Brown
 */

/**
 * CSC369 Assignment 5 - vsfs driver implementation.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

// Using 2.9.x FUSE API
#define FUSE_USE_VERSION 29
#include <fuse.h>

#include "vsfs.h"
#include "fs_ctx.h"
#include "options.h"
#include "util.h"
#include "bitmap.h"
#include "map.h"

//NOTE: All path arguments are absolute paths within the vsfs file system and
// start with a '/' that corresponds to the vsfs root directory.
//
// For example, if vsfs is mounted at "/tmp/my_userid", the path to a
// file at "/tmp/my_userid/dir/file" (as seen by the OS) will be
// passed to FUSE callbacks as "/dir/file".
//
// Paths to directories (except for the root directory - "/") do not end in a
// trailing '/'. For example, "/tmp/my_userid/dir/" will be passed to
// FUSE callbacks as "/dir".


/**
 * Initialize the file system.
 *
 * Called when the file system is mounted. NOTE: we are not using the FUSE
 * init() callback since it doesn't support returning errors. This function must
 * be called explicitly before fuse_main().
 *
 * @param fs    file system context to initialize.
 * @param opts  command line options.
 * @return      true on success; false on failure.
 */
static bool vsfs_init(fs_ctx *fs, vsfs_opts *opts)
{
	size_t size;
	void *image;
	
	// Nothing to initialize if only printing help
	if (opts->help) {
		return true;
	}

	// Map the disk image file into memory
	image = map_file(opts->img_path, VSFS_BLOCK_SIZE, &size);
	if (image == NULL) {
		return false;
	}

	return fs_ctx_init(fs, image, size);
}

/**
 * Cleanup the file system.
 *
 * Called when the file system is unmounted. Must cleanup all the resources
 * created in vsfs_init().
 */
static void vsfs_destroy(void *ctx)
{
	fs_ctx *fs = (fs_ctx*)ctx;
	if (fs->image) {
		munmap(fs->image, fs->size);
		fs_ctx_destroy(fs);
	}
}

/** Get file system context. */
static fs_ctx *get_fs(void)
{
	return (fs_ctx*)fuse_get_context()->private_data;
}

/* Returns the inode number for the element at the end of the path
 * if it exists.  If there is any error, return -1.
 * Possible errors include:
 *   - The path is not an absolute path
 *   - An element on the path cannot be found
 */
static int path_lookup(const char *path,  vsfs_ino_t *ino) {
	if(path[0] != '/') {
		fprintf(stderr, "Not an absolute path\n");
		return -1;
	} 

	// TODO: complete this function and any helper functions
	if (strcmp(path, "/") == 0) {
		*ino = VSFS_ROOT_INO;
		return 0;
	}
	char* path_copy = strdup(path);
	char* token = strtok(path_copy, "/");
	fs_ctx* fs = get_fs();
	vsfs_inode* current_inode = &(fs->itable[VSFS_ROOT_INO]);

	for (int i = 0; i < VSFS_NUM_DIRECT; i++){
		if ((current_inode->i_direct[i] == VSFS_INO_MAX) || (!bitmap_isset(fs->dbmap, fs->sb->num_blocks, current_inode->i_direct[i]))){
			break;
		}
		vsfs_dentry* entry = (vsfs_dentry*) (fs->image + current_inode->i_direct[i] * VSFS_BLOCK_SIZE);
		for (int j = 0; j < (int) (VSFS_BLOCK_SIZE / sizeof(vsfs_dentry)); j++){
			if (strcmp(entry[j].name, token) == 0 && entry[j].ino != VSFS_INO_MAX){
				current_inode = &(fs->itable[entry[j].ino]);
				token = strtok(NULL, "/");
				if (token == NULL){
					*ino = entry[j].ino;
					return 0;
				}
			}
		}
	}

	if (current_inode->i_indirect == 0){
		return -1;
	}
	vsfs_blk_t* indirect = (vsfs_blk_t*) (fs->image + current_inode->i_indirect * VSFS_BLOCK_SIZE);
	for (int i = 0; i < (int) (VSFS_BLOCK_SIZE / sizeof(vsfs_blk_t)); i++){
		if ((indirect[i] == VSFS_INO_MAX) || (!bitmap_isset(fs->dbmap, fs->sb->num_blocks, indirect[i]))){
			break;
		}
		vsfs_dentry* entry = (vsfs_dentry*) (fs->image + indirect[i] * VSFS_BLOCK_SIZE);
		for (int j = 0; j < (int) (VSFS_BLOCK_SIZE / sizeof(vsfs_dentry)); j++){
			if (strcmp(entry[j].name, token) == 0 && entry[j].ino != VSFS_INO_MAX){
				current_inode = &(fs->itable[entry[j].ino]);
				token = strtok(NULL, "/");
				if (token == NULL){
					*ino = entry[j].ino;
					return 0;
				}
				break;
			}
		}
	}

	return -1;
}


static vsfs_inode * vsfs_get_inode(const char *path)
{
	vsfs_ino_t ino;
	fs_ctx *fs = get_fs();
	int ret = path_lookup(path, &ino);
	if ((ino == VSFS_INO_MAX) | (ret == -1)) {
		return NULL;
	}
	vsfs_inode *inode = &(fs->itable[ino]);
	return inode;
}

/**
 * Get file system statistics.
 *
 * Implements the statvfs() system call. See "man 2 statvfs" for details.
 * The f_bfree and f_bavail fields should be set to the same value.
 * The f_ffree and f_favail fields should be set to the same value.
 * The following fields can be ignored: f_fsid, f_flag.
 * All remaining fields are required.
 *
 * Errors: none
 *
 * @param path  path to any file in the file system. Can be ignored.
 * @param st    pointer to the struct statvfs that receives the result.
 * @return      0 on success; -errno on error.
 */
static int vsfs_statfs(const char *path, struct statvfs *st)
{
	(void)path;// unused
	fs_ctx *fs = get_fs();
	vsfs_superblock *sb = fs->sb; /* Get ptr to superblock from context */
	
	memset(st, 0, sizeof(*st));
	st->f_bsize   = VSFS_BLOCK_SIZE;   /* Filesystem block size */
	st->f_frsize  = VSFS_BLOCK_SIZE;   /* Fragment size */
	// The rest of required fields are filled based on the information 
	// stored in the superblock.
        st->f_blocks = sb->num_blocks;     /* Size of fs in f_frsize units */
        st->f_bfree  = sb->free_blocks;    /* Number of free blocks */
        st->f_bavail = sb->free_blocks;    /* Free blocks for unpriv users */
	st->f_files  = sb->num_inodes;     /* Number of inodes */
        st->f_ffree  = sb->free_inodes;    /* Number of free inodes */
        st->f_favail = sb->free_inodes;    /* Free inodes for unpriv users */

	st->f_namemax = VSFS_NAME_MAX;     /* Maximum filename length */

	return 0;
}

/**
 * Get file or directory attributes.
 *
 * Implements the lstat() system call. See "man 2 lstat" for details.
 * The following fields can be ignored: st_dev, st_ino, st_uid, st_gid, st_rdev,
 *                                      st_blksize, st_atim, st_ctim.
 * All remaining fields are required.
 *
 * NOTE: the st_blocks field is measured in 512-byte units (disk sectors);
 *       it should include any metadata blocks that are allocated to the 
 *       inode (for vsfs, that is the indirect block). 
 *
 * NOTE2: the st_mode field must be set correctly for files and directories.
 *
 * Errors:
 *   ENAMETOOLONG  the path or one of its components is too long.
 *   ENOENT        a component of the path does not exist.
 *   ENOTDIR       a component of the path prefix is not a directory.
 *
 * @param path  path to a file or directory.
 * @param st    pointer to the struct stat that receives the result.
 * @return      0 on success; -errno on error;
 */
static int vsfs_getattr(const char *path, struct stat *st)
{
	if (strlen(path) >= VSFS_PATH_MAX) return -ENAMETOOLONG;
	// fs_ctx *fs = get_fs();

	memset(st, 0, sizeof(*st));

	//TODO: lookup the inode for given path and, if it exists, fill in the
	// required fields based on the information stored in the inode.

	// vsfs_ino_t ino = VSFS_INO_MAX;
	// int ret = path_lookup(path, &ino);
	
	// if (ret == -1){
	// 	assert(ino == VSFS_INO_MAX);
	// 	return -ENOENT;
	// }
	// vsfs_inode *inode = &(fs->itable[ino]);

	vsfs_inode * inode = vsfs_get_inode(path);
	if (inode == NULL){
		return -ENOENT;
	}

	st->st_mode = inode->i_mode;
	st->st_nlink = inode->i_nlink;
	st->st_size = inode->i_size;
	st->st_blocks = inode->i_blocks * VSFS_BLOCK_SIZE / 512;
	if (inode->i_blocks >= VSFS_NUM_DIRECT){
		st->st_blocks += VSFS_BLOCK_SIZE / 512;
	}
	st->st_mtim = inode->i_mtime;
	return 0;
}

/**
 * Read a directory.
 *
 * Implements the readdir() system call. Should call filler(buf, name, NULL, 0)
 * for each directory entry. See fuse.h in libfuse source code for details.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a directory.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a filler() call failed).
 *
 * @param path    path to the directory.
 * @param buf     buffer that receives the result.
 * @param filler  function that needs to be called for each directory entry.
 *                Pass 0 as offset (4th argument). 3rd argument can be NULL.
 * @param offset  unused.
 * @param fi      unused.
 * @return        0 on success; -errno on error.
 */
static int vsfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi)
{
	(void)offset;// unused
	(void)fi;// unused
	fs_ctx *fs = get_fs();

	//TODO: lookup the directory inode for given path and iterate through its
	// directory entries
	vsfs_inode * inode = vsfs_get_inode(path);

	int num_entries = div_round_up(inode->i_size, sizeof(vsfs_dentry));
	int num_blocks = inode->i_blocks;
	int dentry_per_block = VSFS_BLOCK_SIZE / sizeof(vsfs_dentry);

	// check the direct blocks
	for (int i = 0; i < VSFS_NUM_DIRECT; i++){
		if (num_blocks == 0){
			break;
		}
		vsfs_dentry * dentry = (vsfs_dentry *) (fs->image + inode->i_direct[i] * VSFS_BLOCK_SIZE);
		for (int j = 0; j < dentry_per_block; j++){

			if (dentry[j].ino != VSFS_INO_MAX) {
				if (filler(buf, dentry[j].name, NULL, 0) != 0){
					return -ENOMEM;
				}
				num_entries--;
				if (num_entries == 0) {
					break;
				}
			}
		}
		num_blocks--;
	}

	// inode has more than VSFS_NUM_DIRECT blocks, check indirect block
	if (num_blocks > 0){
		vsfs_blk_t * indirect = (vsfs_blk_t *) (fs->image + inode->i_indirect * VSFS_BLOCK_SIZE);
		for (int i = 0; i < num_blocks; i++){
			vsfs_dentry * dentry = (vsfs_dentry *) (fs->image + indirect[i] * VSFS_BLOCK_SIZE);
			for (int j = 0; j < dentry_per_block; j++){

				if (dentry[j].ino != VSFS_INO_MAX) {
					if (filler(buf, dentry[j].name, NULL, 0) != 0){
						return -ENOMEM;
					}
					num_entries--;
					if (num_entries == 0) {
						return 0;
					}
				}
			}
		}
	}
	return 0;

}

/**
 * Create a directory.
 *
 * Implements the mkdir() system call.
 *
 * You do NOT need to implement this function. 
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" doesn't exist.
 *   The parent directory of "path" exists and is a directory.
 *   "path" and its components are not too long.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *
 * @param path  path to the directory to create.
 * @param mode  file mode bits.
 * @return      0 on success; -errno on error.
 */
static int vsfs_mkdir(const char *path, mode_t mode)
{
	mode = mode | S_IFDIR;
	fs_ctx *fs = get_fs();

	//OMIT: create a directory at given path with given mode
	(void)path;
	(void)mode;
	(void)fs;
	return -ENOSYS;
}

/**
 * Remove a directory.
 *
 * Implements the rmdir() system call.
 *
 * You do NOT need to implement this function. 
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a directory.
 *
 * Errors:
 *   ENOTEMPTY  the directory is not empty.
 *
 * @param path  path to the directory to remove.
 * @return      0 on success; -errno on error.
 */
static int vsfs_rmdir(const char *path)
{
	fs_ctx *fs = get_fs();

	//OMIT: remove the directory at given path (only if it's empty)
	(void)path;
	(void)fs;
	return -ENOSYS;
}

/**
 * Create a file.
 *
 * Implements the open()/creat() system call.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" doesn't exist.
 *   The parent directory of "path" exists and is a directory.
 *   "path" and its components are not too long.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *
 * @param path  path to the file to create.
 * @param mode  file mode bits.
 * @param fi    unused.
 * @return      0 on success; -errno on error.
 */
static int vsfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	(void)fi;// unused
	assert(S_ISREG(mode));
	fs_ctx *fs = get_fs();

	//TODO: create a file at given path with given mode
	char * path_copy = strdup(path);
	char * token = strtok(path_copy, "/");
	vsfs_inode * parent = &fs->itable[VSFS_ROOT_INO];
	vsfs_dentry * last_block;
	// find the last block of parent directory
	if (parent->i_blocks <= VSFS_NUM_DIRECT) {
		last_block = (vsfs_dentry *) (fs->image + parent->i_direct[parent->i_blocks - 1] * VSFS_BLOCK_SIZE);
	}
	else{
		vsfs_blk_t * indirect = (vsfs_blk_t *) (fs->image + parent->i_indirect * VSFS_BLOCK_SIZE);
		last_block = (vsfs_dentry *) (fs->image + indirect[parent->i_blocks - VSFS_NUM_DIRECT - 1] * VSFS_BLOCK_SIZE);
	}

	int entry_per_block = VSFS_BLOCK_SIZE / sizeof(vsfs_dentry);
	vsfs_dentry * new;
	int i;
	for (i = 0; i < entry_per_block; i++){
		if (last_block[i].ino == VSFS_INO_MAX){
			new = &last_block[i];
			break;
		}
	}
	// the last block is full and need a new block or put it in indirect block
	if (i == entry_per_block){
		vsfs_dentry * new_block;
		// root directory haven't used all direct blocks
		if (parent->i_blocks < VSFS_NUM_DIRECT){
			if (bitmap_alloc(fs->dbmap, fs->sb->num_blocks, &parent->i_direct[parent->i_blocks]) != 0){
				return -ENOSPC;
			}
			new_block = (vsfs_dentry *) (fs->image + parent->i_direct[parent->i_blocks] * VSFS_BLOCK_SIZE);
		}
		// root directory have used all direct blocks but haven't used indirect block
		else if (parent->i_blocks == VSFS_NUM_DIRECT){
			if (bitmap_alloc(fs->dbmap, fs->sb->num_blocks, &parent->i_indirect) != 0){
				return -ENOSPC;
			}
			fs->sb->free_blocks--;
			vsfs_blk_t * indirect_block = (vsfs_blk_t *) (fs->image + parent->i_indirect * VSFS_BLOCK_SIZE);
			if (bitmap_alloc(fs->dbmap, fs->sb->num_blocks, &indirect_block[0]) != 0){
				return -ENOSPC;
			}
			new_block = (vsfs_dentry *) (fs->image + indirect_block[0] * VSFS_BLOCK_SIZE);
		}
		// root directory have used all direct blocks and use indirect block
		else{
			if (parent->i_blocks == VSFS_NUM_DIRECT + VSFS_BLOCK_SIZE / sizeof(vsfs_blk_t)){ // used up all blocks, the fs is full
				return -ENOSPC;
			}
			vsfs_blk_t * indirect = (vsfs_blk_t *) (fs->image + parent->i_indirect * VSFS_BLOCK_SIZE);
			if (bitmap_alloc(fs->dbmap, fs->sb->num_blocks, &indirect[parent->i_blocks - VSFS_NUM_DIRECT]) != 0){
				return -ENOSPC;
			}
			new_block = (vsfs_dentry *) (fs->image + indirect[parent->i_blocks - VSFS_NUM_DIRECT] * VSFS_BLOCK_SIZE);
		}
		// update parent directory and superblock information
		parent->i_blocks++;
		parent->i_size += VSFS_BLOCK_SIZE;
		fs->sb->free_blocks--;

		// initialize the new block
		for (int j = 0; j < entry_per_block; j++){
			new_block[j].ino = VSFS_INO_MAX;
		}
		// our new entry is the first entry in the new block
		new = &new_block[0];
	}
	// update parent directory modify time
	if (clock_gettime(CLOCK_REALTIME, &parent->i_mtime) != 0){
		perror("clock_gettime");
	}
	// allocate a new inode for the new file
	if (bitmap_alloc(fs->ibmap, fs->sb->num_inodes, &new->ino) != 0){
		return -ENOSPC;
	}
	fs->sb->free_inodes--;
	strncpy(new->name, token, VSFS_NAME_MAX);
	vsfs_inode * new_inode = &fs->itable[new->ino];

	// initialize the new inode
	new_inode->i_mode = mode;
	new_inode->i_size = 0;
	new_inode->i_blocks = 0;
	new_inode->i_nlink = 1;
	if (clock_gettime(CLOCK_REALTIME, &new_inode->i_mtime) != 0){
		perror("clock_gettime");
	}

	return 0;
}

/**
 * Remove a file.
 *
 * Implements the unlink() system call.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * Errors: none
 *
 * @param path  path to the file to remove.
 * @return      0 on success; -errno on error.
 */
static int vsfs_unlink(const char *path)
{
	fs_ctx *fs = get_fs();

	//TODO: remove the file at given path
	(void)path;
	(void)fs;
	return -ENOSYS;
}


/**
 * Change the modification time of a file or directory.
 *
 * Implements the utimensat() system call. See "man 2 utimensat" for details.
 *
 * NOTE: You only need to implement the setting of modification time (mtime).
 *       Timestamp modifications are not recursive. 
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists.
 *
 * Errors: none
 *
 * @param path   path to the file or directory.
 * @param times  timestamps array. See "man 2 utimensat" for details.
 * @return       0 on success; -errno on failure.
 */
static int vsfs_utimens(const char *path, const struct timespec times[2])
{
	fs_ctx *fs = get_fs();
	vsfs_inode *ino = NULL;
	
	//TODO: update the modification timestamp (mtime) in the inode for given
	// path with either the time passed as argument or the current time,
	// according to the utimensat man page
	(void)path;
	(void)fs;
	(void)ino;
	
	// 0. Check if there is actually anything to be done.
	if (times[1].tv_nsec == UTIME_OMIT) {
		// Nothing to do.
		return 0;
	}

	// 1. TODO: Find the inode for the final component in path

	
	// 2. Update the mtime for that inode.
	//    This code is commented out to avoid failure until you have set
	//    'ino' to point to the inode structure for the inode to update.
	if (times[1].tv_nsec == UTIME_NOW) {
		//if (clock_gettime(CLOCK_REALTIME, &(ino->i_mtime)) != 0) {
			// clock_gettime should not fail, unless you give it a
			// bad pointer to a timespec.
		//	assert(false);
		//}
	} else {
		//ino->i_mtime = times[1];
	}

	//return 0;
	return -ENOSYS;
}

/**
 * Change the size of a file.
 *
 * Implements the truncate() system call. Supports both extending and shrinking.
 * If the file is extended, the new uninitialized range at the end must be
 * filled with zeros.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *   EFBIG   write would exceed the maximum file size. 
 *
 * @param path  path to the file to set the size.
 * @param size  new file size in bytes.
 * @return      0 on success; -errno on error.
 */
static int vsfs_truncate(const char *path, off_t size)
{
	fs_ctx *fs = get_fs();

	//TODO: set new file size, possibly "zeroing out" the uninitialized range
	vsfs_inode * inode = vsfs_get_inode(path);
	uint32_t old_size = inode->i_size;
	if ((uint32_t) size > (VSFS_NUM_DIRECT + (VSFS_BLOCK_SIZE / sizeof(vsfs_blk_t))) * VSFS_BLOCK_SIZE){
		return -EFBIG;
	}
	if (old_size < size) {
		uint32_t num_blocks = div_round_up(size, VSFS_BLOCK_SIZE);
		if (num_blocks == inode->i_blocks){
			// no need to allocate new blocks
			inode->i_size = size;
			if (clock_gettime(CLOCK_REALTIME, &inode->i_mtime) != 0){
				perror("clock_gettime");
			}
			return 0;
		}
		// allocate new blocks
		else {
			int blocks_need = num_blocks - div_round_up(old_size, VSFS_BLOCK_SIZE);
			assert(blocks_need >= 0);
			int direct_block_need = 0;
			int indirect_block_need = 0;
			if (inode->i_blocks < VSFS_NUM_DIRECT){
				direct_block_need = (blocks_need <= VSFS_NUM_DIRECT - (int) inode->i_blocks) ? blocks_need : VSFS_NUM_DIRECT - (int) inode->i_blocks;
				indirect_block_need = (blocks_need - direct_block_need >= 0) ? blocks_need - direct_block_need : 0;
			}
			else {
				indirect_block_need = blocks_need;
			}
			// allocate direct blocks
			if (direct_block_need > 0){
				for (int i = 0; i < direct_block_need; i++){
					if (bitmap_alloc(fs->dbmap, fs->sb->num_blocks, &inode->i_direct[inode->i_blocks]) != 0){
						return -ENOSPC;
					}
					fs->sb->free_blocks--;
					assert(inode->i_direct[inode->i_blocks] != 0);
					memset(fs->image + inode->i_direct[inode->i_blocks] * VSFS_BLOCK_SIZE, 0, VSFS_BLOCK_SIZE);
					inode -> i_blocks++;

				}
			}
			if (indirect_block_need > 0) {
				if (inode->i_blocks == VSFS_NUM_DIRECT){
					if (bitmap_alloc(fs->dbmap, fs->sb->num_blocks, &inode->i_indirect) != 0){
						return -ENOSPC;
					}
					fs->sb->free_blocks--;
				}
				vsfs_blk_t * indirect_block = (vsfs_blk_t *) (fs->image + inode->i_indirect * VSFS_BLOCK_SIZE);
				for (int i = 0; i < indirect_block_need; i++){
					if (bitmap_alloc(fs->dbmap, fs->sb->num_blocks, &indirect_block[inode->i_blocks - VSFS_NUM_DIRECT]) != 0){
						return -ENOSPC;
					}
					fs->sb->free_blocks--;
					assert(indirect_block[inode->i_blocks - VSFS_NUM_DIRECT] != 0);
					memset(fs->image + indirect_block[inode->i_blocks - VSFS_NUM_DIRECT] * VSFS_BLOCK_SIZE, 0, VSFS_BLOCK_SIZE);
					inode->i_blocks++;
				}
			}
			int n_blocks = div_round_up(size, VSFS_BLOCK_SIZE);
			for (int i = 0; i < n_blocks - VSFS_NUM_DIRECT; i++){
				assert(inode->i_indirect != 0);
				vsfs_blk_t * indirect_block = (vsfs_blk_t *) (fs->image + inode->i_indirect * VSFS_BLOCK_SIZE);
				assert(indirect_block[i] != 0);
			}

			// update inode
			inode->i_size = size;
			if (clock_gettime(CLOCK_REALTIME, &(inode->i_mtime)) != 0) {
				perror("clock_gettime");
			}
			
		}

	}
	else{
		if (div_round_up(old_size, VSFS_BLOCK_SIZE) == div_round_up(size, VSFS_BLOCK_SIZE)){
			inode->i_size = size;
			if (clock_gettime(CLOCK_REALTIME, &(inode->i_mtime)) != 0) {
				perror("clock_gettime");
			}
			return 0;
		}
		else{
			vsfs_blk_t old_blocks = inode->i_blocks;
			uint32_t num_blocks = old_blocks - div_round_up(size, VSFS_BLOCK_SIZE);
			int direct_block_free = 0;
			int indirect_block_free = 0;
			if (inode->i_blocks > VSFS_NUM_DIRECT){
				direct_block_free = (div_round_up(size, VSFS_BLOCK_SIZE) > VSFS_NUM_DIRECT) ? 0 : VSFS_NUM_DIRECT - div_round_up(size, VSFS_BLOCK_SIZE);
				indirect_block_free = (div_round_up(size, VSFS_BLOCK_SIZE) > VSFS_NUM_DIRECT) ? num_blocks : num_blocks - direct_block_free;
			}
			else {
				direct_block_free = num_blocks;
			}
			if (indirect_block_free > 0){
				indirect_block_free = ((inode->i_blocks - VSFS_NUM_DIRECT) < num_blocks - direct_block_free) ? inode->i_blocks - VSFS_NUM_DIRECT : num_blocks - direct_block_free; 
				vsfs_blk_t * indirect_block = (vsfs_blk_t *) (fs->image + inode->i_indirect * VSFS_BLOCK_SIZE);
				for (int i = 0; i < indirect_block_free; i++){
					vsfs_blk_t index = indirect_block[inode->i_blocks - VSFS_NUM_DIRECT - 1];
					bitmap_free(fs->dbmap, fs->sb->num_blocks, index);
					indirect_block[index] = 0;
					fs->sb->free_blocks++;
					inode->i_blocks --;
				}
				if (inode->i_blocks == VSFS_NUM_DIRECT){
					bitmap_free(fs->dbmap, fs->sb->num_blocks, inode->i_indirect);
					inode->i_indirect = 0;
					fs->sb->free_blocks++;
				}
			}
			if (direct_block_free > 0){
				for (int i = 0; i < direct_block_free; i++){
					bitmap_free(fs->dbmap, fs->sb->num_blocks, inode->i_direct[VSFS_NUM_DIRECT - 1 - i]);
					inode->i_direct[inode->i_blocks - 1] = 0;
					fs->sb->free_blocks++;
					inode->i_blocks --;
				}
			}
			inode->i_size = size;
			if (clock_gettime(CLOCK_REALTIME, &(inode->i_mtime)) != 0) {
				perror("clock_gettime");
			}
		}
		
	}
	return 0;
}



/**
 * Read data from a file.
 *
 * Implements the pread() system call. Must return exactly the number of bytes
 * requested except on EOF (end of file). Reads from file ranges that have not
 * been written to must return ranges filled with zeros. You can assume that the
 * byte range from offset to offset + size is contained within a single block.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * Errors: none
 *
 * @param path    path to the file to read from.
 * @param buf     pointer to the buffer that receives the data.
 * @param size    buffer size (number of bytes requested).
 * @param offset  offset from the beginning of the file to read from.
 * @param fi      unused.
 * @return        number of bytes read on success; 0 if offset is beyond EOF;
 *                -errno on error.
 */
static int vsfs_read(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi)
{
	(void)fi;// unused
	fs_ctx *fs = get_fs();

	//TODO: read data from the file at given offset into the buffer
	vsfs_inode * inode = vsfs_get_inode(path);

	if ((vsfs_blk_t) offset > inode->i_size){
		return 0; 
	}
	else if (offset + size > inode->i_size){
		size = inode->i_size - offset;
	}
	if (size == 0){
		return 0;
	}
	vsfs_blk_t start = offset / VSFS_BLOCK_SIZE;
	off_t start_offset = offset % VSFS_BLOCK_SIZE;
	vsfs_blk_t end = (offset + size) / VSFS_BLOCK_SIZE;
	off_t end_offset = (offset + size) % VSFS_BLOCK_SIZE;
	if (start < VSFS_NUM_DIRECT){
		if (end < VSFS_NUM_DIRECT){
			for (vsfs_blk_t i = start; i <= end; i++){
				if (i == start){
					memcpy(buf, fs->image + inode->i_direct[i] * VSFS_BLOCK_SIZE + start_offset, VSFS_BLOCK_SIZE - start_offset);
				}
				else if (i == end){
					memcpy(buf + (i - start) * VSFS_BLOCK_SIZE - start_offset, fs->image + inode->i_direct[i] * VSFS_BLOCK_SIZE, end_offset);
				}
				else {
					memcpy(buf + (i - start) * VSFS_BLOCK_SIZE - start_offset, fs->image + inode->i_direct[i] * VSFS_BLOCK_SIZE, VSFS_BLOCK_SIZE);
				}
			}
		}
		else {
			for (vsfs_blk_t i = start; i < VSFS_NUM_DIRECT; i++){
				if (i == start){
					memcpy(buf, fs->image + inode->i_direct[i] * VSFS_BLOCK_SIZE + start_offset, VSFS_BLOCK_SIZE - start_offset);
				}
				else {
					memcpy(buf + (i - start) * VSFS_BLOCK_SIZE - start_offset, fs->image + inode->i_direct[i] * VSFS_BLOCK_SIZE, VSFS_BLOCK_SIZE);
				}
			}
			vsfs_blk_t * indirect_block = (vsfs_blk_t *) (fs->image + inode->i_indirect * VSFS_BLOCK_SIZE);
			for (vsfs_blk_t i = VSFS_NUM_DIRECT; i <= end; i++){
				if (i == end){
					memcpy(buf + (i - start) * VSFS_BLOCK_SIZE - start_offset, fs->image + indirect_block[i - VSFS_NUM_DIRECT] * VSFS_BLOCK_SIZE, end_offset);
				}
				else {
					memcpy(buf + (i - start) * VSFS_BLOCK_SIZE - start_offset, fs->image + indirect_block[i - VSFS_NUM_DIRECT] * VSFS_BLOCK_SIZE, VSFS_BLOCK_SIZE);
				}
			}
		}
	}
	else {
		vsfs_blk_t * indirect_block = (vsfs_blk_t *) (fs->image + inode->i_indirect * VSFS_BLOCK_SIZE);
		for (vsfs_blk_t i = start; i <= end; i++){
			if (i == start){
				memcpy(buf, fs->image + indirect_block[i - VSFS_NUM_DIRECT] * VSFS_BLOCK_SIZE + start_offset, VSFS_BLOCK_SIZE - start_offset);
			}
			else if (i == end){
				memcpy(buf + (i - start) * VSFS_BLOCK_SIZE - start_offset, fs->image + indirect_block[i - VSFS_NUM_DIRECT] * VSFS_BLOCK_SIZE, end_offset);
			}
			else {
				memcpy(buf + (i - start) * VSFS_BLOCK_SIZE - start_offset, fs->image + indirect_block[i - VSFS_NUM_DIRECT] * VSFS_BLOCK_SIZE, VSFS_BLOCK_SIZE);
			}
		}
	}
	return size;
}

/**
 * Write data to a file.
 *
 * Implements the pwrite() system call. Must return exactly the number of bytes
 * requested except on error. If the offset is beyond EOF (end of file), the
 * file must be extended. If the write creates a "hole" of uninitialized data,
 * the new uninitialized range must filled with zeros. You can assume that the
 * byte range from offset to offset + size is contained within a single block.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *   EFBIG   write would exceed the maximum file size 
 *
 * @param path    path to the file to write to.
 * @param buf     pointer to the buffer containing the data.
 * @param size    buffer size (number of bytes requested).
 * @param offset  offset from the beginning of the file to write to.
 * @param fi      unused.
 * @return        number of bytes written on success; -errno on error.
 */
static int vsfs_write(const char *path, const char *buf, size_t size,
                      off_t offset, struct fuse_file_info *fi)
{
	(void)fi;// unused
	fs_ctx *fs = get_fs();

	//TODO: write data from the buffer into the file at given offset, possibly
	// "zeroing out" the uninitialized range
	vsfs_inode * inode = vsfs_get_inode(path);
	if (offset + size > (VSFS_NUM_DIRECT + VSFS_BLOCK_SIZE / sizeof(vsfs_blk_t)) * VSFS_BLOCK_SIZE){
		return -EFBIG;
	}
	vsfs_blk_t old_size = inode->i_size;
	vsfs_blk_t old_blocks = inode->i_blocks;
	if ((vsfs_blk_t) (offset + size) >= inode->i_size || (offset + size) / VSFS_BLOCK_SIZE >= inode->i_blocks){
		vsfs_truncate(path, offset + size); 
	}
	if (size == 0){
		return 0;
	}
	vsfs_blk_t start = offset / VSFS_BLOCK_SIZE;
	vsfs_blk_t end = (offset + size) / VSFS_BLOCK_SIZE;
	vsfs_blk_t start_offset = offset % VSFS_BLOCK_SIZE;
	vsfs_blk_t end_offset = (offset + size) % VSFS_BLOCK_SIZE;
	if (start < VSFS_NUM_DIRECT){
		if (end < VSFS_NUM_DIRECT){
			for (vsfs_blk_t i = start; i <= end; i++){
				if (i == start){
					assert(inode->i_direct[i] != 0);
					memcpy(fs->image + inode->i_direct[i] * VSFS_BLOCK_SIZE + start_offset, buf, VSFS_BLOCK_SIZE - start_offset);
				}
				else if (i == end){
					assert(inode->i_direct[i] != 0);
					memcpy(fs->image + inode->i_direct[i] * VSFS_BLOCK_SIZE, buf + (i - start) * VSFS_BLOCK_SIZE - start_offset, end_offset);
				}
				else {
					assert(inode->i_direct[i] != 0);
					memcpy(fs->image + inode->i_direct[i] * VSFS_BLOCK_SIZE, buf + (i - start) * VSFS_BLOCK_SIZE - start_offset, VSFS_BLOCK_SIZE);
				}
			}
		}
		else {
			for (vsfs_blk_t i = start; i < VSFS_NUM_DIRECT; i++){
				if (i == start){
					assert(inode->i_direct[i] != 0);
					memcpy(fs->image + inode->i_direct[i] * VSFS_BLOCK_SIZE + start_offset, buf, VSFS_BLOCK_SIZE - start_offset);
				}
				else {
					assert(inode->i_direct[i] != 0);
					memcpy(fs->image + inode->i_direct[i] * VSFS_BLOCK_SIZE, buf + (i - start) * VSFS_BLOCK_SIZE - start_offset, VSFS_BLOCK_SIZE);
				}
			}
			vsfs_blk_t * indirect_block = (vsfs_blk_t *) (fs->image + inode->i_indirect * VSFS_BLOCK_SIZE);
			for (vsfs_blk_t i = VSFS_NUM_DIRECT; i <= end; i++){
				if (i == end){
					assert(indirect_block[i - VSFS_NUM_DIRECT] != 0);
					memcpy(fs->image + indirect_block[i - VSFS_NUM_DIRECT] * VSFS_BLOCK_SIZE, buf + (i - start) * VSFS_BLOCK_SIZE - start_offset, end_offset);
				}
				else {
					assert(indirect_block[i - VSFS_NUM_DIRECT] != 0);
					memcpy(fs->image + indirect_block[i - VSFS_NUM_DIRECT] * VSFS_BLOCK_SIZE, buf + (i - start) * VSFS_BLOCK_SIZE - start_offset, VSFS_BLOCK_SIZE);
				}
			}
		}
	}
	else {
		vsfs_blk_t * indirect_block = (vsfs_blk_t *) (fs->image + inode->i_indirect * VSFS_BLOCK_SIZE);
		for (vsfs_blk_t i = start; i <= end; i++){
			if (i == start){
				assert(indirect_block[i - VSFS_NUM_DIRECT] != 0);
				memcpy(fs->image + indirect_block[i - VSFS_NUM_DIRECT] * VSFS_BLOCK_SIZE + start_offset, buf, VSFS_BLOCK_SIZE - start_offset);
			}
			else if (i == end){
				assert(indirect_block[i - VSFS_NUM_DIRECT] != 0);
				memcpy(fs->image + indirect_block[i - VSFS_NUM_DIRECT] * VSFS_BLOCK_SIZE, buf + (i - start) * VSFS_BLOCK_SIZE - start_offset, end_offset);
			}
			else {
				assert(indirect_block[i - VSFS_NUM_DIRECT] != 0);
				memcpy(fs->image + indirect_block[i - VSFS_NUM_DIRECT] * VSFS_BLOCK_SIZE, buf + (i - start) * VSFS_BLOCK_SIZE - start_offset, VSFS_BLOCK_SIZE);
			}
		}
	}
	// if there is a hole
	if (old_size < offset){
		if (old_blocks <= VSFS_NUM_DIRECT){
			memset(fs->image + inode->i_direct[old_blocks - 1] * VSFS_BLOCK_SIZE, 0, old_size % VSFS_BLOCK_SIZE);
			// if (start < VSFS_NUM_DIRECT){
			// 	for (vsfs_blk_t i = old_blocks; i < start; i++){
			// 		memset(fs->image + inode->i_direct[i] * VSFS_BLOCK_SIZE, 0, VSFS_BLOCK_SIZE);
			// 	}
			// 	memset(fs->image + inode->i_direct[start] * VSFS_BLOCK_SIZE, 0, start_offset);
			// }
			// else {
			// 	// for (vsfs_blk_t i = old_blocks; i < VSFS_NUM_DIRECT; i++){
			// 	// 	memset(fs->image + inode->i_direct[i] * VSFS_BLOCK_SIZE, 0, VSFS_BLOCK_SIZE);
			// 	// }
			// 	// vsfs_blk_t * indirect_block = (vsfs_blk_t *) (fs->image + inode->i_indirect * VSFS_BLOCK_SIZE);
			// 	// for (vsfs_blk_t i = VSFS_NUM_DIRECT; i < start; i++){
			// 	// 	memset(fs->image + indirect_block[i - VSFS_NUM_DIRECT] * VSFS_BLOCK_SIZE, 0, VSFS_BLOCK_SIZE);
			// 	// }
			// 	memset(fs->image + indirect_block[start - VSFS_NUM_DIRECT] * VSFS_BLOCK_SIZE, 0, start_offset);
			// }
		}
		else {
			vsfs_blk_t * indirect_block = (vsfs_blk_t *) (fs->image + inode->i_indirect * VSFS_BLOCK_SIZE);
			// for (vsfs_blk_t i = old_blocks; i < start; i++){
			// 	memset(fs->image + indirect_block[i - VSFS_NUM_DIRECT] * VSFS_BLOCK_SIZE, 0, VSFS_BLOCK_SIZE);
			// }
			memset(fs->image + indirect_block[old_blocks - 1 - VSFS_NUM_DIRECT] * VSFS_BLOCK_SIZE, 0, old_size % VSFS_BLOCK_SIZE);
			// memset(fs->image + indirect_block[start - VSFS_NUM_DIRECT] * VSFS_BLOCK_SIZE, 0, start_offset);
		}
	}

	return size;
}



static struct fuse_operations vsfs_ops = {
	.destroy  = vsfs_destroy,
	.statfs   = vsfs_statfs,
	.getattr  = vsfs_getattr,
	.readdir  = vsfs_readdir,
	.mkdir    = vsfs_mkdir,
	.rmdir    = vsfs_rmdir,
	.create   = vsfs_create,
	.unlink   = vsfs_unlink,
	.utimens  = vsfs_utimens,
	.truncate = vsfs_truncate,
	.read     = vsfs_read,
	.write    = vsfs_write,
};

int main(int argc, char *argv[])
{
	vsfs_opts opts = {0};// defaults are all 0
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	if (!vsfs_opt_parse(&args, &opts)) return 1;

	fs_ctx fs = {0};
	if (!vsfs_init(&fs, &opts)) {
		fprintf(stderr, "Failed to mount the file system\n");
		return 1;
	}

	return fuse_main(args.argc, args.argv, &vsfs_ops, &fs);
}
