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

#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define WAIT_MODE

#include "emu.h"

static struct {
  unsigned char key[16];

  unsigned char key_code;
  int key_count;

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

#define	KEY_INVERT	0x01
#define	KEY_ONOFF	0x02
    static const struct {
        unsigned char key_code;
        unsigned char flags;
        unsigned char ascii;
        unsigned vkey;
    } key_table[] = {
#ifndef SR50
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
#elif 0
        {0x15, 0, 0x1B, 0},     {0x25, 0, 0, VK_F9}, {0x35, 0, 'l', 0},   {0x55, 0, '\b', 0},  {0x65, 0, ' ', 0},
        {0x17, 0, 'p', 0}, 	     {0x27, 0, 'x', 0},   {0x37, 0, 's', 0},   {0x57, 0, 'c', 0},   {0x67, 0, 't', 0},
        {0x13, 0, 0, VK_INSERT}, {0x23, 0, '>', 0},   {0x33, 0, '<', 0},   {0x53, 0, '&', 0},   {0x63, 0, 'y', 0},
        {0x14, 0, 0, VK_DELETE}, {0x24, 0, 'e', 0},   {0x34, 0, '(', 0},   {0x54, 0, ')', 0},   {0x64, 0, '/', 0},
        {0x12, 0, 'g', 0},       {0x22, 0, '7', 0},   {0x32, 0, '8', 0},   {0x52, 0, '9', 0},   {0x62, 0, '*', 0},
        {0x11, 0, 'b', 0},       {0x21, 0, '4', 0},   {0x31, 0, '5', 0},   {0x51, 0, '6', 0},   {0x61, 0, '-', 0},
        {0x18, 0, 'r', 0},       {0x28, 0, '1', 0},   {0x38, 0, '2', 0},   {0x58, 0, '3', 0},   {0x68, 0, '+', 0},
        {0x16, 0, '$', 0},       {0x26, 0, '0', 0},   {0x36, 0, '.', 0},   {0x56, 0, 'n', 0},   {0x66, 0, '\n', 0},
#elif 1
        /* 50 */
        {0x24, 0, 'a', 0},   {0x57, 0, 's', 0},   {0x56 , 0, 'c', 0},   {0x5D, 0, 't', 0},   {0x21, 0, ' ', 0},
        {0x22, 0, 'h', 0},   {0x53, 0, 'd', 0},   {0x54 , 0, 'l', 0},   {0x51, 0, 'E', 0},   {0x31, 0, 'L', 0},
        {0x33, 0, 'x', 0},   {0x3C, 0, 'S', 0},   {0x36 , 0, 'i', 0},   {0x3D, 0, '!', 0},   {0x1A, 0, 'r', 0},
        {0x66, 0, '>', 0},   {0x68, 0, '<', 0},   {0x61 , 0, '&', 0},   {0x69 , 0, 'X', 0},   {0x1B, 0, 'Y', 0},
        {0x26, 0, 0x7F, 0},      {0x2D, 0, 'e', 0},   {0x67, 0, 'p', 0},   {0x16, 0, '/', 0},
        {0x07, 0, '7', 0},   {0x08, 0, '8', 0},   {0x09, 0, '9', 0},   {0x17, 0, '*', 0},
        {0x04, 0, '4', 0},   {0x05, 0, '5', 0},   {0x06, 0, '6', 0},   {0x12, 0, '-', 0},
        {0x01, 0, '1', 0},   {0x02, 0, '2', 0},   {0x03, 0, '3', 0},   {0x13, 0, '+', 0},
        {0x0A, 0, '0', 0},   {0x23, 0, '.', 0},   {0x27, 0, 'n', 0},   {0x11, 0, '\n', 0},
        {0x5E, KEY_ONOFF, 'R', 0},
        // printer buttons
        {0x2C, 0, '#', 0}, // PRINT
        {0x2F, KEY_ONOFF, '?', 0}, // TRACE
        {0x0C, 0, '@', 0}, // ADVANCE

#endif
        {0}
    };

static struct termios new_settings, stored_settings;

unsigned long long GetTickCount (void)
{
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC, &tp);

	return tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
}

void Sleep(unsigned long long delay)
{
	usleep(delay*1000);
}


