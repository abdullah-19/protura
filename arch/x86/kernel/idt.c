/*
 * Copyright (C) 2014 Matt Kilgore
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v2 as published by the
 * Free Software Foundation.
 */

#include <protura/types.h>
#include <protura/debug.h>
#include <protura/atomic.h>
#include <protura/scheduler.h>
#include <mm/memlayout.h>

#include "irq_handler.h"
#include <arch/asm.h>
#include <arch/syscall.h>
#include <arch/drivers/pic8259.h>
#include <arch/gdt.h>
#include <arch/cpu.h>
#include <arch/task.h>
#include <arch/idt.h>

static struct idt_ptr idt_ptr;
static struct idt_entry idt_entries[256] = { {0} };

struct idt_identifier {
    atomic32_t count;
    const char *name;
    enum irq_type type;
    void (*handler)(struct irq_frame *);
};

static struct idt_identifier idt_ids[256] = { { ATOMIC32_INIT(0), 0 } };

void irq_register_callback(uint8_t irqno, void (*handler)(struct irq_frame *), const char *id, enum irq_type type)
{
    struct idt_identifier *ident = idt_ids + irqno;

    ident->handler = handler;
    ident->name = id;
    ident->type = type;
}

void idt_init(void)
{
    int i;

    idt_ptr.limit = sizeof(idt_entries) - 1;
    idt_ptr.base  = (uintptr_t)&idt_entries;

    for (i = 0; i < 256; i++)
        IDT_SET_ENT(idt_entries[i], 0, _KERNEL_CS, (uint32_t)(irq_hands[i]), DPL_KERNEL);

    IDT_SET_ENT(idt_entries[INT_SYSCALL], 1, _KERNEL_CS, (uint32_t)(irq_hands[INT_SYSCALL]), DPL_USER);

    idt_flush(((uintptr_t)&idt_ptr));
}

void interrupt_dump_stats(void (*print) (const char *fmt, ...))
{
    int k;
    (print) ("Interrupt stats:\n");
    for (k = 0; k < 256; k++) {
        /* Only display IRQ's that have a handler */
        if (!idt_ids[k].handler)
            continue;

        (print) (" %s(%02d) - %d\n", idt_ids[k].name, k, atomic32_get(&idt_ids[k].count));
    }
}

void irq_global_handler(struct irq_frame *iframe)
{
    struct idt_identifier *ident = idt_ids + iframe->intno;
    struct task *t;
    struct cpu_info *cpu = cpu_get_local();
    int frame_flag = 0;

    atomic32_inc(&ident->count);

    /* Only actual INTERRUPT types increment the intr_count */
    if (ident->type == IRQ_INTERRUPT)
        cpu->intr_count++;

    /* If this is a syscall, then we just came from user-space, so we save this
     * frame into the current task's context. */
    t = cpu->current;
    if (ident->type == IRQ_SYSCALL && t) {
        frame_flag = 1;
        t->context.frame = iframe;
    }

    if (iframe->intno >= 0x20 && iframe->intno <= 0x31) {
        if (iframe->intno >= 40)
            outb(PIC8259_IO_PIC2, PIC8259_EOI);
        outb(PIC8259_IO_PIC1, PIC8259_EOI);
    }

    if (ident->handler != NULL)
        (ident->handler) (iframe);

    /* There's a possibility that interrupts are on, if this was a syscall.
     *
     * This point represents the end of the interrupt. From here we do clean-up
     * and exit to the running task */
    cli();

    /* If this flag is set, then we're at the end of an interrupt chain - unset
     * 'frame' so that the next interrupt from this task will reconize it's the
     * new first interrupt. Also note. */
    if (frame_flag)
        t->context.frame = NULL;

    /* If this isn't an interrupt irq, then we don't do have to do the
     * stuff after this. */
    if (ident->type == IRQ_INTERRUPT)
        cpu->intr_count--;

    /* Did we die? */
    if (t->killed)
        sys_exit(0);

    /* If something set the reschedule flag and we're the last interrupt
     * (Meaning, we weren't fired while some other interrupt was going on, but
     * when a task was running), then we yield the current task, which 
     * reschedules a new task to start running.
     */
    if (cpu->intr_count == 0 && cpu->reschedule) {
        scheduler_task_yield_preempt();
        cpu->reschedule = 0;
    }

    /* Is he dead yet? */
    if (t->killed)
        sys_exit(0);
}

