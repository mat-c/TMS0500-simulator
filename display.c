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
    char out[30];
    char out1[30];
    int pos;
} disp;

char *display_debug(void)
{
    return disp.out1;
}

int display_process(void *priv, struct bus *bus)
{
    if (bus->sstate == 0 && !bus->write) {
        if (bus->dstate <= 13 && bus->dstate >= 1) {
            if (bus->dstate == 1 && bus->display_segH)
                disp.out[0] = '-';
            if (bus->dstate == 2) {
                if (bus->display_segH)
                    disp.out[disp.pos++] = '-';
                else
                    disp.out[disp.pos++] = ' ';
            }
            if (bus->dstate != 13)
                disp.out[disp.pos++] = bus->display_digit;
            else
                disp.out[disp.pos++] = ' ';
            if (bus->display_dpt)
                disp.out[disp.pos++] = '.';
            //LOG("\nSEG.%d='%c' (%s)\n", bus->dstate, bus->display_digit, disp.out);
        }
        if (bus->dstate==0) {
            if (memcmp(disp.out1, disp.out, sizeof(disp.out1))) {
                LOG("\nDISP='%s'\n", disp.out);
                printf(" \r%s", disp.out);
                memcpy(disp.out1, disp.out, sizeof(disp.out1));
            }
            //memset(disp.out, '\0', sizeof(disp.out));
            memset(disp.out, ' ', sizeof(disp.out)-1);
            disp.pos = 0;
        }
    }
    return 0;
}
