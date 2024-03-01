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

//#define KEEP_RUN

#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>
#include <string.h>

#include "emu.h"


#define	KEY_INVERT	0x01
#define	KEY_ONOFF	0x02
struct keymap {
        unsigned char key_code;
        unsigned char flags;
        unsigned char ascii;
        unsigned char dummy;
};

static struct {
  unsigned char key[16];
  const struct keymap *keymap;

  unsigned char key_code;
  int key_count;
  int keyboardidle;

  // CPU cycle counter (used to simulate real CPU frequency)
  unsigned cycle;
  long long tick;
  unsigned ex_cnt;

} cpu;
// 455kHz / 2 / 16 = 14219
// 20ms ~ 284.375 instructions
// 50ms ~ 710.9375 instructions
#define	CPU_FREQ	455000	//[Hz]
#define	EMUL_TICK	20	//[ms]
#define	EMUL_CYCLE	((EMUL_TICK * CPU_FREQ) / 2 / 16 / 1000)


static const struct keymap key_table_ti58[] = {
        {0x11, 0, 'A', 0},     {0x21, 0, 'B', 0}, {0x31, 0, 'C', 0}, {0x51, 0, 'D', 0}, {0x61, 0, 'E', 0},
        {0x12, 0, 0x1B, 0},     {0x22, 0, 'I', 0}, {0x32, 0, 'l', 0},   {0x52, 0, 0x7F, 0},  {0x62, 0, ' ', 0},
        {0x13, 0, 'p', 0}, 	     {0x23, 0, 'x', 0},   {0x33, 0, 's', 0},   {0x53, 0, 'c', 0},   {0x63, 0, 't', 0},
        {0x14, 0, 'i', 0},       {0x24, 0, '>', 0},   {0x34, 0, '<', 0},   {0x54, 0, '&', 0},   {0x64, 0, 'y', 0},
        {0x15, 0, 'd', 0},       {0x25, 0, 'e', 0},   {0x35, 0, '(', 0},   {0x55, 0, ')', 0},   {0x65, 0, '/', 0},
        {0x16, 0, 'g', 0},       {0x26, 0, '7', 0},   {0x36, 0, '8', 0},   {0x56, 0, '9', 0},   {0x66, 0, '*', 0},
        {0x17, 0, 'b', 0},       {0x27, 0, '4', 0},   {0x37, 0, '5', 0},   {0x57, 0, '6', 0},   {0x67, 0, '-', 0},
        {0x18, 0, 'r', 0},       {0x28, 0, '1', 0},   {0x38, 0, '2', 0},   {0x58, 0, '3', 0},   {0x68, 0, '+', 0},
        {0x19, 0, '$', 0},       {0x29, 0, '0', 0},   {0x39, 0, '.', 0},   {0x59, 0, 'n', 0},   {0x69, 0, '\n', 0},
        // printer buttons
        {0x2C, 0, '#', 0}, // PRINT
        {0x2F, KEY_ONOFF, '?', 0}, // TRACE
        {0x0C, 0, '@', 0}, // ADVANCE
        // card buttons
        {0x4A, KEY_INVERT, '~', 0}, // card inserted
        {0}
};

