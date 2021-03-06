#include "idt.h"

#include "idt_int.h"
#include "syscall.h"
#include "../lib.h"
#include "../i8259.h"

#define IDT_DPL_KERNEL	0
#define IDT_DPL_USER	3

/**
 *	Create an IDT entry template
 *
 *	@param idte: Pointer to IDT entry to write to
 *	@param handler: Address of handler
 *	@param dpl: Descriptor privilege level
 */
void idt_make_entry(idt_desc_t *idte, void *handler, int dpl);

/**
 *	Create an IDT interrupt gate
 *
 *	@param idte: Pointer to IDT entry to write to
 *	@param handler: Address of handler
 *	@param dpl: Descriptor privilege level
 */
void idt_make_interrupt(idt_desc_t *idte, void *handler, int dpl);

/**
 *	Create an IDT trap gate
 *
 *	@param idte: Pointer to IDT entry to write to
 *	@param handler: Address of handler
 *	@param dpl: Descriptor privilege level
 */
void idt_make_trap(idt_desc_t *idte, void *handler, int dpl);

void idt_construct(idt_desc_t *idt) {
	int i;

	printf("Initializing IDT...\n");
	// Initialize interrupt 0x00 - 0x1f: processor exceptions
	idt_make_interrupt(idt + 0x00, &(idt_int_de), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x01, &(idt_int_reserved), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x02, &(idt_int_nmi), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x03, &(idt_int_bp), IDT_DPL_USER);
	idt_make_interrupt(idt + 0x04, &(idt_int_of), IDT_DPL_USER);
	idt_make_interrupt(idt + 0x05, &(idt_int_br), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x06, &(idt_int_ud), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x07, &(idt_int_nm), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x08, &(idt_int_df), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x09, &(idt_int_reserved), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x0a, &(idt_int_ts), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x0b, &(idt_int_np), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x0c, &(idt_int_ss), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x0d, &(idt_int_gp), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x0e, &(idt_int_pf), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x0f, &(idt_int_reserved), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x10, &(idt_int_mf), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x11, &(idt_int_ac), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x12, &(idt_int_mc), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x13, &(idt_int_xf), IDT_DPL_KERNEL);
	for (i = 0x14; i < 0x20; i++) {
		idt_make_interrupt(idt + i, &(idt_int_reserved), IDT_DPL_KERNEL);
	}

	// Initialize interrupt 0x20 - 0x2f: PIC interrupts
	idt_make_interrupt(idt + 0x20, &(idt_int_irq0), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x21, &(idt_int_irq1), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x22, &(idt_int_irq2), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x23, &(idt_int_irq3), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x24, &(idt_int_irq4), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x25, &(idt_int_irq5), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x26, &(idt_int_irq6), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x27, &(idt_int_irq7), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x28, &(idt_int_irq8), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x29, &(idt_int_irq9), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x2a, &(idt_int_irq10), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x2b, &(idt_int_irq11), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x2c, &(idt_int_irq12), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x2d, &(idt_int_irq13), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x2e, &(idt_int_irq14), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x2f, &(idt_int_irq15), IDT_DPL_KERNEL);

	// Initialize interrupt 0x80: System call
	syscall_register_all();
	idt_make_interrupt(idt + 0x80, &(idt_int_usr), IDT_DPL_USER);
}

int idt_addEventListener(unsigned type, irq_listener listener) {
	// Sanity check
	if (!listener || type >= 0x10)
		return -1;
	// Don't overwrite
	if (idt_int_irq_listeners[type] != &idt_int_irq_default)
		return -1;
	// Set handler
	idt_int_irq_listeners[type] = listener;
	// Enable IRQ
	enable_irq(type);

	return 0;
}

int idt_removeEventListener(unsigned type) {
	// Sanity check
	if (type >= 0x10)
		return -1;
	// Don't double-free
	if (idt_int_irq_listeners[type] == &idt_int_irq_default)
		return -1;
	// Reset handler
	idt_int_irq_listeners[type] = idt_int_irq_default;
	// Disable IRQ
	disable_irq(type);

	return 0;
}

irq_listener idt_getEventListener(unsigned type) {
	// Sanity check
	if (type >= 0x10)
		return NULL;
	// Default listener is NO listener
	if (idt_int_irq_listeners[type] == &idt_int_irq_default)
		return NULL;

	return idt_int_irq_listeners[type];
}

void idt_make_entry(idt_desc_t *idte, void *handler, int dpl) {
	uint32_t addr32 = (uint32_t) handler;
	(*idte).val[0] = addr32 & 0x0000ffff;
	(*idte).val[1] = addr32 & 0xffff0000;
	(*idte).seg_selector = 2 * 8; // Kernel Code Segment
	(*idte).present = 1;
	(*idte).size = 1; // Always 32 bits
	(*idte).dpl = dpl;
}

void idt_make_interrupt(idt_desc_t *idte, void *handler, int dpl) {
	idt_make_entry(idte, handler, dpl);
	(*idte).reserved4 = 0;
	(*idte).reserved3 = 0;
	(*idte).reserved2 = 1;
	(*idte).reserved1 = 1;
	(*idte).reserved0 = 0;
}

void idt_make_trap(idt_desc_t *idte, void *handler, int dpl) {
	idt_make_entry(idte, handler, dpl);
	(*idte).reserved4 = 0;
	(*idte).reserved3 = 1;
	(*idte).reserved2 = 1;
	(*idte).reserved1 = 1;
	(*idte).reserved0 = 0;
}
