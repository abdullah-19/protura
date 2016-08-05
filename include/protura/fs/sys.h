/*
 * Copyright (C) 2015 Matt Kilgore
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v2 as published by the
 * Free Software Foundation.
 */
#ifndef INCLUDE_FS_SYS_H
#define INCLUDE_FS_SYS_H

#include <protura/irq.h>
#include <protura/fs/stat.h>
#include <protura/fs/inode.h>
#include <protura/fs/file.h>
#include <protura/mm/user_ptr.h>
#include <protura/fs/sync.h>

#define PATH_MAX 256

int sys_open(const char *__user file, int flags, mode_t mode);
int sys_close(int fd);
int sys_read(int fd, void *__user buf, size_t len);
int sys_write(int fd, void *__user buf, size_t len);
off_t sys_lseek(int fd, off_t off, int whence);
int sys_read_dent(int fd, struct dent *__user dent, size_t size);
int sys_pipe(int *fds);

int sys_truncate(const char *__user file, off_t length);
int sys_ftruncate(int fd, off_t length);
int sys_link(const char *old, const char *new);
int sys_unlink(const char *name);
int sys_chdir(const char *__user path);
int sys_stat(const char *__user path, struct stat *buf);
int sys_fstat(int fd, struct stat *buf);
int sys_mkdir(const char *__user name, mode_t mode);
int sys_mknod(const char *__user node, mode_t mode, dev_t dev);
int sys_rmdir(const char *__user name);

/* Called internally by sys_open - Performs the same function, but takes
 * arguments relating to inode's and file's rather then a path name and
 * userspace flags. If you have to use sys_open in the kernel. */
int __sys_open(struct inode *inode, unsigned int file_flags, struct file **filp);

int sys_execve(const char *__user executable, const char *const __user argv[], const char *const __user envp[], struct irq_frame *);

#endif
