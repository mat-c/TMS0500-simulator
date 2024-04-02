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

// I11..8
static const char *mask[16] = {
  "<flag> ????",
  "ALL",
  "DPT",
  "DPT", // 1",
  "DPT", // C",
  "LLSD", // 1",
  "EXP",
  "EXP", // 1",
  "<keyboard> ????",
  "MANT",
  "<wait> ????",
  "MLSD", // 5",
  "MAEX",
  "MLSD", // 1",
  "MMSD", // 1",
  "MAEX", // 1"
};
static const char *N[16] = {
  "?",
  "0",
  "0",
  "1",
  "xC",
  "1",
  "0",
  "1",
  "?",
  "0",
  "?",
  "5",
  "0",
  "1",
  "1",
  "1"
};
// I7..I3
static const char *alu[32] = {
  //op  out,X,Y
  "ADD\t%s.%s,A,#%s", // N
  "SUB\t%s.%s,A,#%s", // N
  "MOV\t%s.%s,B", // N
  "NEG\t%s.%s,B", // N
  "ADD\t%s.%s,C,#%s", // N
  "SUB\t%s.%s,C,#%s", // N
  "MOV\t%s.%s,D", // N
  "NEG\t%s.%s,D", // N
  "SHL\t%s.%s,A",
  "SHR\t%s.%s,A",
  "SHL\t%s.%s,B",
  "SHR\t%s.%s,B",
  "SHL\t%s.%s,C",
  "SHR\t%s.%s,C",
  "SHL\t%s.%s,D",
  "SHR\t%s.%s,D",
  "ADD\t%s.%s,A,B",
  "SUB\t%s.%s,A,B",
  "ADD\t%s.%s,C,B",
  "SUB\t%s.%s,C,B",
  "ADD\t%s.%s,C,D",
  "SUB\t%s.%s,C,D",
  "ADD\t%s.%s,A,D",
  "SUB\t%s.%s,A,D",
  "ADD\t%s.%s,A,const",
  "SUB\t%s.%s,A,const",
  "MOV\t%s.%s,#%s\t; io read",
  "MOV\t%s.%s,#-%s",
  "ADD\t%s.%s,C,const",
  "SUB\t%s.%s,C,const",
  "MOV\t%s.%s,R5",
  "MOV\t%s.%s,-R5\t; ????"
};
// I2..0
static const char *sum[8] = {
  "A",
  "IO",
  "AxB",
  "B",
  "C",
  "CxD",
  "D",
  "AxE"
};
// 0 0000 .... ____
// I7..4 = bit number
static const char *flag[16] = {
  "TST\tfA[%u]",
  "SET\tfA[%u]",
  "CLR\tfA[%u]",
  "INV\tfA[%u]",
  "XCH\tfA[%u],fB[%u]",
  "SET\tKR[%u]",
  "MOV\tfA[%u],fB[%u]", // B->A
  "MOV\tfA[1..4],R5", //S0 ... S3
  "TST\tfB[%u]",
  "SET\tfB[%u]",
  "CLR\tfB[%u]",
  "INV\tfB[%u]",
  "CMP\tfA[%u],fB[%u]",
  "CLR\tKR[%u]",
  "MOV\tfB[%u],fA[%u]", // A->B
  "MOV\tfB[1..4],R5" //S0 ... S3
};
// 0 1010 xxxx ____
static const char *wait[16] = {
  "WAIT\tD%u",
  "CLR\tIDL",
  "CLR\tfA",
  "WAIT\tBUSY\t; N.C.",
  "INC\tKR",
  "TST\tKR[%u]",
  "FLGR5",
  "MOV\tR5,#%u",
  "xch\tKR[4..7],R5", // KR<->R5 -> see below
  "SET\tIDL",
  "CLR\tfB",
  "TST\tBUSY\t; N.C.",
  "MOV\tKR,EXT[4..15]", // 0..3 are zeroed ??
  "XCH\tKR,SR",
  "lib", // LIB instructions -> see below
  "MOV", //STO/RCL -> see below
};
// 0 1010 ____ 1000
static const char *prn[16] = {
  "MOV\tR5,KR[4..7]",
  "MOV\tKR[4..7],R5",
  "IN\tCRD",
  "OUT\tCRD",
  "CRD_OFF",
  "CRD_READ",
  "OUT\tPRT",
  "OUT\tPRT_FUNC",
  "PRT_CLEAR",
  "PRT_STEP",
  "PRT_PRINT",
  "PRT_FEED",
  "CRD_WRITE",
  "??.xD\t\t; no-op????",
  "??.xE\t\t; set COND????",
  "RAM_OP"
};
static const char *no_op[16] = {
  "IN\tLIB",
  "OUT\tLIB_PC",
  "IN\tLIB_PC",
  "IN\tLIB_HIGH",
  "NOP.x4\t;????",
  "NOP.x5\t;????",
  "NOP.x6\t;????",
  "NOP.x7\t;????",
  "NOP.x8\t;????",
  "NOP.x9\t;????",
  "NOP.xA\t;????",
  "NOP.xB\t;????",
  "NOP.xC\t;????",
  "NOP.xD\t;????",
  "NOP.xE\t;????",
  "NOP.xF\t;????",
};
// IRG bity
#define        IRG_BRANCH        0x1000

