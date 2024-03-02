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

static int display_process(void *priv, struct bus *bus)
{
    /* alu update on S0W and clear at S14W */
    if (bus->sstate == 0 && !bus->write) {
        /* 13 digits display
         * D13/D1 is - and .
         * D12 ... D3 digit/. (mantisa)
         * D2 is -
         * D2 D1 is digit (exponent)
         *
         * - is connected to segH
         */
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

            /* not really seen in rom, but hw allow it */
            if (bus->dstate == 13 && bus->display_segH)
                disp.out[0] = '-';

            if (bus->display_dpt && bus->dstate >= 3)
                disp.out[disp.pos++] = '.';
            else if (bus->display_dpt && bus->dstate == 1)
                LOG("???? dpt at D1 ");
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

static int display_process2(void *priv, struct bus *bus)
{
    /* alu update on S0W and clear at S14W */
    if (bus->sstate == 0 && !bus->write) {
        /* 12 digit connected to D13-D2
         * D13 is (C or -) and .
         */
        if (bus->dstate <= 13 && bus->dstate >= 2) {
            /* bus->display_segH is connected to C and D13
             */
            if (bus->dstate != 13)
                disp.out[disp.pos++] = bus->display_digit;
            else {
                if (bus->display_digit == '-') {
                    if (bus->display_segH)
                        disp.out[disp.pos++] = 'E';
                    else
                        disp.out[disp.pos++] = '-';
                }
                else {
                    if (bus->display_segH)
                        disp.out[disp.pos++] = 'C';
                    else
                        disp.out[disp.pos++] = ' ';
                }
            }

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

void display_print(const char *line)
{
    printf("|      %.20s\n", line);
    printf("\r%s", disp.out);
}

void display_dbgprint(const char *line)
{
    printf("|d     %.13s %s\n", disp.out1, line);
    printf("\r%s", disp.out);
}

int display_init(struct chip *chip, const char *name)
{
    if (name && (!strcmp(name, "ti58") ||
            !strcmp(name, "ti59") || !strcmp(name, "sr51-II"))) {
        chip->process = display_process2;
        printf("new display\n");
    }
    else
        chip->process = display_process;

    return 0;
}
