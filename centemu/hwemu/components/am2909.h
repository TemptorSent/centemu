#pragma once
#include <stdint.h>
#include "../common/clockline.h"
#include "../common/logic-common.h"

enum am2909_source_code { uPC=0x0, AR=0x1, STK0=0x2, Di=0x3 };
static char *am2901_source_mnemonics[4] = { "uPC", "AR", "STK0", "Di" };

enum am2909_stack_control_code { POP=0x0, PUSH=0x1, HOLD=0x2, HOLD2=0x3 };
static char *am2909_stact_control_mnemonics[4] = {"POP","PUSH","HOLD","HOLD2"};

static char *am2909_ops[16][5] ={
	{"uPC+1", "Pop", "uPC", "Pop Stack", "End Loop"},
	{"uPC+1", "Push", "uPC", "Push uPC", "Setup Loop"},
	{"uPC+1", "Hold", "uPC", "Continue", "Continue"},
	{"uPC+1", "Hold", "uPC", "Continue", "Continue"},

	{"AR+1", "Pop", "AR", "Jump to Address in AR; Pop Stack", "End Loop"},
	{"AR+1", "Push", "AR", "Push uPC; Jump to Address in AR", "JSR AR"},
	{"AR+1", "Hold", "AR", "Jump to Address in AR", "JMP AR"},
	{"AR+1", "Hold", "AR", "Jump to Address in AR", "JMP AR"},

	{"STK0+1", "Pop", "STK0", "Jump to Address in STK0; Pop Stack", "RTS"},
	{"STK0+1", "Push", "STK0", "Push uPC; Jump to Address in STK0; Push uPC", "JSR STK0"},
	{"STK0+1", "Hold", "STK0", "Jump to Address in STK0", "JMP STK0"},
	{"STK0+1", "Hold", "STK0", "Jump to Address in STK0", "JMP STK0"},
	
	{"D+1", "Pop", "D", "Jump to Address in D; Pop Stack", "End Loop"},
	{"D+1", "Push", "D", "Push uPC; Jump to Address in D", "JSR D"},
	{"D+1", "Hold", "D", "Jump to Address in D", "JMP D"},
	{"D+1", "Hold", "D", "Jump to Address in D", "JMP D"},
};	

/* am2909 device */
typedef struct am2909_device_t {
	char *id;
	clock_state_t *clk;
	twobit_t *S; /* Source (S1,S0) select */
	bit_t *FE_; /* File Enable (Active LO)  - Stack HOLD when high, Push/Pop occur when low */
	bit_t *PUP; /* When enabled by FE_, increment SP and Push when high, Pop and decrement SP when low */

	bit_t *Cn; /* Carry-in to incrementer */

	nibble_t *Di; /* Di Direct inputs to MUX */

	bit_t *RE_; /* Enable line for internal address register */
	nibble_t *Ri; /* Ri Inputs to internal address register */

	nibble_t uPC; /* Microprogram counter */
	nibble_t AR; /* Address/holding register */
	nibble_t STK[4]; /* Stack (circular buffer) */
	twobit_t SP; /* Stack pointer */


	bit_t *Co; /* Carry-out from incrementer */

	nibble_t *Y; /* Output value */
	nibble_t *ORi; /* ORi output controls (set Yi high if outputs enabled and not zeroed) */
	bit_t *ZERO_; /* Force all Yi output bits to 0 if outputs enabled when low (Active LO)*/
	bit_t *OE_; /* Enable Yi outputs when low (Active LO), High-Z when high */


} am2909_device_t;

int am2909_init(am2909_device_t *dev, char *id,
	clock_state_t *clk, /* Clock state from clockline */
	twobit_t *S, bit_t *FE_, bit_t *PUP, /* Select operation */
	nibble_t *Di, /* Direct inputs */
	nibble_t *Ri, bit_t *RE_, /* AR inputs (tied to Di on am2911) (when RE_ is LO) inputs */
	bit_t *Cn, bit_t *Co, /* Incrementer carry in and carry out, Cn=1 increments uPC, Cn=0 repeats current op */
	nibble_t *ORi, bit_t *ZERO_, bit_t *OE_, /* Outputs overrides: OE_=1->HiZ, ZERO_=0->Y=0, ORi=1->Yi=1(2909 only) */
	nibble_t *Y /* Outputs Yi of Y if not overridden by above */);
void am2909_update(am2909_device_t *dev);

/* Pin reduced am2911 is the same as am2909 except Ri tied to Di internally and no ORi inputs */
typedef am2909_device_t am2911_device_t;

int am2911_init(am2911_device_t *dev, char* id,
	clock_state_t *clk, /* Clock state from clockline */
	twobit_t *S, bit_t *FE_, bit_t *PUP, /* Select operation */
	nibble_t *Di, bit_t *RE_, /* Direct and AR (when RE_ is LO) inputs */
	bit_t *Cn, bit_t *Co, /* Incrementer carry in and carry out, Cn=1 increments uPC, Cn=0 repeats current op */
	bit_t *ZERO_, bit_t *OE_, /* Outputs overrides: OE_=1->HiZ, ZERO_=0->Y=0, ORi=1->Yi=1 */
	nibble_t *Y /* Outputs Yi of Y if not overridden by above */);

void am2911_update(am2911_device_t *dev);

