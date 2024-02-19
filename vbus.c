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


struct chip {
    void *(*init)(int cs);
    int (*process)(void *priv, struct bus *bus);
    void *priv;
    int (*dump_state)(void *priv, struct bus *bus, FILE *f);
};


struct chip chips[] = {
};
int chip_num;

struct bus bus_state;

int run(struct chip *chips, struct bus *bus)
{

    memset(&bus, 0, sizeof(bus));
    while (1) {
        for (bus->dstate = 15; bus->dstate >= 0; bus->dstate--) {
            bus->ext = 0;
            bus->irg = 0;
            memset(bus->io, 0, sizeof(bus->io));
            for (bus->sstate = 0; bus->sstate < 16; bus->sstate++) {
                int ret;
                bus->write = 1;
                for (int i = 0; i < chip_num; i++) {
                    ret = chips[i].process(chips[i].priv, bus);
                    if (ret)
                        return ret;
                }
                bus->write = 0;
                for (int i = 0; i < chip_num; i++) {
                    ret = chips[i].process(chips[i].priv, bus);
                    if (ret)
                        return ret;
                }
            }
        }
    }
    return 0;
}
