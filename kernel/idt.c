#include "idt.h"

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_pointer;

/*
 * This function will be implemented in assembly later.
 * It loads the IDT using the lidt instruction.
 */
extern void idt_load(uint32_t idt_ptr_address);

static void idt_set_gate(
    uint8_t number,
    uint32_t handler,
    uint16_t selector,
    uint8_t type_attr
)
{
    idt[number].offset_low =
        (uint16_t)(handler & 0xFFFF);

    idt[number].selector = selector;
    idt[number].zero = 0;
    idt[number].type_attr = type_attr;

    idt[number].offset_high =
        (uint16_t)((handler >> 16) & 0xFFFF);
}

void idt_init(void)
{
    int i;

    /*
     * Clear all IDT entries.
     */
    for (i = 0; i < IDT_ENTRIES; i++) {

        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].zero = 0;
        idt[i].type_attr = 0;
        idt[i].offset_high = 0;
    }

    /*
     * Configure the IDT pointer.
     */
    idt_pointer.limit =
        (uint16_t)(sizeof(idt_entry_t) * IDT_ENTRIES - 1);

    idt_pointer.base = (uint32_t)&idt;

     /*
     * Load the IDT.
     */
    idt_load((uint32_t)&idt_pointer);
}