static void key_read(int sleep)
{
    static int keydown_count;
    static char keydown;
    unsigned char AsciiChar = 0;
    unsigned vkey = 0;
    int bKeyDown = 0;
    int size;

    if (keydown_count) {
        keydown_count--;
        return;
    }
    if (!keydown) {
        if (sleep)
            if (log_flags & LOG_DEBUG)
                LOG("key sleep");
        if (read(0, &AsciiChar, 1) != 1)
            return;
#ifdef WAIT_MODE
        keydown_count = 100;
#else
        keydown_count = 1000;
#endif
        keydown = AsciiChar;
        bKeyDown = 1;
    }
    else {
#ifdef WAIT_MODE
        keydown_count = 50;
#else
        keydown_count = 500;
#endif
        AsciiChar = keydown;
        bKeyDown = 0;
        keydown = 0;
    }
    if (log_flags & LOG_DEBUG)
        LOG("r.%d %x,%c|", bKeyDown, AsciiChar, AsciiChar);
#ifdef TEST_MODE
    if (bKeyDown)
        printf("\nread key '%x',%c\n", bKeyDown, AsciiChar, AsciiChar);
#endif
#if 0
    if (AsciiChar == 0x18) // ^X
        break;
    if (AsciiChar == 0x12) { // ^R
        cpu_reset ();
        ex_cnt = 0;
    }
#endif

    for (size = 0; key_table[size].ascii || key_table[size].vkey; size++) {
        if ((key_table[size].ascii && key_table[size].ascii == AsciiChar) ||
                (key_table[size].vkey && key_table[size].vkey == vkey)) {
            if (log_flags & LOG_DEBUG)
                LOG ("{K=%02X}\n", key_table[size].key_code);
            if (key_table[size].flags & KEY_INVERT)
                bKeyDown = !bKeyDown;
            if (bKeyDown) {
                if (key_table[size].flags & KEY_ONOFF)
                    cpu.key[key_table[size].key_code & 0x0F] ^= 1 << ((key_table[size].key_code >> 4) & 0x07);
                else
                    cpu.key[key_table[size].key_code & 0x0F] |= 1 << ((key_table[size].key_code >> 4) & 0x07);
            } else {
                if (!(key_table[size].flags & KEY_ONOFF))
                    cpu.key[key_table[size].key_code & 0x0F] &= ~(1 << ((key_table[size].key_code >> 4) & 0x07));
            }
            break;
        }
#if 1
        else if (AsciiChar == 'v' && !(key_table[size].flags))
            cpu.key[key_table[size].key_code & 0x0F] &= ~(1 << ((key_table[size].key_code >> 4) & 0x07));
#endif
    } //for
}

/*
 * SR50 : 2 scan with key press, 1(2*) scan no key (* if cond is unset, there will be one more no key loop)
 * SR50.1 : 2 scan with key press, 2(3*) scan no key
 * SR50A : 2 scan with key press, 2(3*) scan no key
 *
 * */
static int key_read2(int sleep)
{

    unsigned char AsciiChar = 0;
    int size;
    unsigned vkey = 0;
    if (cpu.key_count >= -1) {
        /* 2 repeat key
         * 1 remove key & no key
         * 0 no key
         * -1 no key
         */
        LOG("key count %d", cpu.key_count);
        if (cpu.key_count == 1)
            cpu.key[cpu.key_code & 0x0F] ^= 1 << ((cpu.key_code >> 4) & 0x07);
        cpu.key_count--;
        return 0;
    }
    int ret = read(0, &AsciiChar, 1);
    if (ret != 1) {
        return -1;
    }
    for (size = 0; key_table[size].ascii || key_table[size].vkey; size++) {
        if ((key_table[size].ascii && key_table[size].ascii == AsciiChar) ||
                (key_table[size].vkey && key_table[size].vkey == vkey)) {
            if (log_flags & LOG_DEBUG)
                LOG ("{K=%02X}\n", key_table[size].key_code);
            cpu.key[key_table[size].key_code & 0x0F] |= 1 << ((key_table[size].key_code >> 4) & 0x07);
            cpu.key_code = key_table[size].key_code;
            cpu.key_count = 2;
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

int key_process(void *priv, struct bus *bus)
{
    if (bus->sstate == 15 && !bus->write) {
        int sleep = 0;
        static int keypress = 0;
        bus->key_line = 0;
        if (bus->idle && (bus->irg & 0xFF08) == 0x0800 && !(bus->ext & EXT_HOLD)) {
            LOG("key scan ");
#ifdef WAIT_MODE
            sleep = 1;
            /* full scan */
            new_settings.c_cc[VTIME] = 10; // timeout (tenths of a second)
            new_settings.c_cc[VMIN] = 1; // minimum number of characters
            tcsetattr(0, TCSANOW, &new_settings);
#endif

            if (key_read2(sleep) < 0)
                return -1;
        }
        if (sleep) {
            /* full scan */
            new_settings.c_cc[VTIME] = 0; // timeout (tenths of a second)
            new_settings.c_cc[VMIN] = 0; // minimum number of characters
            tcsetattr(0, TCSANOW, &new_settings);
        }

        bus->key_line = cpu.key[bus->dstate];
        //if (bus->key_line)
        //printf("%x\n", bus->key_line);

#if 0
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

void key_init(void)
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

    new_settings.c_cc[VTIME] = 0; // timeout (tenths of a second)
    new_settings.c_cc[VMIN] = 0; // minimum number of characters

    // apply the new settings
    tcsetattr(0, TCSANOW, &new_settings);
}
