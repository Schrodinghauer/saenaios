/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "keyboard.h"
#include "rtc.h"
#include "pit.h"
#include "debug.h"
#include "terminal_driver/terminal_out_driver.h"
#include "terminal_driver/tty.h"
#include "tests.h"

#include "fsdriver/mp3fs_driver.h"
#include "boot/idt.h"
#include "boot/page_table.h"

#include "proc/signal.h"
#include "proc/scheduler.h"
#include "proc/task.h"
#include "fs/vfs.h"
#include "fs/fs_devfs.h"
#include "libc.h"

#include "atadriver/ata.h"
#include "fsdriver/ext4_driver.h"

//#define RUN_TESTS

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags, bit)   ((flags) & (1 << (bit)))

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void entry(unsigned long magic, unsigned long addr) {
	int mp3fs_load_addr = 0;
	multiboot_info_t *mbi;

	/* Clear the screen. */
	clear();

	printf("Hello world!\n");

	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		printf("Invalid magic number: 0x%#x\n", (unsigned)magic);
		return;
	}

	/* Set MBI to the address of the Multiboot information structure. */
	mbi = (multiboot_info_t *) addr;

	/* Print out the flags. */
	printf("flags = 0x%#x\n", (unsigned)mbi->flags);

	/* Are mem_* valid? */
	if (CHECK_FLAG(mbi->flags, 0))
		printf("mem_lower = %uKB, mem_upper = %uKB\n", (unsigned)mbi->mem_lower, (unsigned)mbi->mem_upper);

	/* Is boot_device valid? */
	if (CHECK_FLAG(mbi->flags, 1))
		printf("boot_device = 0x%#x\n", (unsigned)mbi->boot_device);

	/* Is the command line passed? */
	if (CHECK_FLAG(mbi->flags, 2))
		printf("cmdline = %s\n", (char *)mbi->cmdline);

	if (CHECK_FLAG(mbi->flags, 3)) {
		int mod_count = 0;
		int i;
		module_t* mod = (module_t*)mbi->mods_addr;
        mp3fs_load_addr = mod->mod_start;

		while ((unsigned int)mod_count < mbi->mods_count) {
			printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
			printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
			printf("First few bytes of module:\n");
			for (i = 0; i < 16; i++) {
				printf("0x%x ", *((char*)(mod->mod_start+i)));
			}
			printf("\n");
			mod_count++;
			mod++;
		}
	}
	/* Bits 4 and 5 are mutually exclusive! */
	if (CHECK_FLAG(mbi->flags, 4) && CHECK_FLAG(mbi->flags, 5)) {
		printf("Both bits 4 and 5 are set.\n");
		return;
	}

	/* Is the section header table of ELF valid? */
	if (CHECK_FLAG(mbi->flags, 5)) {
		elf_section_header_table_t *elf_sec = &(mbi->elf_sec);
		printf("elf_sec: num = %u, size = 0x%#x, addr = 0x%#x, shndx = 0x%#x\n",
				(unsigned)elf_sec->num, (unsigned)elf_sec->size,
				(unsigned)elf_sec->addr, (unsigned)elf_sec->shndx);
	}

	/* Are mmap_* valid? */
	if (CHECK_FLAG(mbi->flags, 6)) {
		memory_map_t *mmap;
		printf("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
				(unsigned)mbi->mmap_addr, (unsigned)mbi->mmap_length);
		for (mmap = (memory_map_t *)mbi->mmap_addr;
				(unsigned long)mmap < mbi->mmap_addr + mbi->mmap_length;
				mmap = (memory_map_t *)((unsigned long)mmap + mmap->size + sizeof (mmap->size)))
			printf("    size = 0x%x, base_addr = 0x%#x%#x\n    type = 0x%x,  length    = 0x%#x%#x\n",
					(unsigned)mmap->size,
					(unsigned)mmap->base_addr_high,
					(unsigned)mmap->base_addr_low,
					(unsigned)mmap->type,
					(unsigned)mmap->length_high,
					(unsigned)mmap->length_low);
	}

	/* Construct an LDT entry in the GDT */
	{
		seg_desc_t the_ldt_desc;
		the_ldt_desc.granularity = 0x0;
		the_ldt_desc.opsize      = 0x1;
		the_ldt_desc.reserved    = 0x0;
		the_ldt_desc.avail       = 0x0;
		the_ldt_desc.present     = 0x1;
		the_ldt_desc.dpl         = 0x0;
		the_ldt_desc.sys         = 0x0;
		the_ldt_desc.type        = 0x2;

		SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
		ldt_desc_ptr = the_ldt_desc;
		lldt(KERNEL_LDT);
	}

	/* Construct a TSS entry in the GDT */
	{
		seg_desc_t the_tss_desc;
		the_tss_desc.granularity   = 0x0;
		the_tss_desc.opsize        = 0x0;
		the_tss_desc.reserved      = 0x0;
		the_tss_desc.avail         = 0x0;
		the_tss_desc.seg_lim_19_16 = TSS_SIZE & 0x000F0000;
		the_tss_desc.present       = 0x1;
		the_tss_desc.dpl           = 0x0;
		the_tss_desc.sys           = 0x0;
		the_tss_desc.type          = 0x9;
		the_tss_desc.seg_lim_15_00 = TSS_SIZE & 0x0000FFFF;

		SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

		tss_desc_ptr = the_tss_desc;

		tss.ldt_segment_selector = KERNEL_LDT;
		tss.ss0 = KERNEL_DS;
		tss.esp0 = 0x800000;
		ltr(KERNEL_TSS);
	}

	/* Construct IDT */
	idt_construct(idt);
	lidt(idt_desc_ptr);

	/* Initialize Paging */
	page_ece391_init();

	// init tty
	tty_init();
	terminal_out_clear();

	/* Init the PIC */
	i8259_init();

	/* Initialize devices, memory, filesystem, enable device interrupts on the
	 * PIC, any other initialization stuff... */
	keyboard_init();
	rtc_init();
	pit_init();

	signal_init();



	/* Enable interrupts */
	/* Do not enable the following until after you have set up your
	 * IDT correctly otherwise QEMU will triple fault and simple close
	 * without showing you any output */
	//printf("Enabling Interrupts\n");
	sti();
	// Create kernel process identity
	task_create_kernel_pid();
	// install device driver fs
	devfs_installfs();
	if (!mp3fs_load_addr || mp3fs_installfs(mp3fs_load_addr)){
		printf("error installing mp3fs\n");
		while(1);
	}
	// mount filesystems
	struct sys_mount_opts mount_opts;
	syscall_mount((int)"mp3fs", (int)"/", (int)(&mount_opts));
	mp3fs_mkdir("dev", 0777);
	syscall_mount((int)"devfs", (int)"/dev", (int)(&mount_opts));
//	mp3fs_symlink("rtc", "/dev/rtc");

	// register drivers
	rtc_out_driver_register();
	keyboard_driver_register();
	terminal_out_driver_register();
	tty_driver_register();

	ata_driver_register();
	ext4_ece391_init();
	mp3fs_mkdir("ext4", 0777);
	if (syscall_mount((int)"ext4fs", (int)"/ext4", (int)(&mount_opts))){
		printf("ext4fs mount failed\n");
		while(1);
	}


#ifdef RUN_TESTS
	/* Run tests */
	launch_tests();
#endif
	// create a basic process
	task_start_kernel_pid();

	// Bye!

	/* Spin (nicely, so we don't chew up cycles) */
	printf("\nEnd of startup\n");
	while(1);
	asm volatile (".1: hlt; jmp .1;");
}
