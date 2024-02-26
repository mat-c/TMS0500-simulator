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
 * 1       RAM             x        x 
 * 2       x               x        x
 * 3       alu IO_out      x        I: OP: digit0, ADDR=digit[2-3]
 * 4       x               x        I/O/nothing (according OP)      
 */
 
#define RAM_WAIT_CMD 1
#define RAM_WAIT2_CMD 2
#define RAM_EXEC_CMD 4
struct ram {
    int start, end;
    unsigned char data[30][16];
    int flags;
    int cmd;
    int addr;
} reg;

static int ram_process(void *priv, struct bus *bus)
{
    struct ram *ram = priv;
    if (bus->sstate == 0 && bus->write) {
        if ((ram->flags & RAM_EXEC_CMD)) {
            /* clear 1 reg */
            if (ram->cmd == 2) {
	            memset(ram->data[ram->addr], 0, 16*1);
	            LOG (" RAM.clr1[%02d]", ram->addr + ram->start);
                ram->flags &= ~RAM_EXEC_CMD;
            }
            else if (ram->cmd == 4) {
	            memset(ram->data[ram->addr], 0, 16*10);
	            LOG (" RAM.clr10[%02d]", ram->addr + ram->start);
                ram->flags &= ~RAM_EXEC_CMD;
            }
            else if (ram->cmd == 0) {
                memcpy(bus->io, ram->data[ram->addr], sizeof(bus->io));
                LOG (" RAM.rd[%02d]=", ram->addr + ram->start);
                for (int i = 15; i >= 0; i--) LOG("%X", bus->io[i]);
                ram->flags &= ~RAM_EXEC_CMD;
            }
        }
    }
    else if (bus->sstate == 15 && !bus->write) {
        /* write ram */
        if ((ram->flags & RAM_EXEC_CMD) && ram->cmd == 1) {
            memcpy(ram->data[ram->addr], bus->io, sizeof(bus->io));
            LOG (" RAM.wr[%02d]=", ram->addr + ram->start); for (int i = 15; i >= 0; i--) LOG("%X", bus->io[i]);
            ram->flags &= ~RAM_EXEC_CMD;
        }
        if (ram->flags & RAM_WAIT2_CMD) {
            /* get cmd from io bus */
            int addr = bus->io[3] * 10 + bus->io[2];
            int cmd = bus->io[0];
            if (addr >= ram->start && addr < ram->end) {
                if (cmd <= 2 || cmd == 4) {
                    addr -= ram->start;
                    ram->addr = addr;
                    ram->cmd = cmd;
                    ram->flags |= RAM_EXEC_CMD;
                }
                else
                    LOG (" RAM.cmd=%d", cmd);
            }
            ram->flags &= ~RAM_WAIT2_CMD;
        }
        if (ram->flags & RAM_WAIT_CMD) {
            ram->flags &= ~RAM_WAIT_CMD;
            ram->flags |= RAM_WAIT2_CMD;
        }
        /* match RAM inst */
        if ((bus->irg & 0xFFFF) == 0x0AF8) {
            ram->flags |= RAM_WAIT_CMD;
        }
    }
    return 0;
}


int ram_init(struct chip *chip, int addr)
{
    struct ram *ram;

    ram = malloc(sizeof(*ram));
    ram->flags = 0;
    ram->start = addr * 30;
    ram->end = ram->start + 30;
    /* ram data are reset on power
     * not reset by rom */
    memset(ram->data, 0, 30*16);
    chip->priv = ram;
    chip->process = ram_process;
    printf("ram base 0x%x size %d\n", ram->start, 30);
    return 0;
}
