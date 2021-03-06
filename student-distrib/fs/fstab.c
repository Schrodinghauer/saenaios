#include "fstab.h"

#include "../errno.h"
#include "../lib.h"
#include "../proc/task.h"
#include "file_lookup.h"

#include "../../libc/src/syscalls.h" // Definitions from libc

#define FSTAB_MAX_FS	16
#define FSTAB_MAX_MNT	16

file_system_t fstab_fs[FSTAB_MAX_FS];
vfsmount_t fstab_mnt[FSTAB_MAX_MNT];

int fstab_register_fs(file_system_t *fs) {
	int avail_idx = -1, i;

	if (!fs || fs->name[0] == '\0') {
		return -EFAULT;
	}

	for (i = 0; i < FSTAB_MAX_FS; i++) {
		if (avail_idx < 0 && fstab_fs[i].name[0] == '\0') {
			avail_idx = i;
		} else if (strncmp(fstab_fs[i].name, fs->name, FSTAB_FS_NAME_LEN)==0) {
			return -EEXIST;
		}
	}

	if (avail_idx < 0) {
		return -ENFILE;
	}

	memcpy(fstab_fs + avail_idx, fs, sizeof(file_system_t));

	return 0;
}

int fstab_unregister_fs(const char *name) {
	int i;

	if (!name) {
		return -EFAULT;
	}

	for (i = 0; i < FSTAB_MAX_FS; i++) {
		if (strncmp(fstab_fs[i].name, name, FSTAB_FS_NAME_LEN)==0) {
			fstab_fs[i].name[0] = '\0'; // Mark as deleted
			return 0;
		}
	}

	return -ENOENT;
}

file_system_t *fstab_get_fs(const char *name) {
	int i;

	if (!name) {
		errno = EFAULT;
		return NULL;
	}

	for (i = 0; i < FSTAB_MAX_FS; i++) {
		if (strncmp(fstab_fs[i].name, name, FSTAB_FS_NAME_LEN)==0) {
			return fstab_fs + i;
		}
	}

	errno = ENOENT;
	return NULL;
}

vfsmount_t *fstab_get_mountpoint(const char *path, int *offset) {
	int max_match = 0, max_idx = -1, i, j;
	char *mntname;

	if (!path || !offset) {
		errno = EFAULT;
		return NULL;
	}
	if (path[0] != '/') {
		errno = EINVAL;
		return NULL;
	}

	for (i = 0; i < FSTAB_MAX_MNT; i++) {
		mntname = fstab_mnt[i].mountpoint;
		for (j = 0; path[j] == mntname[j] && path[j] != '\0'; j++);
		if (j > max_match && mntname[j] == '\0') {
			max_match = j;
			max_idx = i;
		}
	}

	if (max_idx < 0) {
		errno = ENOENT;
		return NULL;
	}

	*offset = max_match;
	return fstab_mnt + max_idx;
}

int syscall_mount(int typeaddr, int destaddr, int optaddr) {
	char *type;
	pathname_t dest = "/";
	struct sys_mount_opts *opts;
	int i, avail_idx = -1;
	file_system_t *fs = NULL;
	task_t *proc;
	inode_t *mntnode;

	if (!typeaddr || !destaddr || !optaddr) {
		return -EFAULT;
	}

	// mount call privilege check
	proc = task_list + task_current_pid();
	if (proc->uid != 0 && proc->gid != 0) {
		return -EPERM;
	}

	type = (char *) typeaddr;
	opts = (struct sys_mount_opts *) optaddr;
	errno = -path_cd(dest, (char *)destaddr);
	if (errno != 0) {
		return -errno;
	}

	// Lookup filesystem by name
	for (i = 0; i < FSTAB_MAX_FS; i++) {
		if (strncmp(type, fstab_fs[i].name, FSTAB_FS_NAME_LEN) == 0) {
			fs = fstab_fs + i;
			break;
		}
	}
	if (!fs) {
		return -ENODEV;
	}

	// Mountpoint permission check
	i = strlen(dest);
	if (dest[i-1] == '/') {
		dest[i-1] = '\0';
		i--;
	}
	// Check mountpoint permissions if not mounting rootfs
	if (i != 0) {
		mntnode = file_lookup(dest);
		if (mntnode == NULL) {
			// Cannot access mount point
			return -errno;
		}
		if (mntnode->file_type != FTYPE_DIRECTORY) {
			(*mntnode->sb->s_op->free_inode)(mntnode);
			return -ENOTDIR;
		}
		// Check for RWX permission on the mount point
		errno = -file_permission(mntnode, proc->uid, proc->gid, 7);
		if (errno != 0) {
			(*mntnode->sb->s_op->free_inode)(mntnode);
			return -EPERM;
		}
		// OK. Release inode
		(*mntnode->sb->s_op->free_inode)(mntnode);
	}
	dest[i] = '/';
	dest[i+1] = '\0';


	for (i = 0; i < FSTAB_MAX_MNT; i++) {
		if (avail_idx < 0 && fstab_mnt[i].mountpoint[0] == '\0') {
			avail_idx = i;
		} else if (strncmp(dest, fstab_mnt[i].mountpoint, PATH_MAX_LEN) == 0) {
			return -EEXIST;
		}
	}

	if (avail_idx<0) {
		return -ENFILE;
	}

	strcpy(fstab_mnt[avail_idx].mountpoint, dest);

	fstab_mnt[avail_idx].sb = (*fs->get_sb)(fs, opts->mountflags,
													opts->source, opts->opts);
	fstab_mnt[avail_idx].open_count = 0;
	fstab_mnt[avail_idx].sb->root = fstab_mnt[avail_idx].sb->root;

	return 0;
}

int syscall_umount(int targetaddr, int b, int c) {
	pathname_t target = "/";
	int i;
	task_t *proc;

	if (!targetaddr) {
		return -EFAULT;
	}

	errno = -path_cd(target, (char *)targetaddr);
	if (errno != 0) {
		return -errno;
	}

	// mount call privilege check
	proc = task_list + task_current_pid();
	if (proc->status != TASK_ST_RUNNING) {
		return -ESRCH;
	}
	if (proc->uid != 0 && proc->gid != 0) {
		return -EPERM;
	}

	for (i = 0; i < FSTAB_MAX_MNT; i++) {
		if (strncmp(target, fstab_mnt[i].mountpoint, PATH_MAX_LEN) == 0) {
			if (fstab_mnt[i].open_count != 0) {
				// There are open files
				return -EBUSY;
			}
			fstab_mnt[i].sb->fstype->kill_sb(fstab_mnt[i].sb);
			fstab_mnt[i].mountpoint[0] = '\0';
			return 0;
		}
	}
	return -ENOENT;
}
