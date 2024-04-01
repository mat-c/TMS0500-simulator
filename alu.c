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
#include <string.h>
#include <stdio.h>
#include "bus.h"
#include "emu.h"


// ====================================
// Log control
// ====================================
#define	LOG_FILE	log_file
// disassembly output macro
//#define	DIS(...)	fprintf (LOG_FILE, __VA_ARGS__)
// short log output macro
//#define	LOG(...)	fprintf (LOG_FILE, __VA_ARGS__)
// Hrast-like log output macro
#define	LOG_H(...)	fprintf (LOG_FILE, __VA_ARGS__)


// ====================================
// CPU state variables
// ====================================
static struct {
  // registers
  unsigned char A[16], B[16], C[16], D[16], E[16];
  // bit registers
  unsigned short KR, SR, fA, fB;
  // R5 ALU register
  unsigned char R5;

  // EXT signal (used for data exchange)
  unsigned short EXT;
  unsigned char Sout[16];
  unsigned char Sin[16];
  unsigned short opcode;
  unsigned char key;

  // various CPU flags
#define	FLG_IDLE	0x0001
#define	FLG_HOLD	0x0002
#define FLG_JUMP    0x0004
#define FLG_IO_VALID    0x0400
#define	FLG_COND	0x0800
#define	FLG_COND_LAST	0x1000
#define	FLG_BUSY	0x8000
  unsigned short flags;
  // cycle digit counter
  unsigned char digit;
  // CPU cycle counter (used to simulate real CPU frequency)
  unsigned cycle;

  int addr;
  int reset;
  int zero_suppr;
} cpu;

// mask definitions
typedef struct {
    unsigned char start, end, cpos, cval;
} mask_type;
static const mask_type mask_info[16] = {
    {-1, 0,  0,   0},
    {0, 15,  0,   0}, // ALL
    {0,  0,  0,   0}, // DPT
    {0,  0,  0,   1}, // DPT 1
    {0,  0,  0, 0xC}, // DPT C
    {3,  3,  3,   1}, // LLSD 1
    {1,  2,  1,   0}, // EXP
    {1,  2,  1,   1}, // EXP 1
    {-1, 0,  0,   0},
    {3, 15,  3,   0}, // MANT
    {-1, 0,  0,   0},
    {3, 15,  3,   5}, // MLSD 5
    {1, 15,  1,   0}, // MAEX
    {1, 15,  3,   1}, // MLSD 1
    {1, 15, 15,   1}, // MMSD 1
    {1, 15,  1,   1}  // MAEX 1
};

// ====================================
// ALU functions
// ====================================
// ALU block
// ------------------------------------
enum {ALU_ADD, ALU_SHL, ALU_SUB, ALU_SHR};
#define	ALU_SHIFT	ALU_SHL
// ------------------------------------
static void Alu (unsigned char *dst, unsigned char *srcX, unsigned char *srcY, const mask_type *mask, unsigned char flags) {
    unsigned char carry = 0;
    unsigned char shl = 0;
    int i;
    if (log_flags & LOG_DEBUG) {
        if (srcX) {LOG ("["); for (i = 15; i >= 0; i--) LOG ("%X", srcX[i]); LOG ("]");}
        if (srcY) {LOG ("["); for (i = 15; i >= 0; i--) LOG ("%X", srcY[i]); LOG ("]");}
    }
    for (i = 0; i <= 15; i++) {
        unsigned char sum = 0, shr = 0;
        if (i == mask->start)
            shl = carry = 0;
        if (srcY)
            sum = srcY[i];
        if (!(cpu.flags & FLG_IO_VALID))
            sum |= cpu.Sin[i];
        if (i == mask->cpos)
            sum |= mask->cval;
        shr = sum;
        sum += carry;
        if (flags >= ALU_SUB)
            sum = -sum;
        if (srcX) {
            sum += srcX[i];
            shr |= srcX[i];
        }
        cpu.Sout[i] = (sum & 0x0F);
        if (!i) {
            if ((carry = (sum >= 0x10)))
                sum &= 0x0F;
        } else {
            if ((carry = (sum >= 10))) {
                if (flags < ALU_SUB)
                    sum -= 10;
                else
                    sum += 10;
            }
        }
        // write result to destination
        if (i >= mask->start && i <= mask->end) {
            if (i == mask->start)
                cpu.R5 = sum;
            if (dst) {
                if (flags == ALU_SHL)
                    dst[i] = shl;
                else
                    if (flags == ALU_SHR) {
                        if (i > mask->start)
                            dst[i-1] = shr;
                        if (i == mask->end)
                            dst[i] = 0;
                    } else
                        dst[i] = sum;
                shl = sum;
            }
            if (i == mask->end && !(flags & ALU_SHIFT) && carry)
                cpu.flags &= ~FLG_COND;
        }
    }
}

