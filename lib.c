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

#define MAX_DATA 5000

#define WAIT_IN_DATA 1
#define WAIT_IN_PC 2
#define WAIT_IN_DATA_HIGH 4
struct lib {
    int pc;
    unsigned char data[MAX_DATA];
    int flags;
    int flags_delay;
};


/*
 * 0 1010 ____ 1110
 *
 *
 * 0 : "IN\tLIB",
 * 1 : "OUT\tLIB_PC",
 * 2 : "IN\tLIB_PC",
 * 3 : "IN\tLIB_HIGH",
 *
 * cycle   irg[in]         ext[in]  IO
 * 1       OUT LIB_PC      I:pc     x #data in ext[3-6]
 *
 * 1       IN  LIB_PC      x        x
 * 2       MOV KR,EXT      x        x
 * 3       x               O:pc     x #data out ext[3-6]
 *
 * 1       IN  LIB         x        x
 * 2       MOV KR,EXT      x        x
 * 3       x               O:data   x #data out ext[3-10]
 *
 * 1       IN  LIB_HIGH    x        x
 * 2       MOV KR,EXT      x        x
 * 3       x               O:data   x #data out ext[3-6]
 */
 

static int lib_process(void *priv, struct bus *bus)
{
    struct lib *lib = priv;
    if (bus->sstate == 15 && bus->write) {
        if (lib->flags_delay & WAIT_IN_DATA) {
            int data = lib->data[lib->pc];
            bus->ext |= data << 3;
		    LOG ("LIB.data[%04d]=%02x ", lib->pc, data);
            lib->pc++;
            if (lib->pc >= MAX_DATA)
                lib->pc = 0;
        }
        else if (lib->flags_delay & WAIT_IN_PC) {
            bus->ext |= (lib->pc % 10) << 3;
            lib->pc /= 10;
		    LOG ("LIB.unload_pc=%04d ", lib->pc);
        }
        else if (lib->flags_delay & WAIT_IN_DATA_HIGH) {
            int data = lib->data[lib->pc] >> 4;
            bus->ext |= data << 3;
		    LOG ("LIB.data_high[%04d]=%02x ", lib->pc, data);
        }
    }
    else if (bus->sstate == 15 && !bus->write) {
        
        /* delay one cycle */
        lib->flags_delay = lib->flags;
        lib->flags = 0;
        /* instruction decode */
        switch (bus->irg) {
            case 0x0A0E:
                /* output data */
                lib->flags |= WAIT_IN_DATA;
                break;
            case 0x0A1E:
            {
                /* set pc */
                int pc_h = (bus->ext >> 3) & 0xF;
                lib->pc = lib->pc / 10 + pc_h * 1000;
		        LOG ("LIB.load_pc=%04d ", lib->pc);
                break;
            }
            case 0x0A2E:
                /* output pc */
                lib->flags |= WAIT_IN_PC;
                break;
            case 0x0A3E:
                lib->flags |= WAIT_IN_DATA_HIGH;
                /* output data high */
                break;
        }
    }
    return 0;
}



int lib_init(struct chip *chip, const char *name)
{
    struct lib *lib;
    int size;

    lib = malloc(sizeof(*lib));
    if (!lib)
        return -1;

    lib->pc = 0xDE;
    lib->flags = lib->flags_delay = 0;
    memset(lib->data, 0x92, sizeof(lib->data));
    size = load_dump8(lib->data, sizeof(lib->data), name);
#if 0
    if (!size || size > sizeof(lib->data)) {
        printf("lib invalid\n");
        free(lib);
        return -1;
    }

    if (sizeof(lib->data) - size) {
        /* fill remaining data with op code 92 = Return */
        memset(lib->data + size, 0x92, sizeof(lib->data) - size);
    }
#else
    if (size != sizeof(lib->data)) {
        printf("lib invalid %d\n", size);
        free(lib);
        return -1;
    }
#endif

    chip->priv = lib;
    chip->process = lib_process;
    return 0;
}
