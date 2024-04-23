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

#define BUFFER_SIZE 20
struct print {
    /* NULL character at the end */
    char buffer[BUFFER_SIZE+1];
    int head;
    int busy;
    uint32_t mask;
    const char *print_font;
};

/* table are present in ti59 service manual and
 * also in user doc (for snd op 00-08) */
static const char print_font[64] = {
  ' ','0','1','2','3','4','5','6',
  '7','8','9','A','B','C','D','E',
  '-','F','G','H','I','J','K','L',
  'M','N','O','P','Q','R','S','T',
  '.','U','V','W','X','Y','Z','+',
  'x','*','s','p','e','(',')',',', /* s=sqrt, p=pi */
  '^','%','|','/','=','\'', '#','~', /* | is xchg, # is ^x, ~ is nX */
  'z','?',':','!',']','"','[','$' /* z is ^2, ] is 2nd, " is delta, [ is product, $ is sum */
};

/* . : ok
 * - : ok
 * / : ok
 * + : ok
 * = : ok
 * x : ok
 * : : ok
 * # / ^x : ok
 * e : ok
 * s/sqrt : ok
 * z/ ^2 : ok
 * ! : ?
 * |/xch : ok
 * , : ok
 * '
 * (
 * ~/nX
 * "/delta : ok
 * ] / 2nd
 * ) : ok
 * $/sum : ok
 * ^
 * *
 * p/pi
 * [ / prod : ok
 * % : ok
 */
static const char print_font_sr60[64] = {
  ' ','0','1','2','3','4','5','6',
  '7','8','9','.','z','#','!','"',
  '=','-','+',':','x','\'', '(',')',
  's','^','$','[','/','|','e',']',
  'Q','R','S','T','U','V','W','X',
  'Y','Z','*','p','?',',','%','~',
  'A','B','C','D','E','F','G','H',
  'I','J','K','L','M','N','O','P',
};

/* should reuse print font symbol ! */
static const struct {
  unsigned char code;
  char str[3];
} print_func[] = {
  {0x00, "   "},
  {0x11, " = "},
  {0x12, " - "},
  {0x13, " + "},
  {0x16, " / "}, // _:_
  {0x17, " x "},
  {0x1A, "xsY"}, //x_sqrt_Y
  {0x1B, "Y^x"}, //Yx_
  {0x21, "CLR"},
  {0x22, "INV"},
  {0x23, "DPT"},
  {0x26, "CE "},
  {0x27, "+/-"},
  {0x2D, "EE "},
  {0x31, "e^x"}, // ex_
  {0x33, "x^2"}, // x2_
  {0x36, "1/x"},
  {0x3C, "sX "}, // sqrt_X_
  {0x3D, "X|Y"}, // X exchange Y
  {0x51, "LNX"},
  {0x53, "PRM"},
  {0x54, " % "},
  {0x56, "COS"},
  {0x57, "SIN"},
  {0x5D, "TAN"},
  {0x61, "SUM"},
  {0x66, "STO"},
  {0x67, "pi "}, //_pi_
  {0x68, "RCL"},
  {0x69, "$+ "}, //SUM+
  {0x70, "ERR"},
  {0x71, " ( "},
  {0x72, " ) "},
  {0x73, "LRN"},
  {0x74, "RUN"},
  {0x76, "HLT"},
  {0x78, "STP"},
  {0x7A, "GTO"},
  {0x7C, "IF "},
  {0, {0}}
};

/*
 * 0 1010 ____ 1000
 *
 * 6 "OUT\tPRT",
 * 7 "OUT\tPRT_FUNC",
 * 8 "PRT_CLEAR",
 * 9 "PRT_STEP",
 * A "PRT_PRINT",
 * B "PRT_FEED",
 *
 * cycle   irg[in]         ext[in]  IO
 * 1       OUT PRT/LOAD    code     x  #code in ext[3-8]
 * 2       x               x        x
 * ....
 * 1    OUT PRT_FUNC/FUNC  code     x  #code in ext[3-9]
 * 2       x               x        x
 * ....
 * 1       PRT_CLEAR       x        x
 * 1       PRT_STEP        x        x  #can set busy
 * 1       PRT_PRINT       x        x
 * 1       PRT_FEED        x        x
 */

static void print_step(struct print *print)
{
    if (!print->head)
        print->head = BUFFER_SIZE;
    print->head--;
}

static int print_process(void *priv, struct bus *bus)
{
    struct print *print = priv;
    if (bus->sstate == 15 && !bus->write) {
        if ((bus->irg & 0xFF0F) != print->mask)
            return 0;
        switch (bus->irg) {
            case 0x0A68:
            case 0x0A66:
            {
                /* load char */
                int code = (bus->ext >> 3) & 0x3F;
		        print->buffer[print->head] = print->print_font[code];
                LOG("PRINT_CHAR[%d]='%c' ", print->head, print->buffer[print->head]);
                print_step(print);
                break;
            }
            case 0x0A78:
            case 0x0A76:
            {
                /* load func */
                int code = (bus->ext >> 3) & 0x7F;
                int i;
                for (i = 0; *print_func[i].str; i++) {
                    if (code == print_func[i].code) {
                        for (int j = 2; j >= 0; j-- ) {
                            print->buffer[print->head] = print_func[i].str[j];
                            print_step(print);
                        }
                        LOG("PRINT_FUNC[%d]='%.3s' ", print->head, print_func[i].str);
                        break;
                    }
                }
                if (!*print_func[i].str) {
                    LOG("PRINT[%d]='%d not fund' ", print->head, code);
                    for (int j = 2; j >= 0; j-- ) {
                        print->buffer[print->head] = '?';
                        print_step(print);
                    }
                    LOG("PRINT_FUNC[%d]='%.3s' ", print->head, "???");
                }

                break;
            }
            case 0x0A88:
            case 0x0A86:
                /* clear */
                LOG("PRINT_CLEAR[%d]='%.20s' ", print->head, print->buffer);
                memset(print->buffer, ' ', BUFFER_SIZE);
                print->head = BUFFER_SIZE - 1;
                break;
            case 0x0A98:
            case 0x0A96:
                /* step */
                if (print->busy) {
                    //report busy
                    ;
                }
                else {
                    print_step(print);
                }
                break;
            case 0x0AA8:
            case 0x0AA6:
                /* print */
                if (bus->irg == 0x0AA6)
                    display_ext(print->buffer);
                else
                    display_print(print->buffer);
                LOG("PRINT[%d]='%.20s' ", print->head, print->buffer);
                break;
            case 0x0AB8:
                /* advance half line */
                break;
        }
    }
    return 0;
}



int printer_init(struct chip *chip, enum printer_type type)
{
    struct print *printer;

    printer = malloc(sizeof(*printer));
    memset(printer->buffer, 'X', BUFFER_SIZE);
    printer->buffer[BUFFER_SIZE] = '\0';
    printer->head = 5;
    printer->busy = 0;
    chip->priv = printer;
    chip->process = print_process;
    if (type == TMC0253)
        printer->mask = 0x0A06;
    else
        printer->mask = 0x0A08;

    if (type == TMC0251)
        printer->print_font = print_font;
    else
        printer->print_font = print_font_sr60;

    return 0;
}
