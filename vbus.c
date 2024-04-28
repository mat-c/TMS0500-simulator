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

unsigned log_flags = 0;
FILE *log_file;

#define CHIPS_NUM_MAX 30
struct chip chipss[CHIPS_NUM_MAX] = {
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
        bus->addr = -1;
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
    printf("-r file: add rom file\n");
    printf("-s file: add scom const file\n");
    printf("-k model: cal model\n");
    printf("-R: add a ram module (can be repeated\n");
    printf("-m: add a ti58c ram module (can be repeated\n");
    printf("-p: add printer\n");
    printf("-l file: add library file (ti5x)\n");
    printf("-d: disassemble rom on stderr and exit\n");
    printf("-D: disassemble crom on stderr and exit\n");
}

int main(int argc, char *argv[])
{
    int opt;
    int i = 0;
    int ret = 0;
    int ram_addr = 0;
    int disasm = 0;
    int disasm_crom = 0;
    enum hw hw_opt;
    char *keyb_name = NULL;
    const char *options = "r:s:k:Rmpl:dDv:";

    /* first pass for debug options */
    while ((opt = getopt(argc, argv, options)) != -1) {
        switch (opt) {
        case 'd':
            disasm = 1;
            break;
        case 'D':
            disasm_crom = 1;
            break;
        case 'v':
            log_flags = atoi(optarg);
            break;
        default:
            break;
        }
    }
    log_file = stdout;
    if (log_flags) {
        FILE *f = fopen("log.txt", "a");
        if (f)
            log_file = f;
    }


    optind = 1;

    ret |= alu_init(&chipss[i++]);
    while ((opt = getopt(argc, argv, options)) != -1) {
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
            hw_opt = HW_PRINTER;
            break;
        case 'l':
            ret |= lib_init(&chipss[i++], optarg, disasm_crom);
            break;
        /*ignore debug */
        case 'd':
        case 'D':
        case 'v':
            break;
        default:
            help();
            ret = 1;
        }
        if (ret || i == CHIPS_NUM_MAX)
            break;
    }
    if (ret)
        return 1;

    if (disasm || disasm_crom) {
        return 0;
    }

    if (i + 2 > CHIPS_NUM_MAX)
        return 2;

    ret |= display_init(&chipss[i++], keyb_name);
    ret |= key_init(&chipss[i++], keyb_name, hw_opt);
    run(chipss, &bus_state);
    return 0;
}
