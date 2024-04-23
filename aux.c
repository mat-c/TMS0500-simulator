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

static struct {
    char out[17];
    int nidle;
    int data;
    int pos;
} disp;

static int display_process(void *priv, struct bus *bus)
{
    if (bus->sstate == 15 && !bus->write) {
        if (bus->irg == 0x0AE8) {
                int data = (bus->ext >> 3) & 0xFF;
                LOG("AUX.cmd=%d ", data);
                /* can set busy */
        }
        else if (bus->irg == 0x0AD8) {
                /* can set cond */
        }
    }
    return 0;
}

int aux_init(struct chip *chip, const char *name)
{
    chip->process = display_process;

    return 0;
}