static const char *key_help_ti58 =
      "[A]=A         [B]=B         [C]=C       [D]=D        [E]=E\n"
	  "[2nd]=Esc     [INV]=I       [ln\\log]=l  [CE\\CP]=Back [CLR]=Space\n"
	  "[LRN\\Pgm]=p   [x<>t\\P->R]=x [x^2\\sin]=s [sqrt\\cos]=c [1/x\\tan]=t\n"
	  "[SST\\Ins]=i   [STO\\CMs]=>   [RCL\\Exc]=< [SUM\\Prd]=&  [Y^x\\Ind]=y\n"
	  "[BST\\Del]=d   [EE\\Eng]=e    [(\\Fix]=(   [)\\Int]=)    [/\\|x|]=/\n"
	  "[GTO\\Pause]=g [7\\x=t]=7     [8\\Nop]=8   [9\\Op]=9     [x\\Deg]=*\n"
	  "[SBR\\Lbl]=b   [4\\x>=t]=4    [5\\S+]=5    [6\\avg]=6    [-\\Rad]=-\n"
	  "[RST\\StFlg]=r [1\\IfFlg]=1   [2\\D.MS]=2  [3\\pi]=3     [+\\Grad]=+\n"
	  "[R/S]=$       [0\\Dsz]=0     [.\\Adv]=.   [+/-\\Prt]=n  [=\\List]=Enter\n"
      "-------\n"
	  "PRINT=#        TRACE=?        ADVANCE=@\n";

static const struct keymap key_table_sr52[] = {
        {0x11, 0, 'A', 0},   {0x21, 0, 'B', 0},  {0x31, 0, 'C', 0},    {0x51, 0, 'D', 0},  {0x61, 0, 'E', 0},
        {0x12, 0, 0x1B, 0},  {0x22, 0, 'I', 0},  {0x32, 0, 'l', 0},    {0x52, 0, 0x7F, 0}, {0x62, 0, ' ', 0},
        {0x13, 0, 'p', 0}, 	 {0x23, 0, 's', 0},  {0x33, 0, 'c', 0},    {0x53, 0, 't', 0},  {0x63, 0, 'S', 0},
        {0x14, 0, 'g', 0},   {0x24, 0, '>', 0},  {0x34, 0, '<', 0},    {0x54, 0, '&', 0},  {0x64, 0, 'y', 0},
        {0x15, 0, 'b', 0},   {0x25, 0, 'e', 0},  {0x35, 0, '(', 0},   {0x55, 0, ')', 0},   {0x65, 0, '/', 0},
        {0x16, 0, 'i', 0},   {0x07, 0, '7', 0},   {0x08, 0, '8', 0},   {0x09, 0, '9', 0},    {0x66, 0, '*', 0},
        {0x17, 0, 'd', 0},   {0x04, 0, '4', 0},   {0x05, 0, '5', 0},   {0x06, 0, '6', 0},    {0x67, 0, '-', 0},
        {0x18, 0, 'r', 0},   {0x01, 0, '1', 0},   {0x02, 0, '2', 0},   {0x03, 0, '3', 0},    {0x68, 0, '+', 0},
        {0x19, 0, '$', 0},   {0x0A, 0, '0', 0},   {0x29, 0, '.', 0},   {0x59, 0, 'n', 0},    {0x69, 0, '\n', 0},
        {0x5E, KEY_ONOFF, 'R', 0},
        {0x4A, KEY_ONOFF, '~', 0}, // card inserted
        // printer buttons
        {0x2C, 0, '#', 0}, // PRINT
        {0x2F, KEY_ONOFF, '?', 0}, // TRACE
        {0x0C, 0, '@', 0}, // ADVANCE

        {0}
};
static const char *key_help_sr52 =
      "[A]=A          [B]=B          [C]=C         [D]=D         [E]=E\n"
	  "[2nd]=Esc      [INV]=I        [ln\\log]=l   [CE\\x!]=Back [CLR\\1/x]=Space\n"
      "[LRN\\IND]=p    [sin\\D.MS]=s   [cos\\D/R]=c  [tan\\P/R]=t  [xsqrty\\sqrt]=S\n"
	  "[GTO\\LBL]=g    [STO\\CMs]=>    [RCL\\Exc]=<  [SUM\\Prd]=&  [Y^x\\x^2]=y\n"
      "[SBR\\rtn]=b    [EE\\Fix]=e     [(\\dsz]=(    [)\\pi]=)     [/\\StFlg]=/\n"
	  "[Ins/del]=i    [7]=7          [8]=8        [9]=9        [x\\IfFlg]=*\n"
	  "[SST\\BST]=d    [4]=4          [5]=5        [6]=6        [-\\IfErr]=-\n"
	  "[Hlt\\rset]=r   [1]=1          [2]=2        [3]=3        [+\\IfPos]=+\n"
	  "[R\\read]=$     [0/list]=0     [./ptr]=.    [+/-/pap]=n  [=/IfZero]=Enter\n"
      "----------\n"
      "RAD=R\n";


