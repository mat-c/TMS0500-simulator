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
        if (!bus->idle) {
            disp.nidle++;
            disp.data = bus->io[0];
        }
        else {
            if (disp.nidle > 10 && disp.data == 0) {
                disp.pos = 15;
                LOG("DISP2 reset ");
            }
            else if (disp.nidle >= 6 && disp.pos >= 0) {
                disp.out[disp.pos] = disp.data;
                LOG("DISP2 digit[%d]=%x ", disp.pos, disp.data);
                disp.pos--;
                if (disp.pos == -1) {
                    int i;
                    int j = 0;
                    char out[20];
                    int dp = disp.out[14] + disp.out[13] * 10;
                    memset(out, ' ', sizeof(out) - 1);
                    out[sizeof(out) - 1] = 0;
                    if (disp.out[0] & 1)
                        out[j++] = '-';
                    else
                        out[j++] = ' ';

                    for (i = 12; i > 0; i--) {
                        if (dp == i)
                            out[j++] = '.';
                        out[j++] = '0' + disp.out[13-i];
                    }
                    if (dp == 0)
                        out[j++] = '.';
                    display_ext(out);
                }
            }
            disp.nidle = 0;
        }
    }
    return 0;
}

int display2_init(struct chip *chip, const char *name)
{
    chip->process = display_process;

    return 0;
}
