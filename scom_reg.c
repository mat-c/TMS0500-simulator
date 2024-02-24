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

struct scom_reg {
    unsigned char SCOM[16][16];
    uint32_t fifo;
} reg;

int scom_reg_process(void *priv, struct bus *bus)
{
    if (bus->sstate == 0 && bus->write) {
        reg.fifo = reg.fifo >> 8;
        if (reg.fifo & 0x10) {
            int addr = (reg.fifo >> 5) & 7;
            memcpy(bus->io, reg.SCOM[addr], sizeof(bus->io));
            if (log_flags & LOG_SHORT)
                LOG (" RCL.%d=", addr); for (int i = 15; i >= 0; i--) LOG("%X", bus->io[i]);
        }
    }
    else if (bus->sstate == 15 && !bus->write) {
        if ((reg.fifo & 1) && !(reg.fifo & 0x10)) {
            int addr = (reg.fifo >> 5) & 7;
            memcpy(reg.SCOM[addr], bus->io, sizeof(bus->io));
            if (log_flags & LOG_SHORT)
                LOG (" %xSTO.%d=", reg.fifo, addr); for (int i = 15; i >= 0; i--) LOG("%X", bus->io[i]);
        }
        if ((bus->irg & 0xFF0F) == 0x0A0F) {
            reg.fifo |= (bus->irg & 0xFF) << 16; /* 2 cycles delay */
        }
    }
    return 0;
}