static const struct keymap key_table_sr56[] = {
        {0x11, 0, 0x1B, 0},  {0x21, 0, 'I', 0},   {0x31 , 0, 'l', 0},   {0x51, 0, 't', 0},   {0x61, 0, ' ', 0},
        {0x12, 0, 'p', 0},   {0x22, 0, 'g', 0},   {0x32 , 0, 's', 0},   {0x52, 0, 'c', 0},   {0x62, 0, 't', 0},
        {0x13, 0, 'i', 0},   {0x23, 0, 'X', 0},   {0x33 , 0, '>', 0},   {0x53, 0, '<', 0},   {0x63, 0, '&', 0},
        {0x14, 0, '$', 0},   {0x24, 0, 'r', 0},   {0x34 , 0, 'x', 0},   {0x54 , 0, 'e', 0},  {0x64, 0, 'y', 0},
        {0x15, 0, 0x7F, 0},  {0x25, 0, '(', 0},   {0x35, 0, ')', 0},    {0x55, 0, '/', 0},
        {0x07, 0, '7', 0},   {0x08, 0, '8', 0},   {0x09, 0, '9', 0},    {0x56, 0, '*', 0},
        {0x04, 0, '4', 0},   {0x05, 0, '5', 0},   {0x06, 0, '6', 0},    {0x57, 0, '-', 0},
        {0x01, 0, '1', 0},   {0x02, 0, '2', 0},   {0x03, 0, '3', 0},    {0x58, 0, '+', 0},
        {0x0A, 0, '0', 0},   {0x29, 0, '.', 0},   {0x39, 0, 'n', 0},    {0x59, 0, '\n', 0},
        {0x5E, KEY_ONOFF, 'R', 0},
        // printer buttons
        {0x2C, 0, '#', 0}, // PRINT
        {0x2F, KEY_ONOFF, '?', 0}, // TRACE
        {0x0C, 0, '@', 0}, // ADVANCE

        {0}
};
static const char *key_help_sr56 =
      "[2nd]=Esc      [INV]=I       [ln\\log]=l   [e^x\\10^x]=E [CLR]=Space\n"
      "[LRN\\f(n)]=p  [GTO\\???]=g  [sin\\???]=s  [cos\\Int]=c  [tan\\1/x]=t\n"
      "[SST\\BST]=i   [x<>t\\??]=X  [STO\\CMs]=>  [RCL\\Exc]=<  [SUM\\Prd]=&\n"
      "[R/S\\NOP]=$   [RST\\???]=r  [x^2\\sqrt]=x [EE\\Fix]=e   [Y^x\\xsqrty]=y\n"
      "[CE\\CP]=Back  [(\\subr]=(   [)\\rtn]=)    [/\\pause]=/\n"
	  "[7]=7          [8]=8       [9]=9       [x/pi]=*\n"
	  "[4/SUM+]=4     [5/SUM-]=5  [6]=6       [-/RAD]=-\n"
	  "[1/Mean]=1     [2/P->R]=2  [3/R->P]=3  [+]=+\n"
	  "[0/S.Dev.]=0   [./ptr]=.   [+/-/pap]=n [=/list]=Enter\n"
      "----------\n"
      "RAD=R\n";

