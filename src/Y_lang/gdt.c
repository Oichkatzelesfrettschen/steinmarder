#include "gdt.h"

// The actual table living in your kernel's memory
struct gdt_entry sm_gdt[5];
struct gdt_ptr sm_gdt_ptr;

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    sm_gdt[num].base_low    = (base & 0xFFFF);
    sm_gdt[num].base_middle = (base >> 16) & 0xFF;
    sm_gdt[num].base_high   = (base >> 24) & 0xFF;

    sm_gdt[num].limit_low   = (limit & 0xFFFF);
    sm_gdt[num].granularity = (limit >> 16) & 0x0F;

    sm_gdt[num].granularity |= gran & 0xF0;
    sm_gdt[num].access      = access;
}

void gdt_init() {
    // The GDT Pointer tells the CPU where the table is
    sm_gdt_ptr.limit = (sizeof(struct gdt_entry) * 5) - 1;
    sm_gdt_ptr.base  = (uint64_t)&sm_gdt;

    // 1. Null Segment (Required)
    gdt_set_gate(0, 0, 0, 0, 0);

    // 2. Kernel Code Segment (Offset 0x08)
    // Access: 0x9A (Present, Ring 0, Code, Executable, Readable)
    // Granularity: 0x20 (64-bit mode flag)
    gdt_set_gate(1, 0, 0, 0x9A, 0x20);

    // 3. Kernel Data Segment (Offset 0x10)
    // Access: 0x92 (Present, Ring 0, Data, Writable)
    gdt_set_gate(2, 0, 0, 0x92, 0x00);

    // 4. User Code Segment (Offset 0x18 - For future steinmarder Apps)
    gdt_set_gate(3, 0, 0, 0xFA, 0x20);

    // 5. User Data Segment (Offset 0x20)
    gdt_set_gate(4, 0, 0, 0xF2, 0x00);

    // Tell the CPU to use this new table (requires a small assembly tweak)
    // gdt_flush((uint64_t)&sm_gdt_ptr); 
}
