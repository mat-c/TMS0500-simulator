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

struct scom {
    /* v1 16/ v2 32 */
    unsigned char CONST[16*2][16];
    uint32_t fifo_const;
    int end_const;
    int start_const;
    /* v1 2/ v2 8 */
    unsigned char SCOM[2*4][16];
    uint32_t fifo_reg;
    int end_reg;
    int start_reg;
};

/**
 * Scom handle
 * STO/RCL
 * STO2/RCL2 [28x new instruction]
 * constant io
 * intermal brom
 *
 * theory of operation
 * ext[0-15] KR[1] x x KR[4-15] KR[0]
 * addr[0-3] = EXT[3-5] EXT[7]
 * cs[0-2] = EXT[8-10]
 * irg = 0x0_C_ or 0x_E_ and not 0x0A__/0x00__/0x08__
 * cycle   irg[in]         ext[in]  IO[out]
 * 1       constant io     KR       x
 * 2       x               x        constant[KR] #there is a CS on KR
 * 3       x               x        x
 *
 * ...
 *
 * irg = 0x0A_F
 * cycle   irg[in]         ext[in]  IO[out]
 * 1       RCL_n           x        x
 * 2       x               x        x
 * 3       x               x        data[n] #there is a CS on n
 * 4       x               x        x
 *
 * cycle   irg[in]         ext[in]  IO[in]
 * 1       STO_n           x        x
 * 2       x               x        x
 * 3       x               x        data[n] #there is a CS on n
 * 4       x               x        x
 *
 * only work if ROM is in access SCOM ? No. Init in rom0, reset with STO 0-15
 * cycle   irg[in]         ext[in]  IO
 * 1       RCL2            x        I: addr= digit 0
 * 2       x               x        x
 * 3       x               x        O: data[addr] #there is a CS on addr
 * 4       x               x        x
 *
 * cycle   irg[in]         ext[in]  IO
 * 1       STO2            x        I: addr= digit 0
 * 2       x               x        x
 * 3       x               x        I: data[addr] #there is a CS on addr
 * 4       x               x        x
 */


static int scom_const_process(struct scom *scom, struct bus *bus)
{
    if (bus->sstate == 0 && bus->write) {
        scom->fifo_const >>= 8;
        /* check if fifo is valid */
        if (scom->fifo_const & 0x80) {
            int addr = (scom->fifo_const) & 0x7F;
            memcpy(bus->io, scom->CONST[addr], sizeof(bus->io));
            if (log_flags & LOG_SHORT)
                LOG (" CONST.%d=", addr); for (int i = 15; i >= 0; i--) LOG("%X", bus->io[i]);
        }
    }
    else if (bus->sstate == 15 && !bus->write) {
        /* match alu scomt instruction */
        if (((bus->irg & 0x10D0) == 0x00C0) && (bus->irg & 0xF00) != 0x000 &&
            (bus->irg & 0xF00) != 0x800 && (bus->irg & 0xF00) != 0xA00) {
            int addr = ((bus->ext >> 4) & 0x78) | ((bus->ext >> 3) & 0x07);
            if (addr >= scom->start_const && addr < scom->end_const) {
                addr -= scom->start_const;
                /* high bit is valid marker */
                addr |= 0x80;
                scom->fifo_const |= addr << 8; /* 1 cycles delay */
            }
        }
    }
    return 0;
}

