/**
 *	@file sys/stat.h
 *
 *	data returned by the stat() function
 */
#ifndef	SYS_STAT_H_
#define SYS_STAT_H_

#include "types.h"

/// read, write, execute/search by owner
#define S_IRWXU	0700
/// read permission, owner
#define S_IRUSR	0400
/// write permission, owner
#define S_IWUSR	0200
/// execute/search permission, owner
#define S_IXUSR	0100
/// read, write, execute/search by group
#define S_IRWXG	0070
/// read permission, group
#define S_IRGRP	0040
/// write permission, group
#define S_IWGRP	0020
/// execute/search permission, group
#define S_IXGRP	0010
/// read, write, execute/search by others
#define S_IRWXO	0007
/// read permission, others
#define S_IROTH	0004
/// write permission, others
#define S_IWOTH	0002
/// execute/search permission, others
#define S_IXOTH	0001
/// set-user-ID on execution
#define S_ISUID	04000
/// set-group-ID on execution
#define S_ISGID	02000
/// on directories, restricted deletion flag
#define S_ISVTX	01000
/// read permission
#define S_IR	04
/// write permission
#define S_IW	02
/// execute permission
#define S_IX	01

/**
 *	Metadata of a file returned by `stat`
 *
 *	@see stat
 */
typedef struct stat {
	dev_t		st_dev;		///< ID of device containing file
	ino_t		st_ino; 	///< Inode number
	mode_t		st_mode;	///< File type and mode
	nlink_t		st_nlink;	///< Number of hard links
	uid_t		st_uid;		///< User ID of owner
	gid_t		st_gid;		///< Group ID of owner
	dev_t		st_rdev;	///< Device ID (if special file)
	off_t		st_size;	///< Total size, in bytes
	blksize_t	st_blksize;	///< Block size for filesystem I/O
	blkcnt_t	st_blocks;	///< Number of 512B blocks allocated

	// NOTE TYPE CHECK
	time_t st_atim;			///< Time of last access
	time_t st_mtim;			///< Time of last modification
	time_t st_ctim;			///< Time of last status change

} __attribute__((__packed__)) stat_t;

/**
 *	Get file status & info
 *
 *	Given the file path and name, and a pointer to the user allocated stat
 * 	structure, the function will fill in the stat structure if the
 *	parameters are valid, and permission is right
 *
 *	@param path: char string of the path
 *	@param buf: pointer to a user allocated stat structure
 *
 *	@return 0 for success, negative value for errors
 *
 *	@note will open symbolic link and redirect to the file linked
 */
int stat(const char* path, stat_t* buf);

/**
 *	Get file status & info
 *
 *	Given the file descriptor, and a pointer to the user allocated stat
 * 	structure, the function will fill in the stat structure if the
 *	parameters are valid, and permission is right
 *
 *	@param fd: fd of the file, of course, a valid one
 *	@param buf: pointer to a user allocated stat structure
 *
 *	@return 0 for success, negative value for errors
 */
int fstat(int fd, stat_t* buf);

/**
 *	Get symbolic link file status & info
 *
 *	Given the symbolic file path and name, and a pointer to the user allocated stat
 * 	structure, the function will fill in the stat structure if the
 *	parameters are valid, and permission is right
 *
 *	@param path: char string of the path
 *	@param buf: pointer to a user allocated stat structure
 *
 *	@return 0 for success, negative value for errors
 *
 *	@note this will fill the stat of the symbolic link file
 */
int lstat(const char* path, stat_t* buf);

#endif