// ====================================
// Exchange value
// ------------------------------------
static void Xch (unsigned char *src1, unsigned char *src2, const mask_type *mask) {
    int i;
    for (i = mask->start; i <= mask->end; i++) {
        unsigned char tmp;
        tmp = src1[i];
        src1[i] = src2[i];
        src2[i] = tmp;
    }
}

// ====================================
// main CPU function
// executes instructions
// ------------------------------------
int execute (unsigned short opcode) {
    // update instruction cycle counter
    if (cpu.flags & FLG_IDLE)
        cpu.cycle += 4;
    else
        cpu.cycle++;

    // process opcode
    if (opcode & 0x1000) {
        // ================================
        // jump
        // ================================
        cpu.flags |= FLG_JUMP;
        return 0;
    }
    if (cpu.flags & FLG_JUMP) {
        // COND is set again after last jump in series
        cpu.flags &= ~FLG_JUMP;
        cpu.flags |= FLG_COND;
    }
    switch (opcode & 0x0F00) {
        // ================================
        // flag operations
        // ================================
        case 0x0000:
            {
                unsigned bit = (opcode >> 4) & 0x000F;
                unsigned mask = 1 << bit;
                switch (opcode & 0x000F) {
                    case 0x0000:
                        // TEST FLAG A
                        if (cpu.fA & mask)
                            cpu.flags &= ~FLG_COND;
                        if (log_flags & LOG_DEBUG)
                            LOG ("FA=%04X ", cpu.fA);
                        if (log_flags & LOG_SHORT)
                            LOG ("COND=%u", (cpu.flags & FLG_COND) != 0);
                        break;
                    case 0x0001:
                        // SET FLAG A
                        cpu.fA |= mask;
                        if (log_flags & LOG_SHORT)
                            LOG ("FA=%04X", cpu.fA);
                        break;
                    case 0x0002:
                        // ZERO FLAG A
                        cpu.fA &= ~mask;
                        if (log_flags & LOG_SHORT)
                            LOG ("FA=%04X", cpu.fA);
                        break;
                    case 0x0003:
                        // INVERT FLAG A
                        cpu.fA ^= mask;
                        if (log_flags & LOG_SHORT)
                            LOG ("FA=%04X", cpu.fA);
                        break;
                    case 0x0004:
                        // EXCH. FLAG A B
                        if ((cpu.fA ^ cpu.fB) & mask) {
                            cpu.fA ^= mask;
                            cpu.fB ^= mask;
                        }
                        if (log_flags & LOG_SHORT)
                            LOG ("FA=%04X FB=%04X", cpu.fA, cpu.fB);
                        break;
                    case 0x0005:
                        // SET FLAG KR
                        cpu.KR |= mask;
                        if (log_flags & LOG_SHORT)
                            LOG ("KR=%04X", cpu.KR);
                        break;
                    case 0x0006:
                        // COPY FLAG B->A
                        if ((cpu.fA ^ cpu.fB) & mask)
                            cpu.fA ^= mask;
                        if (log_flags & LOG_SHORT)
                            LOG ("FA=%04X", cpu.fA);
                        break;
                    case 0x0007:
                        // REG 5->FLAG A S0 S3
                        cpu.fA = (cpu.fA & ~0x001E) | ((cpu.R5 & 0x000F) << 1);
                        if (log_flags & LOG_SHORT)
                            LOG ("FA=%04X", cpu.fA);
                        break;
                    case 0x0008:
                        // TEST FLAG B
                        if (cpu.fB & mask)
                            cpu.flags &= ~FLG_COND;
                        if (log_flags & LOG_DEBUG)
                            LOG ("FB=%04X ", cpu.fB);
                        if (log_flags & LOG_SHORT)
                            LOG ("COND=%u", (cpu.flags & FLG_COND) != 0);
                        break;
                    case 0x0009:
                        // SET FLAG B
                        cpu.fB |= mask;
                        if (log_flags & LOG_SHORT)
                            LOG ("FB=%04X", cpu.fB);
                        break;
                    case 0x000A:
                        // ZERO FLAG B
                        cpu.fB &= ~mask;
                        if (log_flags & LOG_SHORT)
                            LOG ("FB=%04X", cpu.fB);
                        break;
                    case 0x000B:
                        // INVERT FLAG B
                        cpu.fB ^= mask;
                        if (log_flags & LOG_SHORT)
                            LOG ("FB=%04X", cpu.fB);
                        break;
                    case 0x000C:
                        // COMPARE FLAG A B
                        if (!((cpu.fA ^ cpu.fB) & mask))
                            cpu.flags &= ~FLG_COND;
                        if (log_flags & LOG_DEBUG)
                            LOG ("FA=%04X FB=%04X ", cpu.fA, cpu.fB);
                        if (log_flags & LOG_SHORT)
                            LOG ("COND=%u", (cpu.flags & FLG_COND) != 0);
                        break;
                    case 0x000D:
                        // ZERO FLAG KR
                        cpu.KR &= ~mask;
                        if (log_flags & LOG_SHORT)
                            LOG ("KR=%04X", cpu.KR);
                        break;
                    case 0x000E:
                        // COPY FLAG A->B
                        if ((cpu.fA ^ cpu.fB) & mask)
                            cpu.fB ^= mask;
                        if (log_flags & LOG_SHORT)
                            LOG ("FB=%04X", cpu.fB);
                        break;
                    case 0x000F:
                        // REG 5->FLAG B S0 S3
                        cpu.fB = (cpu.fB & ~0x001E) | ((cpu.R5 & 0x000F) << 1);
                        if (log_flags & LOG_SHORT)
                            LOG ("FB=%04X", cpu.fB);
                        break;
                }
            }
            break;
            // ================================
            // keyboard operations
            // ================================
        case 0x0800:
            {
                //XXX the rom sometimes doesn't reset COND
                //before key operations...
                //it cause false detection, but there are removed
                //by debouncing. Bug or way to save one instruction
                unsigned char mask;
                // get pressed key(s) mask
                mask = (((opcode & 0x07) | ((opcode >> 1) & 0x78)) ^ 0x7F) & cpu.key;
                if (log_flags & LOG_DEBUG)
                    LOG ("(k%d=%02X)", cpu.digit, cpu.key & mask);
                // check if more than 1 key is pressed
                if (mask & (mask - 1))
                    mask = 0;
                if (!(opcode & 0x0008)) {
                    // scan all keyboard
                    // scan current row
                    if (cpu.key & mask) {
                        unsigned char bit = 0;
                        if (log_flags & LOG_DEBUG)
                            LOG ("(K%d=%02X)", cpu.digit, cpu.key & mask);
                        // get bit position
                        while (!(mask & 1)) {
                            bit++;
                            mask >>= 1;
                        }
                        // clear COND
                        cpu.flags &= ~FLG_COND;
                        // set result to KR
                        cpu.KR = /*(cpu.KR & ~0x07F0) |*/ (cpu.digit << 4) | ((bit << 8) & 0x0700);
                        if (log_flags & LOG_SHORT)
                            LOG ("KR=%04X COND=0", cpu.KR);
                    } else
                        if (cpu.digit) {
                            // wait for digit 0 counter - end of scan
                            cpu.flags |= FLG_HOLD;
                            return 11;
                        }
                } else {
                    // scan current row and update COND
                    if (cpu.key & mask)
                        cpu.flags &= ~FLG_COND;
                    if (log_flags & LOG_DEBUG)
                        LOG ("(K%d=%02X) ", cpu.digit, cpu.key & mask);
                    if (log_flags & LOG_SHORT)
                        LOG ("COND=%u", (cpu.flags & FLG_COND) != 0);
                }
            }
            break;
            // ================================
            // wait operations
            // ================================
        case 0x0A00:
            switch (opcode & 0x000F) {
                case 0x0000:
                    // wait for digit
                    if (cpu.digit != ((opcode >> 4) & 0x000F)) {
                        cpu.flags |= FLG_HOLD;
                        return 12;
                    }
                    if (log_flags & LOG_DEBUG)
                        LOG ("(D=%u)", cpu.digit);
                    break;
                case 0x0001:
                    // Zero Idle
                    cpu.flags &= ~FLG_IDLE;
                    if (log_flags & LOG_SHORT)
                        LOG ("IDLE=0");
                    break;
                case 0x0002:
                    // CLFA
                    cpu.fA = 0;
                    if (log_flags & LOG_SHORT)
                        LOG ("FA=%04X", cpu.fA);
                    break;
                case 0x0003:
                    // Wait Busy
#warning "Unknown behaviour..."
                    break;
                case 0x0004:
                    // INCKR
                    cpu.KR += 0x0010;
                    if (!(cpu.KR & 0xFFF0))
                        cpu.KR ^= 0x0001;
                    if (log_flags & LOG_SHORT)
                        LOG ("KR=%04X", cpu.KR);
                    break;
                case 0x0005:
                    // TKR
                    if (cpu.KR & (1 << ((opcode >> 4) & 0x000F)))
                        cpu.flags &= ~FLG_COND;
                    if (log_flags & LOG_DEBUG)
                        LOG ("KR=%04X ", cpu.KR);
                    if (log_flags & LOG_SHORT)
                        LOG ("COND=%u", (cpu.flags & FLG_COND) != 0);
                    break;
                case 0x0006:
                    // FLGR5 + peripherals
                    switch (opcode & 0x00F0) {
                      case 0x0010:
                        cpu.R5 = (cpu.fB >> 1) & 0x000F;
                        if (log_flags & LOG_DEBUG)
                            LOG ("FB=%04X ", cpu.fB);
                        if (log_flags & LOG_SHORT)
                            LOG ("R5=%01X", cpu.R5);
                        break;
                      case 0x0000:
                        cpu.R5 = (cpu.fA >> 1) & 0x000F;
                        if (log_flags & LOG_DEBUG)
                            LOG ("FA=%04X ", cpu.fA);
                        if (log_flags & LOG_SHORT)
                            LOG ("R5=%01X", cpu.R5);
                        break;
                    }
                    break;
                case 0x0007:
                    // Number
                    cpu.R5 = (opcode >> 4) & 0x000F;
                    if (log_flags & LOG_SHORT)
                        LOG ("R5=%01X", cpu.R5);
                    break;
                case 0x0008:
                    // KRR5/R5KR + peripherals
                    switch (opcode & 0x00F0) {
                        case 0x0000:
                            // KRR5
                            cpu.R5 = (cpu.KR >> 4) & 0x000F;
                            if (log_flags & LOG_SHORT)
                                LOG ("R5=%01X", cpu.R5);
                            break;
                        case 0x0010:
                            // R5KR
                            cpu.KR = (cpu.KR & ~0x00F0) | (cpu.R5 << 4);
                            if (log_flags & LOG_SHORT)
                                LOG ("KR=%04X", cpu.KR);
                            break;
                    }
                    break;
                case 0x0009:
                    // Set Idle
                    cpu.flags |= FLG_IDLE;
                    if (log_flags & LOG_SHORT)
                        LOG ("IDLE=1");
                    break;
                case 0x000A:
                    // CLFB
                    cpu.fB = 0;
                    if (log_flags & LOG_SHORT)
                        LOG ("FB=%04X", cpu.fB);
                    break;
                case 0x000B:
                    // Test Busy
                    if ((cpu.key & (1 << KR_BIT)) || (cpu.flags & FLG_BUSY))
                        cpu.flags &= ~(FLG_COND | FLG_BUSY);
                    if (log_flags & LOG_SHORT)
                        LOG ("(K%d=%02X) COND=%u", cpu.digit, cpu.key & (1 << KR_BIT), (cpu.flags & FLG_COND) != 0);
                    break;
                case 0x000C:
                    // EXTKR
                    // XXX KR[0] set ????
                    //cpu.KR = (cpu.KR & 0x000F) | ((cpu.EXT << 1) & 0xFFF0);
                    cpu.KR = ((cpu.EXT << 1) & 0xFFF0) | (cpu.EXT >> 15);
                    if (log_flags & LOG_SHORT)
                        LOG ("KR=%04X", cpu.KR);
                    break;
                case 0x000D:
                    // XKRSR
                    {
                        unsigned short tmp;
                        tmp = cpu.KR;
                        cpu.KR = cpu.SR;
                        cpu.SR = tmp;
                    }
                    if (log_flags & LOG_SHORT)
                        LOG ("KR=%04X SR=%04X", cpu.KR, cpu.SR);
                    break;
                case 0x000E:
                    // NO-OP + peripherals
                    switch (opcode & 0x00F0) {
                        break;
                    }
                    break;
                case 0x000F:
                    // Register
                    switch (opcode & 0x00F0) {
                        break;
                    }
                    break;
            }
            break;
            // ================================
            // ALU operations
            // ================================
        default: 
            {
                const mask_type *mask = &mask_info[(opcode >> 8) & 0x0F];
                static const struct {
                    unsigned char *srcX, *srcY;
                    unsigned char flags;
                } *alu_inp, ALU_OP[32] = {
                    {cpu.A, 0, ALU_ADD},
                    {cpu.A, 0, ALU_SUB},
                    {0, cpu.B, ALU_ADD},
                    {0, cpu.B, ALU_SUB},
                    {cpu.C, 0, ALU_ADD},
                    {cpu.C, 0, ALU_SUB},
                    {0, cpu.D, ALU_ADD},
                    {0, cpu.D, ALU_SUB},
                    {cpu.A, 0, ALU_SHL},
                    {cpu.A, 0, ALU_SHR},
                    {0, cpu.B, ALU_SHL},
                    {0, cpu.B, ALU_SHR},
                    {cpu.C, 0, ALU_SHL},
                    {cpu.C, 0, ALU_SHR},
                    {0, cpu.D, ALU_SHL},
                    {0, cpu.D, ALU_SHR},
                    {cpu.A, cpu.B, ALU_ADD},
                    {cpu.A, cpu.B, ALU_SUB},
                    {cpu.C, cpu.B, ALU_ADD},
                    {cpu.C, cpu.B, ALU_SUB},
                    {cpu.C, cpu.D, ALU_ADD},
                    {cpu.C, cpu.D, ALU_SUB},
                    {cpu.A, cpu.D, ALU_ADD},
                    {cpu.A, cpu.D, ALU_SUB},
                    // following needs special approach...
                    // -> variable pointers, RAM/SCOM access, R5 access
                    {cpu.A, 0 /*CONSTANT[((cpu.KR >> 5) & 0x78) | ((cpu.KR >> 4) & 0x07)]*/, ALU_ADD}, // IO read
                    {cpu.A, 0 /*CONSTANT[((cpu.KR >> 5) & 0x78) | ((cpu.KR >> 4) & 0x07)]*/, ALU_SUB}, // IO read
                    {0, 0, ALU_ADD}, // IO read: 0 -> SCOM[cpu.REG_ADDR] | RAM[cpu.RAM_ADDR]
                    {0, 0, ALU_SUB},
                    {cpu.C, 0 /*CONSTANT[((cpu.KR >> 5) & 0x78) | ((cpu.KR >> 4) & 0x07)]*/, ALU_ADD}, // IO read
                    {cpu.C, 0 /*CONSTANT[((cpu.KR >> 5) & 0x78) | ((cpu.KR >> 4) & 0x07)]*/, ALU_SUB}, // IO read
                    {0, 0 /*cpu.R5*/, ALU_ADD}, // IO read ??
                    {0, 0 /*cpu.R5*/, ALU_SUB} // IO read ??
                };
                static const struct {
                    unsigned char *dst;
                    char log[4];
                } *alu_out, ALU_DST[8] = {
                    {cpu.A, "A"},
                    {0, "IO"},
                    {0, ""}, // Xch A,B
                    {cpu.B, "B"},
                    {cpu.C, "C"},
                    {0, ""}, // Xch C,D
                    {cpu.D, "D"},
                    {0, ""}  // Xch A,E
                };
                alu_out = &ALU_DST[opcode & 0x07];
                if ((opcode & 0x07) == 0x01)
                    cpu.flags |= FLG_IO_VALID;
                alu_inp = &ALU_OP[(opcode >> 3) & 0x1F];
                switch (opcode & 0x00F8) {
                    default:
                        // generic ALU operation
                        Alu (alu_out->dst, alu_inp->srcX, alu_inp->srcY, mask, alu_inp->flags);
                        break;
                        // process special cases
                    case 0x00F0: // R5->Adder
                    case 0x00F8: // not used in TI-58, probably different behavior...
                        if (alu_out->dst) {
                            int i;
                            for (i = mask->start+1; i <= mask->end; i++)
                                alu_out->dst[i] = 0;
                            alu_out->dst[mask->cpos] = mask->cval;
                            alu_out->dst[mask->start] = cpu.R5;
                            // make BCD correction
                            if (!(opcode & 0x0008))
                                Alu (alu_out->dst, 0, alu_out->dst, mask, ALU_ADD);
                            else
                                Alu (alu_out->dst, 0, alu_out->dst, mask, ALU_SUB); // not sure with this...
                        }
                        break;
                }
                // EXCHANGE instructions
                switch (opcode & 0x0007) {
                    case 0x0002: // A<->B
                        Xch (cpu.A, cpu.B, mask);
                        if (log_flags & LOG_SHORT) {
                            int i;
                            LOG ("A="); for (i = 15; i >= 0; i--) LOG ("%X", cpu.A[i]);
                            LOG (" B="); for (i = 15; i >= 0; i--) LOG ("%X", cpu.B[i]);
                        }
                        break;
                    case 0x0005: // C<->D
                        Xch (cpu.C, cpu.D, mask);
                        if (log_flags & LOG_SHORT) {
                            int i;
                            LOG ("C="); for (i = 15; i >= 0; i--) LOG ("%X", cpu.C[i]);
                            LOG (" D="); for (i = 15; i >= 0; i--) LOG ("%X", cpu.D[i]);
                        }
                        break;
                    case 0x0007: // A<->E
                        Xch (cpu.A, cpu.E, mask);
                        if (log_flags & LOG_SHORT) {
                            int i;
                            LOG ("A="); for (i = 15; i >= 0; i--) LOG ("%X", cpu.A[i]);
                            LOG (" E="); for (i = 15; i >= 0; i--) LOG ("%X", cpu.E[i]);
                        }
                        break;
                }
                if (*alu_out->log && (log_flags & LOG_SHORT)) {
                    int i;
                    unsigned char *ptr = alu_out->dst;
                    if (!ptr)
                        ptr = cpu.Sout;
                    LOG ("%s=", alu_out->log); for (i = 15; i >= 0; i--) LOG ("%X", ptr[i]);
                }
            }
    }

    return 0;
}


