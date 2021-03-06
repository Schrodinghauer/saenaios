/**
 *	@file rtc.h
 *
 *	Header file for rtc driver
 *
 *	vim:ts=4 noexpandtab
 */

#ifndef _RTC_H
#define _RTC_H

#include "types.h"
#include "i8259.h"
#include "lib.h"
#include "fs/vfs.h"
#include "fs/fs_devfs.h"
#include "proc/scheduler.h"

/**
 * 	Time interval datatype used to invoke alarm.
 */
typedef struct itimerval {
	int it_interval;	///< New timer value after expiration. 0 for invoke only once.
	int it_value;		///< Current timer value. 0 for disabled.
} itimerval_t;

/**
 *	RTC struct used to store current RTC states.
 */
typedef struct rtc_file {
	char rtc_status;		///< Indicate if RTC is on
	int rtc_freq;			///< Indicate the rtc_frequency
	pid_t rtc_pid;			///< Indicate the rtc current pid
	itimerval_t timer;		///< Timer struct for alarm
	volatile int rtc_sleep;	///< Indicate if RTC is sleeping
} rtc_file_t;

/**
 *	RTC_IRQ_NUM
 *
 *	IRQ number for RTC hardware.
 */
#define RTC_IRQ_NUM 8

/**
 *	RTC_PORT
 *
 *	Port for writing to RTC
 */

#define RTC_PORT    0x70

/**
 *	CMOS_PORT
 *
 *	Port for reading from RTC
 */

#define CMOS_PORT   0x71

/**
 *	REG_A
 *
 *	RTC control register A.
 */

#define REG_A       0x0A

/**
 *	REG_B
 *
 *	RTC control register B.
 */

#define REG_B       0x0B

/**
 *	REG_C
 *
 *	RTC control register C.
 */

#define REG_C       0x0C

/**
 *	REG_D
 *
 *	RTC control register D.
 */

#define REG_D       0x0D

/**
 *	REG_A_NMI
 *
 *	RTC control register A with NMI enabled (disable interrupts).
 */

#define REG_A_NMI   0x8A

/**
 *	REG_B_NMI
 *
 *	RTC control register B with NMI enabled (disable interrupts).
 */

#define REG_B_NMI   0x8B

/**
 *	REG_C_NMI
 *
 *	RTC control register C with NMI enabled (disable interrupts).
 */

#define REG_C_NMI   0x8C

/**
 *	BIT_SIX
 *
 *	the sixth bit of REG_B for enabling periodic interrupt.
 */

#define BIT_SIX     0x40

/**
 *	Register rtc driver in /dev/
 *
 *	Create driver file "/dev/rtc". Enable syscall for rtc driver.
 */

int rtc_out_driver_register();

/**
 *	Initialize the rtc
 *
 *	Writes to rtc control register, enables irq8 on PIC
 */
void rtc_init();

/**
 *	Temporary rtc interrupt handler
 *
 *	Currently only calls test_interrupts() to make sure that we are able to
 *	receive interrupt. Changes video memory content
 */
void rtc_handler();

/**
 *	Temporary rtc interrupt handler
 *
 *	Currently only called by rtc_test_2 to check rtc functionality.
 *
 */
void test_rtc_handler();

/**
 *	Set rate of RTC interrupts
 *
 *	@param rate: The RTC interrupt rate (divisions)
 */
void rtc_setrate(int rate);

/**
 *	Initialize frequency to 2 Hz and enable RTC by changing rtc_status. 
 *
 *	Occured when RTC needs to be turned on.
 *
 *	@param inode: inode of the opened file.
 *	@param file:  file struct of the opened file.
 *
 */

int rtc_open(inode_t* inode, file_t* file);

/**
 *	Close opened rtc and free all the private data.
 *	
 *	Currently do nothing.
 *
 *	@param inode: inode of the opened file.
 *	@param file:  file struct of the opened file.
 *	
 */

int rtc_close(inode_t* inode, file_t* file);

/**
 *	Block rtc interrupts at a given frequency. (virtualized)
 *
 *	While reading, process will not do anyother thing until count reach 
 *	a giving frequency.
 *
 *	@param file:  file struct of the opened file.
 *	@param buf:   private data for the file.
 *	@param count: datasize of private data.
 *	@param offset: not used.
 *
 */

ssize_t rtc_read(file_t* file, uint8_t* buf, size_t count, off_t* offset);

/**
 *	Set frequency of RTC interrupts (virtualized)
 *
 *
 *	@param file:  file struct of the opened file.
 *	@param buf:   private data for the file.
 *	@param count: datasize of private data.
 *	@param offset: not used.
 *
 */

ssize_t rtc_write(file_t* file, uint8_t* buf, size_t count, off_t* offset);

/**
 *	Check if a number is a power of 2.
 *
 *	@param freq: An input number to be checked.
 */

int is_power_of_two(int freq);

/**
 * 	Check the current state of alarm timer of a process.
 *
 * 	@param value: Receive the state of the alarm timer.
 */

int getitimer(struct itimerval *value);

/**
 * 	Set process' alarm timer to a new state
 *
 *	@param value: The new state to set.
 * 	@param old_value: The previous state of the timer.
 *
 * 	@note: 	If value->it_interval set to 0, the alarm will be disabled after
 *			expiration. Otherwise, the alarm will start a new count from 
 *			it_interval's value. it_value indicates the current count.
 *
 */

int syscall_setitimer(struct itimerval *value, struct itimerval *old_value);

/**
 * 	Set process' alarm timer to sleep and wake it only once after timer expiration.
 *
 *	@param requested: The new state to set.
 * 	@param remain: The previous state of the timer.
 *
 * 	@note: 	If value->it_interval set to 0, the alarm will be disabled after
 *			expiration. Otherwise, the alarm will start a new count from 
 *			it_interval's value. it_value indicates the current count.
 *
 */
int nanosleep(struct itimerval *requested, struct itimerval *remain);

#endif /* _RTC_H */
