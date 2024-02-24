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

#include <string.h>
#include "emu.h"

struct scom_cons {
    unsigned char CONST[16][16];
    uint32_t fifo;
    int size;
} cons;

int scom_const_process(void *priv, struct bus *bus)
{
    if (bus->sstate == 0 && bus->write) {
        cons.fifo = cons.fifo >> 8;
        if (cons.fifo & 0x80) {
            int addr = (cons.fifo) & 0x7F;
            if (addr < cons.size)
                memcpy(bus->io, cons.CONST[addr], sizeof(bus->io));
            if (log_flags & LOG_SHORT)
                LOG (" CONST.%d=", addr); for (int i = 15; i >= 0; i--) LOG("%X", bus->io[i]);
        }
    }
    else if (bus->sstate == 15 && !bus->write) {
        if (((bus->irg & 0x10D0) == 0x00C0) && (bus->irg & 0xF00) != 0x000 &&
            (bus->irg & 0xF00) != 0x800 && (bus->irg & 0xF00) != 0xA00) {
            int addr = ((bus->ext >> 4) & 0x78) | ((bus->ext >> 3) & 0x07) | 0x80;
            cons.fifo |= addr << 8; /* 2 cycles delay */
        }
    }
    return 0;
}


void *scom_const_init(void)
{
    int ret = load_dump8(cons.CONST, sizeof(cons.CONST)/16, "rom-SR50/TMC0521B-CONST.txt");
	cons.size = ret;
    printf("const size %d\n", ret);
    return &cons;

}