static void debug(int addr, int opcode)
{
    if (log_flags) {
        DIS("\n");
        if (log_flags & LOG_SHORT)
#if 1
            DIS ("%04X:%c%c%c.D%02d\t%04X\t", addr, (cpu.flags & FLG_COND) ? 'C' : '-',
                    (cpu.flags & FLG_IDLE) ? 'I' : '-',
                    (cpu.flags & FLG_HOLD) ? 'H' : '-',
                    cpu.digit,
                    opcode);
#else
            DIS ("%04X:%c%c\t%04X\t", addr, (cpu.flags & FLG_COND) ? 'C' : '-',
                    (cpu.flags & FLG_IDLE) ? 'I' : '-',
                    opcode);
#endif
        else
            if (log_flags & LOG_HRAST)
                DIS ("%04X %04X ", addr, opcode);
        disasm (addr, opcode);
        DIS ("\n");
        if (log_flags & LOG_HRAST) {
            int i;
            LOG_H ("A="); for (i = 15; i >= 0; i--) LOG_H ("%X", cpu.A[i]);
            LOG_H (" B="); for (i = 15; i >= 0; i--) LOG_H ("%X", cpu.B[i]);
            LOG_H (" C="); for (i = 15; i >= 0; i--) LOG_H ("%X", cpu.C[i]);
            LOG_H (" D="); for (i = 15; i >= 0; i--) LOG_H ("%X", cpu.D[i]);
            LOG_H (" E="); for (i = 15; i >= 0; i--) LOG_H ("%X", cpu.E[i]);
            LOG_H ("\nFA=%04X [", cpu.fA); for (i = 15; i >= 0; i--) LOG_H ("%d", (cpu.fA >> i) & 1);
            LOG_H ("] KR=%04X [", cpu.KR); for (i = 15; i >= 0; i--) LOG_H ("%d", (cpu.KR >> i) & 1);
            LOG_H ("] EXT=%02X COND=%d IDLE=%d", (cpu.EXT >> 4) & 0xFF, (cpu.flags & FLG_COND) != 0, (cpu.flags & FLG_IDLE) != 0);
            LOG_H (" IOi="); for (i = 15; i >= 0; i--) LOG_H ("%X", cpu.Sin[i]);
            LOG_H (" IO="); for (i = 15; i >= 0; i--) LOG_H ("%X", cpu.Sout[i]);
            LOG_H ("\nFB=%04X [", cpu.fB); for (i = 15; i >= 0; i--) LOG_H ("%d", (cpu.fB >> i) & 1);
            LOG_H ("] SR=%04X R5=%X", cpu.SR, cpu.R5);
            LOG_H ("\n");
        } else {
            LOG ("\t");
        }
    }
}