static const struct keymap key_table_sr51II[] = {
        {0x15, 0, 0x1B, 0},  {0x25, 0, 's', 0},  {0x35, 0, 'c', 0},    {0x55, 0, 't', 0}, {0x65, 0, ' ', 0},
        {0x17, 0, 'I', 0}, 	 {0x27, 0, '%', 0},  {0x37, 0, 'l', 0},    {0x57, 0, 'E', 0},  {0x67, 0, 'r', 0},
        {0x13, 0, 'X', 0},   {0x23, 0, 'x', 0},  {0x33, 0, 'S', 0},    {0x53, 0, 'i', 0},  {0x63, 0, 'y', 0},
        {0x14, 0, 'U', 0},   {0x24, 0, 'e', 0},  {0x34, 0, '(', 0},   {0x54, 0, ')', 0},   {0x64, 0, '/', 0},
        {0x12, 0, '>', 0},   {0x22, 0, '7', 0},   {0x32, 0, '8', 0},   {0x52, 0, '9', 0},    {0x62, 0, '*', 0},
        {0x11, 0, '<', 0},   {0x21, 0, '4', 0},   {0x31, 0, '5', 0},   {0x51, 0, '6', 0},    {0x61, 0, '-', 0},
        {0x18, 0, '&', 0},   {0x28, 0, '1', 0},   {0x38, 0, '2', 0},   {0x58, 0, '3', 0},    {0x68, 0, '+', 0},
        {0x16, 0, 0x7F, 0},  {0x26, 0, '0', 0},   {0x36, 0, '.', 0},   {0x56, 0, 'n', 0},    {0x66, 0, '\n', 0},

        {0}
};
static const char *key_help_sr51II =
	  "[2nd]=Esc      [sin\\sinh]=s        [cos\\cosh]=c   [tan\\tanh]=t [CLR\\CA]=Space\n"
	  "[INV]=I        [%%\\D%%]=%%       [ln\\log]=l    [e^x\\10^x]=E  [xsqrty\\x!]=r\n"
      "[xy]=X         [x^2\\MEAN]=x      [sqrt\\S.DEV]=S [1/x\\VAR]=i [Y^x\\CORR]=y\n"
      "[SUM+\\SUM-]=U  [EE\\Eng]=e  [(\\const]=(    [)\\pi]=)     [/\\Slope]=/\n"
      "[STO\\Fix]=>   [7]=7          [8]=8        [9]=9        [x\\Intcp]=*\n"
	  "[RCL\\EXC]=<    [4]=4          [5]=5        [6]=6        [-\\x']=-\n"
	  "[SUM\\Prd]=&   [1]=1          [2]=2        [3]=3        [+\\y']=+\n"
	  "[CE]=Back     [0]=0     [.]=.    [+/-]=n  [=]=Enter\n";



static const struct keymap key_table_sr50[] = {
        /* 50 */
        {0x24, 0, 'a', 0},   {0x57, 0, 's', 0},   {0x56 , 0, 'c', 0},   {0x5D, 0, 't', 0},   {0x21, 0, ' ', 0},
        {0x22, 0, 'h', 0},   {0x53, 0, 'd', 0},   {0x54 , 0, 'l', 0},   {0x51, 0, 'E', 0},   {0x31, 0, 'L', 0},
        {0x33, 0, 'x', 0},   {0x3C, 0, 'S', 0},   {0x36 , 0, 'i', 0},   {0x3D, 0, '!', 0},   {0x1A, 0, 'r', 0},
        {0x66, 0, '>', 0},   {0x68, 0, '<', 0},   {0x61 , 0, '&', 0},   {0x69 , 0, 'X', 0},   {0x1B, 0, 'Y', 0},
        {0x26, 0, 0x7F, 0},  {0x2D, 0, 'e', 0},   {0x67, 0, 'p', 0},   {0x16, 0, '/', 0},
        {0x07, 0, '7', 0},   {0x08, 0, '8', 0},   {0x09, 0, '9', 0},   {0x17, 0, '*', 0},
        {0x04, 0, '4', 0},   {0x05, 0, '5', 0},   {0x06, 0, '6', 0},   {0x12, 0, '-', 0},
        {0x01, 0, '1', 0},   {0x02, 0, '2', 0},   {0x03, 0, '3', 0},   {0x13, 0, '+', 0},
        {0x0A, 0, '0', 0},   {0x23, 0, '.', 0},   {0x27, 0, 'n', 0},   {0x11, 0, '\n', 0},
        {0x5E, KEY_ONOFF, 'R', 0},
        {0}
};

