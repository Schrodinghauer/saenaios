/**
 *	@file unistd.h
 *
 *	File descriptor IO
 */
#ifndef UNISTD_H
#define UNISTD_H

#include "sys/types.h"
#include "sys/stat.h"

#define SEEK_SET	0 ///< Seek relative to the start of the file
#define SEEK_CUR	1 ///< Seek relative to the current position
#define SEEK_END	2 ///< Seek relative to the end of file

/**
 *	Read bytes from an open file
 *
 *	@param fd: the file descriptor to read from
 *	@param buf: pointer to the buffer to read into
 *	@param count: the maximum number of bytes to read
 *	@return the number of bytes read, or the negative of an errno on failure.
 */
ssize_t read(int fd, void *buf, size_t count);

/**
 *	Write bytes to an open file
 *
 *	This write call is always synchronous, i.e., it will not return until all
 *	data has been acknowledged by the driver.
 *
 *	@param fd: the file descriptor to write to
 *	@param buf: pointer to the data buffer
 *	@param count: the size of the buffer
 *	@return the number of bytes written, or the negative of an errno on failure.
 */
ssize_t write(int fd, const void *buf, size_t count);

/**
 *	System call handler for `lseek`: seek file pointer to given position
 *
 *	@param fd: the file descriptor to write to
 *	@param offset: the new file pointer, relative to `whence`
 *	@param whence: reference frame for `offset`. See `SEEK_*` macros.
 *	@return the absolute offset on success, or the negative of an errno on
 *	failure.
 */
off_t lseek(int fd, off_t offset, int whence);

/**
 *	Duplicate current process
 *
 *	After calling fork, the current process will be duplicated by creating new
 *	page table entries referencing the same physical addresses. The first
 *	attempt to write to these pages will then trigger copy-on-write.
 *
 *	@return the new PID to the calling process on successful, 0 to the newly
 *			forked process, or the negative of an errno on failure
 */
pid_t fork();

/**
 *	Reload current process with the given executable image.
 *
 *	@param path: path to the executable file (ELF)
 *	@param argv: list of command-line arguments, the last element must be NULL
 *				 to signify end-of-list
 *	@param envp: list of environmental variables, the last element must be NULL
 *				 to signify end-of-list
 *	@return the negative of an errno on failure. On success, the new process
 *			is launched and the old process will not be present to receive any
 *			return values.
 */
int execve(const char *path, char *const argv[], char *const envp[]);

/**
 *	Terminate the calling process with given exit status code
 *
 *	This function will always succeed. After calling, the calling process will
 *	no longer exist to receive any returned values.
 *
 *	@param status: the exit status code
 */
void _exit(int status);

/**
 *	Get current process ID
 *
 *	@return the current pid
 */
pid_t getpid();

/**
 *	Change permission of file
 *
 *	@param path: path to the file
 *	@param mode: the new mode to set
 *	@return 0 on success, or -1 on failure. Set errno
 */
int chmod(const char *path, mode_t mode);

/**
 *	Change permission of file from file descriptor
 *
 *	@param fd: the file descriptor
 *	@param mode: the new mode
 *	@return 0 on success, or -1 on failure. Set errno
 */
int fchmod(int fd, mode_t mode);

/**
 *	Change owner of file
 *
 *	@param path: path to the file
 *	@param uid: the new owner user ID
 *	@param gid: the new owner group ID
 *	@return 0 on success, or -1 on failure. Set errno
 */
int chown(const char *path, uid_t uid, gid_t gid);

/**
 *	Change owner of file from file descriptor
 *
 *	@param fd: the file descriptor
 *	@param uid: the new owner user ID
 *	@param gid: the new owner group ID
 *	@return 0 on success, or -1 on failure. Set errno
 */
int fchown(int fd, uid_t uid, gid_t gid);

/**
 *	Create new hard link
 *
 *	@param path1: the file to create link to (source)
 *	@param path2: the path of the link (destination)
 *	@return 0 on success, or -1 on failure. Set errno
 */
int link(const char *path1, const char *path2);

/**
 *	Delete file (remove directory entry)
 *
 *	@param path: the file to unlink
 *	@return 0 on success, or -1 on failure. Set errno
 */
int unlink(const char *path);

/**
 *	Create symbolic link to file
 *
 *	@param path1: the file to create link to (source)
 *	@param path2: the path of the link (destination)
 *	@return 0 on success, or -1 on failure. Set errno
 */
int symlink(const char *path1, const char *path2);

/**
 *	Read the contents of symbolic link
 *	@param path: the symlink file
 *	@param buf: pointer to the buffer to read into
 *	@param count: the maximum number of bytes to read
 *	@return the number of bytes read, or -1 on failure. Set errno.
 */
ssize_t readlink(const char *path, char *buf, size_t bufsize);

/**
 *	Truncate or extend a file from a file descriptor to a specified length
 *
 *	@param fd: the file descriptor
 *	@param length: the new length of the file
 *	@return 0 on success, or -1 on failure. Set errno
 */
int ftruncate(int fd, off_t length);

/**
 *	Truncate or extend a file to a specified length
 *
 *	@param path: the file
 *	@param length: the new length of the file
 *	@return 0 on success, or -1 on failure. Set errno
 */
int truncate(const char *path, off_t length);

/**
 *	Move file
 *
 *	@param oldpath: file to move
 *	@param newpath: new name/location of file
 *	@return 0 on success, or -1 on failure. Set errno
 */
int rename(const char *oldpath, const char *newpath);

/**
 *	Get current working directory
 *
 *	@param buf: the buffer to read cwd into
 *	@param size: the max size of the buffer
 *	@return buf on success, or NULL on failure. Set errno
 */
char *getcwd(char *buf, size_t size);

/**
 *	Change current working directory
 *
 *	@param path: path to the new working directory
 *	@return 0 on success, or -1 on failure. Set errno
 */
int chdir(const char *path);

/**
 *	Create new directory
 *
 *	@param path: path to the new directory
 *	@param mode: permission bits of the new directory file
 *	@return 0 on success, or -1 on failure. Set errno
 */
int mkdir(const char *path, mode_t mode);

/**
 *	Remove directory
 *
 *	@param path: path to the directory to be removed
 *	@return 0 on success, or -1 on failure. Set errno
 */
int rmdir(const char *path);

#endif