static inline int run_early(int opcode)
{
    switch (opcode & 0x1F00) {
        case 0x0000: /* flags */
            return 1;
        case 0x0800: /* keyboard */
            return 1; /* HOLD/COND */
        case 0x0A00: /* wait */
            switch (opcode & 0xF) {
                case 0x0: /* wait digit */
                    return 1; /* HOLD */
                case 0x3: /* wait busy */
                case 0xB: /* test busy. log cpu.digit */
                    return 1; /* COND */
                case 0xC: /* mov KR, EXT */
                    return 0; /* need EXT, to check XXX */
                default:
                    return 1;
            }
        default: /* alu */
            if ((opcode & 7) == 1) /* IO write. Can set R5/COND */
                return 1;
    }
    return 0;
}

static void alu_gen_digit(struct bus *bus)
{
    if (cpu.flags & FLG_IDLE) {
        int i = cpu.digit;
#ifndef DISP_DBG
        if (i == 15)
            cpu.zero_suppr = 1;
        if (i == 3 ||
                (cpu.R5 == i && i != 15) ||
                cpu.B[i] >= 8)
            cpu.zero_suppr = 0;
        if (i == 2)
            cpu.zero_suppr = 1;
        if (cpu.B[i] == 7 || cpu.B[i] == 3 || (cpu.B[i] <= 4 && cpu.zero_suppr && !cpu.A[i]))
            bus->display_digit = ' ';
        else if (cpu.B[i] == 6 || (cpu.B[i] == 5 && !cpu.A[i]))
            bus->display_digit = '-';
        else if (cpu.B[i] == 5)
            bus->display_digit = 'o';
        else if (cpu.B[i] == 4)
            bus->display_digit = '\'';
        //XXX B[3] or B[i] ?
        else if (cpu.B[3] == 2)
            bus->display_digit = '"';
        else {
            bus->display_digit = '0' + cpu.A[i];
            if (cpu.A[i])
                cpu.zero_suppr = 0;
        }
        bus->display_dpt = 0;
        bus->display_segH = 0;
        if (cpu.R5 == i)
            bus->display_dpt = 1;
        //XXX
        //D15 is wrong, but not really used
        if (cpu.fA & (1<<(i+1)))
            bus->display_segH = 1;
#endif
    }
    else {
#if 0
        /* output at Srate */
        if (cpu.fA)
            bus->display_segH = 1;
#else
        /* limit display refresh */
        bus->display_segH = 1;
#endif
    }
}

