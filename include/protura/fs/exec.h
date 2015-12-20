/*
 * Copyright (C) 2015 Matt Kilgore
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v2 as published by the
 * Free Software Foundation.
 */
#ifndef INCLUDE_FS_EXEC_H
#define INCLUDE_FS_EXEC_H

#include <protura/irq.h>
#include <protura/fs/inode.h>

int execve(struct inode *inode, const char *const argv[], const char *const envp[], struct irq_frame *);

#endif