static const char *key_help_sr50 =
      "[arc]=a        [sin]=s      [cos]=c      [tan]=t     [C]=Space\n"
      "[hyp]=h        [D/R]=d      [ln]=l      [exp]=E      [log]=L\n"
      "[x^2]=x        [sqrt]=S     [1/x]=i      [x!]=!      [xsqrty]=r\n"
      "[STO]=>        [RCL]=<      [SUM]=&      [xy]=X      [Y^x]=Y\n"
      "[CE]=Back      [EE]=e       [pi]=p    [/]=/\n"
	  "[7]=7       [8]=8    [9]=9     [x]=*\n"
	  "[4]=4       [5]=5    [6]=6     [-]=-\n"
	  "[1]=1       [2]=2    [3]=3     [+]=+\n"
	  "[0]=0       [.]=.    [+/-]=n   [=]=Enter\n"
      "----------\n"
      "RAD=R\n";

static struct termios new_settings, stored_settings;

static unsigned long long GetTickCount (void)
{
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC, &tp);

	return tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
}

static void Sleep(unsigned long long delay)
{
	usleep(delay*1000);
}


/*
 * SR50 : 2 scan with key press, 1(2*) scan no key (* if cond is unset, there will be one more no key loop)
 * SR50.1 : 2 scan with key press, 2(3*) scan no key
 * SR50A : 2 scan with key press, 2(3*) scan no key
 * SR51-II : 2 scan with key press, 3(4*?) scan no key
 *
 * */
static int key_read2(struct bus *bus)
{

    unsigned char AsciiChar = 0;
    int size;
    int idle = bus->idle;
    if (cpu.keyboardidle && !idle)
        return 0;

    if (cpu.key_count >= -1) {
        /* 2 repeat key
         * 1 remove key & no key
         * 0 no key
         * -1 no key
         */
        LOG("key count %d", cpu.key_count);
        //if (cpu.key_count > 1 && bus->dstate == (cpu.key_code & 0x0F))
        //    bus->key_line |= 1 << ((cpu.key_code >> 4) & 0x07);
        if (cpu.key_count <= 1)
            cpu.key_count--;
        return 0;
    }
    int ret = read(0, &AsciiChar, 1);
#ifndef KEEP_RUN
    if (ret != 1) {
        return -1;
    }
#endif
    for (size = 0; cpu.keymap[size].ascii; size++) {
        if (cpu.keymap[size].ascii && cpu.keymap[size].ascii == AsciiChar) {
            if (log_flags & LOG_DEBUG)
                LOG ("{K=%02X}\n", cpu.keymap[size].key_code);
            LOG("r.1=%c", AsciiChar);
            if (!(cpu.keymap[size].flags & KEY_ONOFF)) {
                //cpu.key[cpu.keymap[size].key_code & 0x0F] |= 1 << ((cpu.keymap[size].key_code >> 4) & 0x07);
                cpu.key_code = cpu.keymap[size].key_code;
                cpu.key_count = 3;
            }
            else {
                /* only revert key state */
                cpu.key[cpu.keymap[size].key_code & 0x0F] ^= 1 << ((cpu.keymap[size].key_code >> 4) & 0x07);
            }

#ifdef TEST_MODE
            static int last_op;
            if (!isdigit(AsciiChar) && AsciiChar != '.' && AsciiChar != 'n') {
                printf("|      %s key %c (%x)\n", display_debug(),
                        AsciiChar=='\n'?'=':AsciiChar, AsciiChar);
                last_op = 1;
            }
            else if (last_op) {
                printf("|      %s res\n", display_debug());
                last_op = 0;
            }
#endif
            break;
        }
    }
    return 1;
}

