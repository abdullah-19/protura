/*
 * Copyright (C) 2014 Matt Kilgore
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v2 as published by the
 * Free Software Foundation.
 */
#ifndef INCLUDE_ARCH_SYSCALL_H
#define INCLUDE_ARCH_SYSCALL_H

#define INT_SYSCALL 0x81

#define SYSCALL_PUTCHAR 0x01
#define SYSCALL_CLOCK   0x02
#define SYSCALL_GETPID  0x03
#define SYSCALL_PUTINT  0x04
#define SYSCALL_PUTSTR  0x05
#define SYSCALL_SLEEP   0x06
#define SYSCALL_FORK    0x07
#define SYSCALL_GETPPID 0x08
#define SYSCALL_OPEN    0x09
#define SYSCALL_CLOSE   0x0A
#define SYSCALL_READ    0x0B
#define SYSCALL_WRITE   0x0C
#define SYSCALL_LSEEK   0x0D
#define SYSCALL_EXEC    0x0E
#define SYSCALL_YIELD   0x0F
#define SYSCALL_EXIT    0x10
#define SYSCALL_WAIT    0x11
#define SYSCALL_DUP     0x12
#define SYSCALL_DUP2    0x13

#ifdef __KERNEL__
#ifndef ASM

#include <protura/irq.h>

void syscall_init(void);

#endif
#endif

#endif
