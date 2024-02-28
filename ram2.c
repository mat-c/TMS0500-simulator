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

/*
 * cycle   irg[in]         ext[in]  IO
 * 1       RCL2            x        I: addr= digit_1 * 16 + digit_0
 * 2       x               x        x
 * 3       x               x        O: data[addr]
 * 4       x               x        x
 *
 * cycle   irg[in]         ext[in]  IO
 * 1       STO2            x        I: addr= digit_1 * 16 + digit_0
 * 2       x               x        x
 * 3       x               x        I: data[addr]
 * 4       x               x        x
 */

/* 4k ram : 4096 / 16 / 4 = 64 */
#define RAM_SIZE_NUMB 64
#define RAM_WAIT_CMD 1
#define RAM_EXEC_CMD 4
struct ram {
    int start, end;
    unsigned char data[RAM_SIZE_NUMB][16];
    int flags;
    int cmd;
    int addr;
};

static int ram_process(void *priv, struct bus *bus)
{
    struct ram *ram = priv;
    if (bus->sstate == 0 && bus->write) {
        if ((ram->flags & RAM_EXEC_CMD)) {
            if (ram->cmd == 0) {
                memcpy(bus->io, ram->data[ram->addr], sizeof(bus->io));
                LOG (" RAM2.rd[%02d]=", ram->addr + ram->start);
                for (int i = 15; i >= 0; i--) LOG("%X", bus->io[i]);
                ram->flags &= ~RAM_EXEC_CMD;
            }
        }
    }
    else if (bus->sstate == 15 && !bus->write) {
        /* write ram */
        if ((ram->flags & RAM_EXEC_CMD) && ram->cmd == 1) {
            memcpy(ram->data[ram->addr], bus->io, sizeof(bus->io));
            LOG (" RAM2.wr[%02d]=", ram->addr + ram->start); for (int i = 15; i >= 0; i--) LOG("%X", bus->io[i]);
            ram->flags &= ~RAM_EXEC_CMD;
        }
        if (ram->flags & RAM_WAIT_CMD) {
            ram->flags &= ~RAM_WAIT_CMD;
            ram->flags |= RAM_EXEC_CMD;
        }
        /* match RAM inst STOR 0x0A76 / RCLR 0x0A86 */
        if ((bus->irg & 0xFFFF) == 0x0A76 || (bus->irg & 0xFFFF) == 0x0A86) {
            int addr = bus->io[1] * 16 + bus->io[0];
            if (addr >= ram->start && addr < ram->end) {
                addr -= ram->start;
                ram->addr = addr;
                ram->cmd = !(bus->irg & 0x0080);
                ram->flags |= RAM_WAIT_CMD;
            }
        }
    }
    return 0;
}


int ram2_init(struct chip *chip, int addr)
{
    struct ram *ram;

    ram = malloc(sizeof(*ram));
    ram->flags = 0;
    ram->start = addr * RAM_SIZE_NUMB;
    ram->end = ram->start + RAM_SIZE_NUMB;
    /* TODO file backend to save/restore */
    memset(ram->data, 0xE, RAM_SIZE_NUMB*16);
    chip->priv = ram;
    chip->process = ram_process;
    printf("ram base 0x%x size %d\n", ram->start, RAM_SIZE_NUMB);
    return 0;
}