static int alu_process(void *priv, struct bus *bus)
{
    cpu.digit = bus->dstate;
    cpu.EXT = bus->ext;
    cpu.key = bus->key_line;

    if (cpu.reset) {
        if (bus->sstate == 0 && bus->write && cpu.reset-- > 1)
            bus->ext = 1;
        else if (bus->sstate == 15 && !bus->write && cpu.reset == 1) {
            cpu.opcode = bus->irg;
            cpu.addr = bus->addr;
            if (bus->addr == -1)
                return 1;
        }
        return 0;
    }

    if (bus->sstate == 0 && bus->write) {
        debug(cpu.addr, cpu.opcode);
        cpu.flags &= ~FLG_HOLD;
        memset(cpu.Sin, 0, sizeof(cpu.Sin));
        memset(cpu.Sout, 0, sizeof(cpu.Sin));
        if (cpu.flags & FLG_COND)
            cpu.flags |= FLG_COND_LAST;

        //XXX D change at S15W. But last alu input S15R
        //TODO update digit, dpt, segH here...
        alu_gen_digit(bus);

        /* we need to run here :
         * instruction that set HOLD (need S2W)
         * alu operation that write io (need S0W)
         * instruction that change KR (need SxW)
         * instruction that set PREG (need S0W)
         *
         * we don't want to run here :
         * instruction that read io
         * instruction that read ext
         */
        if (run_early(cpu.opcode)) {
            execute(cpu.opcode);
            if (cpu.flags & FLG_IO_VALID) {
                memcpy(bus->io, cpu.Sout,  sizeof(bus->io));
                cpu.flags &= ~FLG_IO_VALID;
            }
        }
        /* Output KR, unless MOV     KR,EXT[4..15]
         * We need to send KR from current cycle :
         * - KR[1]/PREG need to be set during SET KR[1] cycle
         * - Printer code need it
         *   -- instruction changing KR on alu/PRINTER instructions on irg
         *   according to ti59 service manual. PRINTER instructions use ext data
         *   from irg PRINTER instructions Dcycle.
         *   -- running test, show that code modify KR one instruction before printer one
         *   and we need the updated KR for correct print
         */
        if (cpu.opcode != 0x0A0C)
            bus->ext = ((cpu.KR >> 1) | (cpu.KR << 15)) & 0xFFF9;
    }
    else if (bus->sstate == 1 && bus->write) {
        /* Set COND flags from previous cycle.
         * XXX in realy hardware, current COND is delayed ?
         * Other peripheral can also set it
         * read by xrom
         */
        if (cpu.flags & FLG_COND_LAST) {
            bus->ext |= EXT_COND;
            cpu.flags &= ~FLG_COND_LAST;
        }
    }
    else if (bus->sstate == 2 && bus->write) {
        if (cpu.flags & FLG_HOLD) {
            bus->ext |= EXT_HOLD;
        }
    }
    else if (bus->sstate == 2 && !bus->write) {
        if ((bus->ext & EXT_HOLD) == 0) {
            // clear PREG bit if bus is not hold
            cpu.KR &= ~0x2;
        }
    }
    else if (bus->sstate == 14 && bus->write) {
        /* clear segment before D line switch */
        bus->display_dpt = 0;
        bus->display_segH = 0;
        bus->display_digit = ' ';
    }
    /* some alu operation depends on IO and other write to IO
     * dst IO : xxx001
     * */
    else if (bus->sstate == 15 && !bus->write) {
        if (!run_early(cpu.opcode)) {
            memcpy(cpu.Sin,  bus->io, sizeof(bus->io));
            execute(cpu.opcode);
        }
        /* save next opcode */
        cpu.opcode = bus->irg;
        cpu.addr = bus->addr;
        /* KR[1] and KR[2] not used. Reuse them to save COND, HOLD ?
         * XXX check if some KR instruction can clear it
         * */

        if (bus->addr == -1)
            return 1;
    }

    bus->idle = cpu.flags & FLG_IDLE;


    return 0;
}