/* check flags */
#define FLG_IOR_EXPECTED 0x1
#define FLG_IOW_EXPECTED 0x2
#define FLG_IOWx_EXPECTED 0x6
#define FLG_IOx_EXPECTED 0x8
#define FLAG_IO_READ 0x10
#define FLAG_IO_WRITE 0x20
#define FLAG_IO_WRITE_PREV 0x40

static unsigned check_flags;

void disasm (unsigned addr, unsigned opcode) {
    /* instruction are in patent US 3900722 */
    unsigned new_check_flags = 0;


    /* branch instruction. Cond is set on failling of branch bit */
    if (opcode & IRG_BRANCH) {
        unsigned dest = addr;
        // branch
        if (opcode & 0x0001)
            dest -= (opcode >> 1) & 0x03FF;
        else
            dest += (opcode >> 1) & 0x03FF;
        //DIS ("BRA%c\t%c%d\t;%04X", (opcode&0x0800) ? '1' : '0', (opcode & 0x0001) ? '-' : '+', (opcode >> 1) & 0x03FF, dest);
        //DIS ("BRA.%s\t%04X", (opcode&0x0800) ? "C" : "nC", dest);
        DIS ("BRA%c\t%04X", (opcode&0x0800) ? '1' : '0', dest);
        if ((opcode & 0x17FF) == 0x1002)
            DIS ("\t; clear COND");
    }
    else
    switch ((opcode>>8)&0x0F) {
        // special instructions
        case 0x00:
            // flag operations
            // opcode&0x0F : encoded state time are +2 from actual states
            //DIS ("flag\tst=%d\t%s", (opcode>>4)&0x0F, flag[opcode&0x0F]);
            DIS (flag[opcode&0x0F], (opcode >> 4) & 0x0F, (opcode >> 4) & 0x0F);
            if (opcode == 0x0015)
                DIS ("\t; PREG");
            // mov r5, flag have no argument
            if ((opcode & 7) == 7 && (opcode & 0xF0))
                DIS("\t;????");
            break;
        case 0x08:
          {
            int i;
            int keymask = ~(((opcode  >> 1) & 0x78) | (opcode & 0x7));
            // keyboard
            // XXX in patent KT is inversed (error ?)
            DIS ("KEY\t%02X", opcode&0xFF);
            DIS ("\t;scan=%d", !(opcode & 0x8));
            for (i = 0; i < 7; i++) {
                if (keymask & (1<<i))
                    DIS(" K%c", 'N'+i);
            }
            break;
          }
        case 0x0A:
        {
            int wait_type = opcode&0x0F;
            int wait_arg = (opcode>>4)&0x0F;
          // wait operations
          if (wait_type == 0x08) { // XCH KR,R5
              // XCH KR,R5 is also used for periph instruction
              // NOP for alu ???
              DIS (prn[wait_arg]);
              /* RAM op*/
              /* expect IO Wx x can be R or W or no io (delete) */
              if (wait_arg == 15)
                  new_check_flags |= FLG_IOWx_EXPECTED;
          } else if (wait_type == 0x06) { // FLGR5
              if (wait_arg <= 1)
                DIS ("MOV\tR5,f%c[1..4]", 'A' + wait_arg);
              else if (wait_arg == 7) {
                DIS ("PRT2_FUNC/RAM2_W");
                check_flags &= ~FLAG_IO_WRITE_PREV;
                new_check_flags |= FLG_IOW_EXPECTED;
              }
              else if (wait_arg == 9) {
                DIS ("PRT2_STEP");
              }
              else if (wait_arg == 0xA) {
                DIS ("PRT2_PRINT");
              }
              else if (wait_arg == 6) {
                DIS ("OUT PRT2");
              }
              else if (wait_arg == 8) {
                DIS ("PRT2_CLEAR/RAM2_R");
                check_flags &= ~FLAG_IO_WRITE_PREV;
                new_check_flags |= FLG_IOR_EXPECTED;
              }
              else
                  DIS("MOV\tR5,f%c[1..4]\t;????", 'A' + wait_arg);
          } else if (wait_type == 0x0F) { //Register
              // STO/RCL
              check_flags &= ~FLAG_IO_WRITE_PREV;
              if (opcode & 0x0010) {
                  DIS ("RCL %c", 'F' + ((opcode & 0xF0)>>5));
                  new_check_flags |= FLG_IOR_EXPECTED;
              } else {
                  DIS ("STO %c", 'F' + ((opcode & 0xF0)>>5));
                  new_check_flags |= FLG_IOW_EXPECTED;
              }
              if (opcode & 0x80)
                  DIS("\t;???? %d", opcode & 0xc0);
          } else if (wait_type == 0x0E) { // NOOP/LIB
              DIS (no_op[wait_arg]);
          } else {
              DIS (wait[opcode&0x0F], wait_arg);
              /* 0 : wait Dstate, 5 : tst KR[arg], 7 mov R5, #arg */
              if (!(wait_type == 0 || wait_type == 5 || wait_type == 7) &&
                      wait_arg)
                  DIS("\t;????");
          }
          break;
        }
          // generic instructions
  default:
    // generic ALU decoding
    if (sum[opcode&0x07][1] == 'x' && (opcode & 0xF0) == 0xD0)
      DIS ("XCH\t%c.%s,%c", sum[opcode&0x07][0], mask[(opcode>>8)&0x0F], sum[opcode&0x07][2]);
    else
      DIS (alu[(opcode>>3)&0x1F], sum[opcode&0x07], mask[(opcode>>8)&0x0F], N[(opcode>>8)&0x0F]);
    if ((opcode & 0xF0) != 0x00 && (opcode & 0xF0) != 0x20 && (opcode & 0xF0) != 0xD0 && *N[(opcode>>8)&0x0F] != '0')
      DIS ("|#%s", N[(opcode>>8)&0x0F]);

    /* IO write. Do not ouput log, if there may be a write access (FLG_IOx_EXPECTED) */
    if (sum[opcode&0x07][0] == 'I' && !(check_flags & (FLG_IOW_EXPECTED|FLG_IOx_EXPECTED))) {
        DIS("\t; output ignore (use R5/COND)");
        if (mask[(opcode>>8)&0x0F][0] == 'A')
            new_check_flags |= FLAG_IO_WRITE_PREV;
    }
    if (sum[opcode&0x07][0] == 'I')
        check_flags |= FLAG_IO_WRITE;
    else
        check_flags |= FLAG_IO_READ;

    break;
  }

    if (check_flags & FLAG_IO_WRITE_PREV) {
            DIS("\t; unused IO.ALL write ????");
            check_flags &= ~FLAG_IO_WRITE_PREV;
    }

    if ((check_flags & FLG_IOR_EXPECTED) && !(check_flags & FLAG_IO_READ)) {
        DIS("\t; ???? expected io read, but doing write !");
    }
    if ((check_flags & FLG_IOW_EXPECTED) && !(check_flags & FLAG_IO_WRITE)) {
        DIS("\t; ?? expected io write. Zero write !");
    }
    check_flags &= ~FLG_IOR_EXPECTED;
    check_flags &= ~FLG_IOx_EXPECTED;
    if ((check_flags & FLG_IOWx_EXPECTED) == FLG_IOWx_EXPECTED) {
        check_flags &= ~FLG_IOWx_EXPECTED;
        check_flags |= FLG_IOx_EXPECTED;
    } else if (check_flags & FLG_IOW_EXPECTED)
        check_flags &= ~FLG_IOW_EXPECTED;

    check_flags &= ~(FLAG_IO_READ|FLAG_IO_WRITE);

    if (new_check_flags) {
        if (check_flags)
            DIS("\t; ???? too much check old=0x%x new=0x%x", check_flags, new_check_flags);
        check_flags = new_check_flags;
    }
}
