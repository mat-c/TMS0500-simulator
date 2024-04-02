/*
 * Copyright (C) 2014 Hynek Sladky; sladky@mujmail.cz
 * Copyright (C) 2024 by Matthieu CASTET <castet.matthieu@free.fr>
 *
 * A part of this coded is base on Hynek emulator 
 * see http://hsl.wz.cz/ti_59.htm / https://www.hrastprogrammer.com/emulators.htm
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

#include "emu.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>

// ------------------------------------
int load_dump (unsigned short *buf, int buf_len, const char *name, int *base) {
    int rom_size = 0;
    int base_addr = -1;
    FILE *f;
#define	LINELEN	1024
    char line[LINELEN];
    if (!base)
        return 0;
    *base = -1;
    if ((f = fopen (name, "rt")) == NULL)
        return 0;
    while (!feof (f)) {
        unsigned data, idx = 0;
        int addr;
        if (!fgets (line, LINELEN, f))
            break;
        if (!isxdigit(line[0]) || !isxdigit(line[1]) ||
            !isxdigit(line[2]) || !isxdigit(line[3]) ||
            line[4] != ':')
            continue;
        if (!sscanf (line, "%X: ", &addr))
            continue;
        if (base_addr == -1)
            base_addr = addr;
        if (addr >= base_addr)
            addr -= base_addr;
        else {
            fprintf (stderr, "load %s: address 0x%X out of range\n", name, addr);
            continue;
        }
        while (line[idx] > ' ') idx++;
        while (line[idx] && line[idx] <= ' ') idx++;
        while (sscanf (line+idx, "%X", &data) > 0) {
            if (addr < buf_len) {
                buf[addr++] = data;
                if (rom_size < addr)
                    rom_size = addr;
            }
            else
                fprintf (stderr, "load %s: address 0x%X out of range\n", name, addr + base_addr);
            while (line[idx] > ' ') idx++;
            while (line[idx] && line[idx] <= ' ') idx++;
        }
    }
    fclose (f);
    *base = base_addr;
    return rom_size;
}

int load_dumpK (unsigned char buf[][16], int buf_len, const char *name, int *base) {
    int rom_size = 0;
    int base_addr = -1;
    FILE *f;
#define	LINELEN	1024
    char line[LINELEN];
    if (!base)
        return 0;
    *base = -1;
    if ((f = fopen (name, "rt")) == NULL)
        return 0;
    while (!feof (f)) {
        int addr, idx = 0;
        uint64_t data;
        if (!fgets (line, LINELEN, f))
            break;

        const char *end = "ADDR: CONSTANT ROM (KEY CODE)";
        if (strncmp(end, line, strlen(end)) == 0)
            break;

        if (!isdigit(line[0]) || !isdigit(line[1]) ||
            !isdigit(line[2]) || line[3] != ':')
            continue;

        if (!sscanf (line, "%d: ", &addr))
            continue;
        if (base_addr == -1)
            base_addr = addr;
        if (addr >= base_addr)
            addr -= base_addr;
        else {
            fprintf (stderr, "load %s: address 0x%X out of range\n", name, addr);
            continue;
        }

        while (line[idx] > ' ') idx++;
        while (line[idx] && line[idx] <= ' ') idx++;
        while (sscanf (line+idx, "%lX", &data) > 0) {
            if (addr < buf_len) {
                for (int j = 0; j < 16; j++) {
                    buf[addr][j] = data & 0xf;
                    data >>= 4;
                }
                addr++;
                if (rom_size < addr)
                    rom_size = addr;
            }
            else
                fprintf (stderr, "load %s: address 0x%X too big\n", name, addr);
            while (line[idx] > ' ') idx++;
            while (line[idx] && line[idx] <= ' ') idx++;
        }
    }
    fclose (f);
    *base = base_addr;
    return rom_size;
}

int load_dump8 (unsigned char *buf, int buf_len, const char *name) {
    int rom_size = 0;
    FILE *f;
#define	LINELEN	1024
    char line[LINELEN];
    if ((f = fopen (name, "rt")) == NULL)
        return 1;
    while (!feof (f)) {
        int addr;
        unsigned data, idx = 0;
        if (!fgets (line, LINELEN, f))
            break;
        if (!isdigit(line[0]) || !isdigit(line[1]) ||
                !isdigit(line[2]) || !isdigit(line[3]) ||
                line[4] != ':')
            continue;
        if (!sscanf (line, "%d: ", &addr))
            continue;
        while (line[idx] > ' ') idx++;
        while (line[idx] && line[idx] <= ' ') idx++;
        while (sscanf (line+idx, "%X", &data) > 0) {
            if (addr < buf_len) {
                buf[addr++] = data;
                if (rom_size < addr)
                    rom_size = addr;
            }
            else
                fprintf (stderr, "load %s: address 0x%X too big\n", name, addr);
            while (line[idx] > ' ') idx++;
            while (line[idx] && line[idx] <= ' ') idx++;
        }
    }
    fclose (f);
    return rom_size;
}

const char libtoken[100][8] = {
// 2nd => '
// printer syntax: see all_codes.lst
// key codes: see TI-5x user guide page V-50 and USpat 4153937 Table III
  "0",
  "1",
  "2",
  "3",
  "4",
  "5",
  "6",
  "7",
  "8",
  "9",
  "E'",
  "A",
  "B",
  "C",
  "D",
  "E",
  "A'",
  "B'",
  "C'",
  "D'",
  "CLR'", // 20
  "2nd",
  "INV",
  "LNx",
  "CE",
  "CLR",
  "2nd'",
  "INV'",
  "LOG",
  "CP",
  "TAN",
  "LRN",
  "x<->t",
  "x^2",
  "sqrt",
  "1/x",
  "PGM",
  "P/R", //"P->R",
  "SIN",
  "COS",
  "IND",
  "SST",
  "STO",
  "RCL",
  "SUM",
  "Y^x",
  "INS",
  "CMs",
  "EXC",
  "PRD",
  "|x|",
  "BST",
  "EE",
  "(",
  ")",
  "/",
  "DEL",
  "ENG",
  "FIX",
  "INT",
  "DEG",
  "GTO",
  "PG*", //"Pgm-Ind",
  "EX*", //"Exc-Ind",
  "PD*", //"Prd-Ind",
  "*",
  "PAU", //"Pause",
  "EQ", //"x=t",
  "NOP",
  "OP",
  "RAD",
  "SBR",
  "ST*", //STO-Ind",
  "RC*", //"RCL-Ind",
  "SM*", //"SUM-Ind",
  "-",
  "LBL",
  "GE", //"x>=t",
  "Sum+",
  "avg_x",
  "GRD",
  "RST",
  "HIR",
  "GO*", //"GTO-Ind",
  "OP*", //"Op-Ind",
  "+",
  "STF", //"StFlg",
  "IFF", //"IfFlg",
  "DMS",
  "pi",
  "LST",
  "R/S",
  "RTN",
  ".",
  "+/-",
  "=",
  "WRT",
  "DSZ",
  "ADV",
  "PRT"
};