static int key_process(void *priv, struct bus *bus)
{
    /* process the key at state S15 just after D state change and before key processing
     * at state S0 (for hold reason)
     */
    if (bus->sstate == 15 && !bus->write) {
        if ((bus->irg & 0xFF08) == 0x0800 && !(bus->ext & EXT_HOLD)) {
            if (key_read2(bus) < 0)
                return -1;
        }
        if (bus->dstate == (cpu.key_code & 0x0F) && cpu.key_count > 1) {
            bus->key_line |= 1 << ((cpu.key_code >> 4) & 0x07);
            cpu.key_count--;
        }

        bus->key_line |= cpu.key[bus->dstate];

#ifdef KEEP_RUN
        // real speed simulation
        // 455kHz / 2 / 16 = 14219
        // 20ms ~ 284.375 instructions
        // 50ms ~ 710.9375 instructions
        if ((cpu.cycle - cpu.ex_cnt) > EMUL_CYCLE) {
            cpu.ex_cnt += EMUL_CYCLE;
            while ((GetTickCount () - cpu.tick) < EMUL_TICK)
                Sleep (EMUL_TICK);
            cpu.tick += EMUL_TICK;
        }
        if (bus->idle)
            cpu.cycle += 4;
        else
            cpu.cycle++;
#endif


    }
    return 0;
}

static void key_init2(void)
{
    cpu.tick = GetTickCount ();
    setbuf(stdout, NULL);

    //	int flags = fcntl(0, F_GETFL, 0);
    //	fcntl(0, F_SETFL, flags | O_NONBLOCK);

    tcgetattr(0, &stored_settings);

    // copy existing setting flags
    new_settings = stored_settings;

    // modify flags
    // first, disable canonical mode
    // (canonical mode is the typical line-oriented input method)
    new_settings.c_lflag &= (~ICANON);
    new_settings.c_lflag &= (~ECHO); // don't echo the character
    //new_settings.c_lflag &= (~ISIG); // don't automatically handle control-C

#ifdef KEEP_RUN
    new_settings.c_cc[VTIME] = 0; // timeout (tenths of a second)
    new_settings.c_cc[VMIN] = 0; // minimum number of characters
    cpu.keyboardidle = 0;
#else
    new_settings.c_cc[VTIME] = 10; // timeout (tenths of a second)
    new_settings.c_cc[VMIN] = 1; // minimum number of characters
    cpu.keyboardidle = 1;
#endif

    // apply the new settings
    tcsetattr(0, TCSANOW, &new_settings);

}


int key_init(struct chip *chip, const char *name)
{
    if (!name)
        name = "sr50";
    key_init2();
    chip->priv = NULL;
    chip->process = key_process;

    printf("keymap %s\n", name);
    if (!strcmp(name, "ti58")) {
        /* if unset, enable card reader code
         */
        cpu.key[7] |= (1 << KR_BIT);
        cpu.keymap = key_table_ti58;
        printf(key_help_ti58);
    }
    else if (!strcmp(name, "ti59")) {
        /* close card reader */
        cpu.key[10] |= (1 << KR_BIT);
        cpu.keymap = key_table_ti58;
        printf(key_help_ti58);
    }
    else if (!strcmp(name, "sr51-II")) {
        cpu.keymap = key_table_sr51II;
        printf(key_help_sr51II);
    }
    else if (!strcmp(name, "sr52")) {
        cpu.keymap = key_table_sr52;
        printf(key_help_sr52);
    }
    else if (!strcmp(name, "sr56")) {
        cpu.keymap = key_table_sr56;
        printf(key_help_sr56);
    }
    else {
        cpu.keymap = key_table_sr50;
        printf(key_help_sr50);
    }
    return 0;
}
