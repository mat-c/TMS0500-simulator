/*
 * Copyright (C) 2014 Hynek Sladky; sladky@mujmail.cz
 * Copyright (C) 2024 by Matthieu CASTET <castet.matthieu@free.fr>
 *
 * A part of this coded is base on Hynek emulator 
 * see http://hsl.wz.cz/ti_59.htm / https://www.hrastprogrammer.com/emulators.htm
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

#include "emu.h"
#include <stdio.h>
#include <ctype.h>

// ------------------------------------
int load_dump (unsigned short *buf, int buf_len, const char *name, int *base) {
    int rom_size = 0;
    int base_addr = -1;
    FILE *f;
#define	LINELEN	1024
    char line[LINELEN];
    if (!base)
        return 0;
    if ((f = fopen (name, "rt")) == NULL)
        return 0;
    while (!feof (f)) {
        unsigned data, idx = 0;
        int addr;
        if (!fgets (line, LINELEN, f))
            break;
        if (!isxdigit(line[0]) || !isxdigit(line[1]) ||
            !isxdigit(line[2]) || !isxdigit(line[3]) ||
            line[4] != ':')
            continue;
        if (!sscanf (line, "%X: ", &addr))
            continue;
        if (base_addr == -1)
            base_addr = addr;
        if (addr >= base_addr)
            addr -= base_addr;
        else {
            fprintf (stderr, "load %s: address 0x%X out of range\n", name, addr);
            continue;
        }
        while (line[idx] > ' ') idx++;
        while (line[idx] && line[idx] <= ' ') idx++;
        while (sscanf (line+idx, "%X", &data) > 0) {
            if (addr < buf_len) {
                buf[addr++] = data;
                if (rom_size < addr)
                    rom_size = addr;
            }
            else
                fprintf (stderr, "load %s: address 0x%X out of range\n", name, addr + base_addr);
            while (line[idx] > ' ') idx++;
            while (line[idx] && line[idx] <= ' ') idx++;
        }
    }
    fclose (f);
    *base = base_addr;
    return rom_size;
}

int load_dumpK (unsigned char buf[][16], int buf_len, const char *name, int *base) {
    int rom_size = 0;
    int base_addr = -1;
    FILE *f;
#define	LINELEN	1024
    char line[LINELEN];
    if (!base)
        return 0;
    if ((f = fopen (name, "rt")) == NULL)
        return 0;
    while (!feof (f)) {
        int addr, idx = 0;
        uint64_t data;
        if (!fgets (line, LINELEN, f))
            break;
        if (!isdigit(line[0]) || !isdigit(line[1]) ||
            !isdigit(line[2]) || line[3] != ':')
            continue;

        if (!sscanf (line, "%d: ", &addr))
            continue;
        if (base_addr == -1)
            base_addr = addr;
        if (addr >= base_addr)
            addr -= base_addr;
        else {
            fprintf (stderr, "load %s: address 0x%X out of range\n", name, addr);
            continue;
        }

        while (line[idx] > ' ') idx++;
        while (line[idx] && line[idx] <= ' ') idx++;
        while (sscanf (line+idx, "%lX", &data) > 0) {
            if (addr < buf_len) {
                for (int j = 0; j < 16; j++) {
                    buf[addr][j] = data & 0xf;
                    data >>= 4;
                }
                addr++;
                if (rom_size < addr)
                    rom_size = addr;
            }
            else
                fprintf (stderr, "load %s: address 0x%X too big\n", name, addr);
            while (line[idx] > ' ') idx++;
            while (line[idx] && line[idx] <= ' ') idx++;
        }
    }
    fclose (f);
    *base = base_addr;
    return rom_size;
}

#if 0
int load_dumpK (unsigned char buf[][16], int buf_len, const char *name) {
    int rom_size = 0;
    FILE *f;
#define	LINELEN	1024
    char line[LINELEN];
    if ((f = fopen (name, "rt")) == NULL)
        return 1;
    while (!feof (f)) {
        unsigned addr, data, idx = 0;
        if (!fgets (line, LINELEN, f))
            break;
        if (!sscanf (line, "%d: ", &addr))
            continue;
        while (line[idx] > ' ') idx++;
        while (line[idx] && line[idx] <= ' ') idx++;
        while (sscanf (line+idx, "%X", &data) > 0) {
            if (addr < buf_len) {
                buf[addr++] = data;
                if (rom_size < addr - 1)
                    rom_size = addr - 1;
            }
            else
                fprintf (stderr, "load %s: address 0x%X too big\n", name, addr);
            while (line[idx] > ' ') idx++;
            while (line[idx] && line[idx] <= ' ') idx++;
        }
    }
    fclose (f);
    return rom_size;
}
#endif