int alu_init(struct chip *chip)
{
    /* force preg 0 */
    //cpu.KR = 2;
    /* set cond for easy compare with other logs */
    cpu.flags |= FLG_COND;
#if 1
    memset(cpu.A, 0xE, sizeof(cpu.A));
    memset(cpu.B, 0xE, sizeof(cpu.B));
    /* ti58 is doing ADD     IO.ALL,C,#0
     * in init sequence. This clear COND
     * with EE..EE init
     */
    memset(cpu.C, 0x5, sizeof(cpu.B));
    memset(cpu.D, 0xE, sizeof(cpu.B));
    memset(cpu.E, 0xE, sizeof(cpu.B));
    cpu.SR = 0XDEAD;
    cpu.fA = 0XDEAD;
    cpu.fB = 0XDEAD;
    /* SR51 don't clear KR */
    //cpu.KR = 0xDEAD;
    cpu.R5 = 0xE;
#else
    memset(cpu.A, 0x0, sizeof(cpu.A));
    memset(cpu.B, 0x0, sizeof(cpu.B));
    memset(cpu.C, 0x0, sizeof(cpu.B));
    memset(cpu.D, 0x0, sizeof(cpu.B));
    memset(cpu.E, 0x0, sizeof(cpu.B));
    cpu.SR = 0;
    cpu.fA = 0;
    cpu.fB = 0;
    //cpu.KR |= 0xDE00;
    cpu.R5 = 0;
#endif
    cpu.reset = 5;

    chip->process = alu_process;
    printf("alu init\n");
    return 0;
}
