/*
 * Copyright (C) 2015 Matt Kilgore
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v2 as published by the
 * Free Software Foundation.
 */
#ifndef INCLUDE_DRIVERS_KEYBOARD_H
#define INCLUDE_DRIVERS_KEYBOARD_H

#include <protura/types.h>
#include <fs/char.h>

void keyboard_init(void);

extern struct file_ops keyboard_file_ops;

#endif
