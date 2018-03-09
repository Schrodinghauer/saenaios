#ifndef BOOT_IDT_H
#define BOOT_IDT_H

#include "../x86_desc.h"

#define KEYBOARD_VEC    0x21
#define RTC_VEC         0x28

/**
 *	Construct IDT table
 */
void idt_construct(idt_desc_t *idt);

#endif
