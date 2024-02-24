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

#include <stdint.h>
#include <assert.h>

#include "bus.h"
#include "emu.h"

/**
 *  http://www.datamath.org/Chips/TMC0560.htm
 *  signal :
 *     clk/idle : sync input
 *     ext : input
 *     IRG : out/HiZ
 *
 *  send 13 bits instruction in S3...S15 LSB fist
 */


struct brom_state {
	unsigned int pc;
	uint16_t last_ext;
	uint16_t last_irg;

	uint16_t data[1024];
	int end;
    int start;
};

#define BROM_CS 0
#define PC_ADDR(x) ((x) & 0x3FF)


static unsigned int handle_branch(const struct brom_state *bstate, int cond)
{
	int bcond = (bstate->last_irg >> 11) & 1;
	int boffset = (bstate->last_irg >> 1) & 0x3FF;
	int bneg = (bstate->last_irg) & 1;

    //DIS("branch %d %d\n", !!cond, bcond);
	if (!!cond == bcond) {
		if (bneg)
			boffset = -boffset;
		return bstate->pc + boffset;
	}
	return bstate->pc + 1;
}

static void brom_process_out(struct brom_state *bstate, struct bus *bus_state)
{
    /* optimisation output state early. S4 */
    if (bus_state->sstate != 4)
        return;

    /* with is implementation, the cpu
     * nHOLD(ext) WAIT(irg) XXXX (exec)
     * HOLD(ext) WAIT(irg)  WAIT (exec)
     * HOLD(ext) WAIT(irg)  WAIT (exec)
     * HOLD(ext) WAIT(irg)  WAIT (exec)
     * HOLD(ext) WAIT(irg)  WAIT (exec)
     * nHOLD(ext) KEY(irg)  WAIT (exec)
     * nHOLD(ext) XXXX(irg) KEY (exec)
     */
    if (bus_state->ext & EXT_HOLD) {
        /* output same instruction 
         * HOLD bit has priority over PREG bit
         * (see HW manual External debugger)
         * */
        bstate->pc += 0;
    }
    /* there is one instruction delay :
     * - set KR[1] was set cycle n-2
     * - PREG was set in cycle n-1, but we don't have 
     *   new address at the start of this cycle
     *   and output next instruction
     * - use new address at this cycle
     */
    else if (bstate->last_ext & EXT_PREG) {
        bstate->pc = bstate->last_ext >> 3;
        //DIS("PREG 0x%04x\n", bstate->last_ext);
    }
    /* check branch instruction to update address */
    else if (bstate->last_irg & IRG_BRANCH_MASK) {
        bstate->pc = handle_branch(bstate, bus_state->ext & EXT_COND);
    }
    else {
        bstate->pc++;
    }


    //DIS("addr %d last_irg 0x%04x\n", bstate->pc, bstate->last_irg);

    /* output instruction if selected. First rom used 1K mask
     * but latter can be up to 2.5K.
     * Don't use mask for simplicity
     */
    if (bstate->pc >= bstate->start && bstate->pc < bstate->end) {
        unsigned int addr = bstate->pc - bstate->start;
        /* dbg check nobody already write irg 
         * XXX 0 instruction is valid
         */
        assert(bus_state->irg == 0);
        bus_state->irg = bstate->data[addr];
        bus_state->addr = addr;
        //DIS("addr%d new irg%04x\n", addr, bus_state->irg);
    }
}

static void brom_process_in(struct brom_state *bstate, struct bus *bus_state)
{
    /* optimisation. Read output at last state */
    if (bus_state->sstate == 15) {
        /* save irg,ext for next cycle */
        bstate->last_irg = bus_state->irg;
        bstate->last_ext = bus_state->ext;
    }
}

int brom_process(void *priv, struct bus *bus_state)
{
    struct brom_state *bstate = priv;
    if (bus_state->write)
        brom_process_out(bstate, bus_state);
    else
        brom_process_in(bstate, bus_state);

    return 0;
}

void *brom_init(const char *name)
{
    struct brom_state *bstate = malloc(sizeof(struct brom_state));
    int size;
    int base;
    if (!bstate)
        return NULL;
    bstate->last_ext = 0;
    bstate->last_irg = 0;
    bstate->pc = 1<<16;

    size = load_dump(bstate->data, sizeof(bstate->data), name, &base);
    //ret = load_dump(bstate->data, sizeof(bstate->data), "rom-SR50r1/TMC0521E.txt");
    //ret = load_dump(bstate->data, sizeof(bstate->data), "rom-SR50A/TMC0531A.txt");
	bstate->end = base + size;
    bstate->start = base;
    printf("rom '%s'  base %d size %d\n",
            name, base, size);
    return bstate;
}
