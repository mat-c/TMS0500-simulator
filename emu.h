/*
 * Copyright (C) 2024 by Matthieu CASTET <castet.matthieu@free.fr>
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "bus.h"

#define SR50
#define TEST_MODE


// ====================================
// Log control
// ====================================
#define	LOG_SHORT	0x0001
#define	LOG_HRAST	0x0002
#define	LOG_DEBUG	0x0004

extern FILE *log_file;
extern unsigned log_flags;
#define	DIS(...)	fprintf (log_file, __VA_ARGS__)
#define	LOG(...)	do { if (log_flags & LOG_SHORT) fprintf (log_file, __VA_ARGS__); } while (0)

void disasm (unsigned addr, unsigned opcode);


int alu_process(void *priv, struct bus *bus);
void alu_init(void);


int brom_process(void *priv, struct bus *bus);
void *brom_init(const char *name);

int load_dump (unsigned short *buf, int buf_len, const char *name, int *base);
int load_dump8 (unsigned char buf[][16], int buf_len, const char *name);


char *display_debug(void);
int display_process(void *priv, struct bus *bus);
int key_process(void *priv, struct bus *bus);

int scom_reg_process(void *priv, struct bus *bus);


int scom_const_process(void *priv, struct bus *bus);
void *scom_const_init(void);
