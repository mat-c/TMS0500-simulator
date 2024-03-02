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

#include "bus.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "bus.h"
#include "emu.h"


struct chip chipss[30] = {
    {.process = display_process, .priv = NULL},
    {.process = NULL, .priv = NULL},
    {.process = NULL, .priv = NULL},
    {.process = NULL, .priv = NULL},
    {.process = NULL},
};

struct bus bus_state;

int run(struct chip chips[], struct bus *bus)
{

    memset(bus, 0, sizeof(*bus));
    bus->dstate = 15;
    while (1) {
        bus->ext = 0;
        bus->irg = 0;
        memset(bus->io, 0, sizeof(bus->io));
        for (bus->sstate = 0; bus->sstate < 16; bus->sstate++) {
            int ret;
            bus->write = 1;
            for (int i = 0; chips[i].process; i++) {
                ret = chips[i].process(chips[i].priv, bus);
                if (ret) {
                    printf("%d error %d\n", i, ret);
                    return ret;
                }
            }
            bus->write = 0;
            for (int i = 0; chips[i].process; i++) {
                ret = chips[i].process(chips[i].priv, bus);
                if (ret) {
                    printf("%d error %d\n", i, ret);
                    return ret;
                }
            }
            /* dstate is updated between S14R/S15W */
            if (bus->sstate == 14) {
                bus->key_line = 0;
                if (bus->dstate)
                    bus->dstate--;
                else
                    bus->dstate = 15;
            }
        }
        if (log_flags & LOG_SHORT)
            LOG(" EXT=0x%04x IRG=0x%04x\n", bus->ext, bus->irg);
    }
    return 0;
}

static void help(void)
{
    printf("-r: rom file\n");
    printf("-s: scom const file\n");
}

int main(int argc, char *argv[])
{
    int opt;
    int i = 1;
    int ret = 0;
    int ram_addr = 0;
    int disasm = 0;
    char *keyb_name = NULL;
    ret |= alu_init(&chipss[i++]);
    while ((opt = getopt(argc, argv, "r:s:k:Rmdpl:")) != -1) {
        switch (opt) {
        case 'r':
            ret |= brom_init(&chipss[i++], optarg, disasm);
            break;
        case 's':
            ret |= scom_init(&chipss[i++], optarg);
            break;
        case 'k':
            keyb_name = optarg;
            break;
        case 'R':
            ret |= ram_init(&chipss[i++], ram_addr++);
            break;
        case 'm':
            ret |= ram2_init(&chipss[i++], ram_addr++);
            break;
        case 'p':
            ret |= printer_init(&chipss[i++]);
            break;
        case 'l':
            ret |= lib_init(&chipss[i++], optarg);
            break;
        case 'd':
            disasm = 1;
            break;
        default:
            help();
        }
        if (ret)
            break;
    }
    if (ret)
        return 1;

    if (disasm) {
        return 0;
    }
    ret |= key_init(&chipss[i++], keyb_name);
    run(chipss, &bus_state);
    return 0;
}
