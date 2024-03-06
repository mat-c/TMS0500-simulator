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

#pragma once

#include <stdint.h>


struct bus {
	/* 16 bits from cpu/crom (LSB first on bus S0..S15)
	 * ext[0] : PREG (load new pc)
	 * ext[1] : COND (condition flag/used for branch)
	 * ext[2] : HOLD (do not auto increment pc)
	 * ext[15..3] : data
	 *    if PREG=1 : new pc
	 *    can be use to send/receive data
	 */
	#define EXT_PREG 1
	#define EXT_COND 2
	#define EXT_HOLD 4
	uint16_t ext;

	/*  13 bits instruction (LSB first S3...S15) */
	#define IRG_BRANCH_MASK 0x1000
	uint16_t irg;

    /* IO bus 16 digit (LSB first) */
	uint8_t io[16];

    /* ALU out for display */
    char display_digit;
    uint8_t dpt:1;
    uint8_t segH:1;

    /* ALU input for key, busy */
    #define KN_BIT  0
    #define KO_BIT  1
    #define KP_BIT  2
    #define KQ_BIT  3
    #define KR_BIT  4
    #define KS_BIT  5
    #define KT_BIT  6
    uint8_t key_line;

    /* D15 to D0 */
    int dstate;

    /* S0 to S15 */
    int sstate;
    /* write = 0 : read bus, write = 1 : write bus */
    int write;
    int idle;

    /* debug */
    int addr;
};
