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

// ------------------------------------
int load_dump (unsigned short *buf, int buf_len, const char *name) {
    int rom_size = 0;
    FILE *f;
#define	LINELEN	1024
    char line[LINELEN];
    if ((f = fopen (name, "rt")) == NULL)
        return 0;
    while (!feof (f)) {
        unsigned addr, data, idx = 0;
        if (!fgets (line, LINELEN, f))
            break;
        if (!sscanf (line, "%X: ", &addr))
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

