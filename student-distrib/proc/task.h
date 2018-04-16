/**
 *	@file proc/task.h
 *
 *	Data structures tracking tasks/processes.
 */
#ifndef PROC_TASK_H
#define PROC_TASK_H

#include "../types.h"
#include "../fs/vfs.h"
#include "../libc.h"
#include "../boot/page_table.h"
#include "../boot/syscall.h"
#include "../boot/ece391_syscall.h"

#include "signal.h"

#define TASK_ST_NA			0	///< Process PID is not in use
#define TASK_ST_RUNNING		1	///< Process is actively running on processor
#define TASK_ST_SLEEP		2	///< Process is sleeping
#define TASK_ST_ZOMBIE		3	///< Process is awaiting parent `wait()`
#define TASK_ST_DEAD		4	///< Process is dead

#define TASK_MAX_PROC		32767	///< Maximum of concurrently-scheduled tasks
#define TASK_MAX_OPEN_FILES	16		///< Per-process limit of concurrent open files
#define TASK_MAX_PAGE_MAPS	16		///< Maximum pages a process may request

/**
 *	Structure of saved registers produced by `pusha`
 */
typedef struct s_regs {
	uint32_t magic； ///< Should be 1145141919
	uint32_t flags; ///< Saved eflags
	uint32_t edi; ///< Saved edi
	uint32_t esi; ///< Saved esi
	uint32_t ebp; ///< Saved ebp
	uint32_t esp; ///< Saved esp
	uint32_t ebx; ///< Saved ebx
	uint32_t edx; ///< Saved edx
	uint32_t ecx; ///< Saved ecx
	uint32_t eax; ///< Saved eax
} regs_t;

#define TASK_PTENT_CPONWR	0x1		///< Current page is copy-on-write

/**
 *	A mapped memory page in a process's mapped page table
 */
typedef struct s_task_ptentry {
	uint32_t vaddr; ///< Virtual address visible to the process
	uint32_t paddr; ///< Physical address to write into the page table
	uint16_t pt_flags; ///< Flags passed to `page_dir_add_*_entry`
	uint16_t priv_flags; ///< Private flags
} task_ptentry_t;

/**
 *	Structure for a process in the PID table
 *
 *	@todo Scheduler may want to insert saved state
 *	@todo Fork and Execve may want to insert memory map
 *	@todo VFS security may want to insert identity information
 */
typedef struct s_task {
	uint8_t status;		///< Current status of this task
	pid_t pid;			///< current process id
	pid_t parent;		///< parent process id
	regs_t regs;		///< Registers stored for current process
	uint32_t eip;		///< Instruction pointer for current process
	uint32_t ks_esp;	///< Esp0, aka stack bottom address of kernel stack
	int32_t flags;		///< flags stored for current process
	file_t *files[TASK_MAX_OPEN_FILES]; ///< File descriptor pool
	char args[128];		///< @deprecated Should be placed on user stack
	task_ptentry_t pages[TASK_MAX_PAGE_MAPS]; ///< Mapped pages
	task_sigact_t sigacts[SIG_MAX]; ///< Signal handlers
	uint32_t signals; ///< Pending signals
} task_t;

/**
 *	List of processes
 */
extern task_t task_list[TASK_MAX_PROC];

/**
 *	Get the PID of the currently executing process
 *
 *	@note: this process currently always return 0 for the kernel code
 *
 *	@return the PID.
 */
pid_t task_current_pid();

/**
 *	TEMPORARY: initialize pid 0 for kernel code
 *
 *	@todo Will be removed later
 */
void task_create_kernel_pid();

/**
 *	Find an available pid
 *
 *	@return the new pid, or -EAGAIN if all pids are in use (probably very bad)
 */
int16_t task_alloc_pid();

/**
 *	Fork current process
 *
 *	@return The new pid on success, or the negative of an errno on failure
 */
int syscall_fork(int, int, int);

/**
 *	Execute a new file with the current process
 *
 *	@param pathp: pointer to `char *`, the path to the executable file
 *	@param argvp: pointer to `char *[]`, an array of arguments
 *	@param envpp: pointer to `char *[]`, an array of environment variables
 *	@return The caller will no longer exist to receive the return value if the
 *			call succeeded. The negative of an errno is returned on failure.
 *	@note argv and envp are NULL-terminated arrays
 */
int syscall_execve(int pathp, int argvp, int envpp);

/**
 *	Exit current process
 *
 *	@param status: the exit code
 *	@return This call will not return
 */
int syscall__exit(int status, int, int);

/**
 *	ECE391 (`exec` style) execute wrapper
 *
 *	@param cmdlinep: pointer to `char *`, the entire command line
 *	@return 0 on success, or the negative of an error on failure
 *	@note This call emulates the ece391_execute by appending a new frame
 *		  executing `wait` onto the current user stack, so that the user code
 *		  will not resume execution until the child process has terminated.
 */
int syscall_ece391_execute(int cmdlinep, int b, int c);

/**
 *	ECE391 halt wrapper
 *
 *	@return This call will not return
 */
int syscall_ece391_halt(int, int, int);

/**
 *	Push buffer onto the user stack. Update user esp.
 *
 *	@param esp: pointer to user esp
 *	@param buf: data buffer to be pushed
 *	@param size: size in bytes of the data buffer
 *	@return 0 on success. -EFAULT if user stack overflow
 *	@note If an overflow would happen, the user stack will enter an undefined
 *		  making executing any further user code impossible, including the
 *		  SIGSEGV handler (a SIGSEGV endless recursion would occur). The caller
 *		  should directly kill the process by resetting its SIGSEGV handler to
 *		  SIG_DFL kernel default handler and send it a SIGSEGV to gracefully
 *		  report such error to its possibly waiting parent.
 */
int task_user_pushs(uint32_t *esp, uint8_t *buf, size_t size);

/**
 *	Push dword onto the user stack. Update user esp.
 *
 *	@param esp: pointer to user esp
 *	@param val: 32-bit value to be pushed
 *	@return 0 on success. -EFAULT if user stack overflow
 *	@see task_user_pushs
 */
int task_user_pushl(uint32_t *esp, uint32_t val);

#endif