static int scom_reg_process(struct scom *scom, struct bus *bus)
{
    if (bus->sstate == 0 && bus->write) {
        scom->fifo_reg >>= 8;
        /* RCL */
        if (scom->fifo_reg & 0x10) {
            int addr = (scom->fifo_reg >> 5) & 7;
            memcpy(bus->io, scom->SCOM[addr], sizeof(bus->io));
            LOG (" RCL.%d=", addr + scom->start_reg); for (int i = 15; i >= 0; i--) LOG("%X", bus->io[i]);
            LOG (" ");
        }
    }
    else if (bus->sstate == 15 && !bus->write) {
        /* fifo not empty and STO */
        if ((scom->fifo_reg & 1) && !(scom->fifo_reg & 0x10)) {
            int addr = (scom->fifo_reg >> 5) & 7;
            memcpy(scom->SCOM[addr], bus->io, sizeof(bus->io));
            LOG (" STO.%d=", addr + scom->start_reg); for (int i = 15; i >= 0; i--) LOG("%X", bus->io[i]);
            LOG (" ");
        }
        /* match STO/RCL inst */
        if ((bus->irg & 0xFF0F) == 0x0A0F) {
            int addr = (bus->irg >> 5) & 7;
            if (addr >= scom->start_reg && addr < scom->end_reg) {
                addr -= scom->start_reg;
                scom->fifo_reg |= ((addr << 5) | (bus->irg & 0x1F)) << 16; /* 2 cycles delay */
            }
        }
    }
    return 0;
}

static int scom2_reg_process(struct scom *scom, struct bus *bus)
{
    if (bus->sstate == 0 && bus->write) {
        scom->fifo_reg >>= 8;
        /* RCL */
        if (scom->fifo_reg & 0x10) {
            int addr = (scom->fifo_reg >> 5) & 7;
            memcpy(bus->io, scom->SCOM[addr], sizeof(bus->io));
            LOG (" RCL.%d=", addr + scom->start_reg); for (int i = 15; i >= 0; i--) LOG("%X", bus->io[i]);
            LOG (" ");
        }
    }
    else if (bus->sstate == 15 && !bus->write) {
        /* fifo not empty and STO */
        if ((scom->fifo_reg & 1) && !(scom->fifo_reg & 0x10)) {
            int addr = (scom->fifo_reg >> 5) & 7;
            memcpy(scom->SCOM[addr], bus->io, sizeof(bus->io));
            LOG (" STO.%d=", addr + scom->start_reg); for (int i = 15; i >= 0; i--) LOG("%X", bus->io[i]);
            LOG (" ");
        }
        /* match STO F/RCL F inst */
        if ((bus->irg & 0xFFEF) == 0x0A0F) {
            /* address in hex 0-F */
            int addr = bus->io[0];
            if (addr >= scom->start_reg && addr < scom->end_reg) {
                addr -= scom->start_reg;
                scom->fifo_reg |= ((addr << 5) | (bus->irg & 0x1F)) << 16; /* 2 cycles delay */
            }
        }
    }
    return 0;
}

static int scom_process(void *priv, struct bus *bus)
{
    struct scom *scom = priv;
    int ret;
    ret = scom_const_process(scom, bus);
    if (ret)
        return ret;
    ret = scom_reg_process(scom, bus);
    return ret;
}

static int scom2_process(void *priv, struct bus *bus)
{
    struct scom *scom = priv;
    int ret;
    ret = scom_const_process(scom, bus);
    if (ret)
        return ret;
    ret = scom2_reg_process(scom, bus);
    return ret;
}

int scom_init(struct chip *chip, const char *name)
{
    int base;
    unsigned int size;
    struct scom *scom;
    scom = malloc(sizeof(*scom));
    if (!scom)
        return -1;
    scom->fifo_const = 0;
    scom->fifo_reg = 0;

    size = load_dumpK(scom->CONST, sizeof(scom->CONST)/16, name, &base);
	scom->end_const = base + size;
	scom->start_const = base;

    printf("const base %d size %d\n", base, size);
    for (unsigned int i = 0; i < size; i++) {
        printf("%02d: ", i+base);
        for (int j = 15; j >= 0; j--) {
                printf("%x", scom->CONST[i][j]);
        }
        printf("\n");
    }
    if (size > sizeof(scom->CONST)/16) {
        printf("const too big\n");
        free(scom);
        return -1;
    }

    chip->priv = scom;
    if (size > 16) {
        chip->process = scom2_process;
        scom->start_reg = base / 32 * 8;
        scom->end_reg = scom->start_reg + 8;
    }
    else {
        chip->process = scom_process;
        scom->start_reg = base / 16 * 2;
        scom->end_reg = scom->start_reg + 2;
    }



    return 0;

}
