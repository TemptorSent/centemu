#pragma once
#include <stdint.h>
#include <stdio.h>
#include "../common/logic-common.h"
#include "../common/clockline.h"

#define _74(part) logic_74 ## part
#define d_74(part) logic_74 ## part (logic_74 ## part ## _device_t *dev)
#define dt_74(part) logic_74 ## part ## _device_t
#define dstruct_74(part) struct logic_74 ## part ## _device_t
#define dtd_74(part,alias) typedef struct logic_74 ## part ## _device_t logic_74 ## alias ## _device_t
//#define dalias_74(part,alias) (*logic_74 ## alias)(dt_74(alias) *dev)=_74(part);
#define dalias_74(part,alias) logic_74 ## alias( dt_74(alias) *dev ) { _74(part)(dev); }

/* 74LS00 Series NAND/NOR Gates */

dstruct_74(ls00) {
	bit_t *A1, *B1, *Y1; /* Gate 1 Inputs A1 & B1 and Output Y1 */
	bit_t *A2, *B2, *Y2; /* Gate 2 Inputs A2 & B2 and Output Y2 */
	bit_t *A3, *B3, *Y3; /* Gate 3 Inputs A3 & B3 and Output Y3 */
	bit_t *A4, *B4, *Y4; /* Gate 4 Inputs A4 & B4 and Output Y4 */
};

/* 74LS00 Quad 2-Input NAND Gate */
dtd_74(ls00,ls00);
void d_74(ls00);

/* 74LS01 Quad 2-Input NAND Gate With Open Collector Outputs*/
dtd_74(ls00,ls01);
void d_74(ls01);

/* 74LS02 Quad 2-Input NOR Gate */
dtd_74(ls00,ls02);
void d_74(ls02);

/* 74LS03 Quad 2-Input NAND Gate with Open Collector Outputs */
/* Alias for 74LS01 */
dtd_74(ls00,ls03);
//void dalias_74(ls01,ls03);
void d_74(ls03);


/* 74LS04 Series Hex Inverters */

/* 74LS04 Hex Inverter */
dstruct_74(ls04) {
	bit_t *A1,*Y1; /* Gate 1 input and inverted output */
	bit_t *A2,*Y2; /* Gate 2 input and inverted output */
	bit_t *A3,*Y3; /* Gate 3 input and inverted output */
	bit_t *A4,*Y4; /* Gate 4 input and inverted output */
	bit_t *A5,*Y5; /* Gate 5 input and inverted output */
	bit_t *A6,*Y6; /* Gate 6 input and inverted output */
};
dtd_74(ls04,ls04);
void d_74(ls04);

/* 74LS05 Hex Inverter with Open Collector outputs */
dtd_74(ls04,ls05);
void d_74(ls05);

/* 74LS06 Hex Inverter Buffers/Drivers with Open Collector outputs */
dtd_74(ls04,ls06);
void d_74(ls06);

/* 74LS07 Hex Buffers/Drivers (non-inverting) with Open Collector outputs */
dtd_74(ls04,ls07);
void d_74(ls07);

/* 74LS08 Quad 2-Input AND Gate */
dtd_74(ls00,ls08);
void d_74(ls08);

/* 74LS09 Quad 2-Input AND Gate With Open Collector Outputs*/
dtd_74(ls00,ls09);
void d_74(ls09);


/* 74LS10 Series 3-Input Logic Gates */

dstruct_74(ls10) {
	bit_t *A1, *B1, *C1, *Y1; /* Gate 1 Inputs A,B,C and Y Output */
	bit_t *A2, *B2, *C2, *Y2; /* Gate 2 Inputs A,B,C and Y Output */
	bit_t *A3, *B3, *C3, *Y3; /* Gate 3 Inputs A,B,C and Y Output */
};

/* 74LS10 Triple 3-Input NAND Gate */
dtd_74(ls10,ls10);
void d_74(ls10);

/* 74LS10 Triple 3-Input AND Gate */
dtd_74(ls10,ls11);
void d_74(ls11);


/* 74LS20 Series 4-Input Logic Gates */

dstruct_74(ls20) {
	bit_t *A1, *B1, *C1, *D1, *Y1; /* Gate 1 Inputs A,B,C,D and Y Output */
	bit_t *A2, *B2, *C2, *D2, *Y2; /* Gate 2 Inputs A,B,C,D and Y Output */
};

/* 74LS20 Double 4-Input NAND Gate */
dtd_74(ls20,ls20);
void d_74(ls20);

/* 74LS20 Double 4-Input AND Gate */
dtd_74(ls20,ls21);
void d_74(ls21);


/* 74LS30 Single 8-Input NAND Gate */
dstruct_74(ls30) {
	bit_t *A, *B, *C, *D, *E, *F, *G, *H, *Y; /* Inputs A-H and Y Output */
};

dtd_74(ls30,ls30);
void d_74(ls30);


/* 74LS37 Quad 2-Input NAND Buffer */
/* Alias for 74LS00 */
dtd_74(ls00,ls37);
void d_74(ls37);

/* 74LS38 Quad 2-Input NAND Buffer With Open Collector outputs */
/* Alias for 74LS01 */
dtd_74(ls00,ls38);
void d_74(ls38);

/* 74LS74 Dual Positive-Edge-Triggered D Flip-Flops with Preset, Clear, and Complementary Outputs */
dstruct_74(ls74) {
	clock_state_t *clk1; /* Flip-Flop 1 Clock state */
	bit_t *PRE1_, *CLR1_, *D1; /* Flip-Flop 1 Preset, Clear, and Data inputs*/
	bit_t *Q1, *Q1_; /* Flip-Flop 1 Outputs, non-inverting and inverting (complementary) */

	clock_state_t *clk2; /* Flip-Flop 2 Clock state */
	bit_t *PRE2_, *CLR2_, *D2; /* Flip-Flop 2 Preset, Clear, and Data inputs*/
	bit_t *Q2, *Q2_; /* Flip-Flop 2 Outputs, non-inverting and inverting (complementary) */
	bit_t q1,q1_, q2,q2_; /* Internal state of Flip-Flops 1 & 2 */
};
dtd_74(ls74,ls74);
void d_74(ls74);

/* 74LS138 Series Decoders/Demultiplexers */

/* 74LS138 3 bit 1-of-8 Line Decoder/Demultiplexer With Three Enable Inputs (2 Active LO, 1 Active HI) */
dstruct_74(ls138) {
	uint8_t *CBA; /* Select lines in bit order */
	uint8_t *G1,*G2A_,*G2B_; /* ANDed Enable lines, G1 Active HI, G2A_,G2B_ Active LO */
	uint8_t *Y; /* Outputs (inverting) */
};
dtd_74(ls138,ls138);
void d_74(ls138);

/* 74LS139 Dual 2 bit 1-of-4 Line Decoders/Demultiplexers With Independent Enable Inputs (Active LO) */
dstruct_74(ls139) {
	uint8_t *BA1; /* Gate 1 select lines */
	bit_t *G1_; /* Gate 1 enable input (Active LO) */
	uint8_t *Y1; /* Gate 1 outputs */

	uint8_t *BA2; /* Gate 2 select lines */
	bit_t *G2_; /* Gate 2 enable input (Active LO) */
	uint8_t *Y2; /* Gate 2 outputs */
};
dtd_74(ls139,ls139);
void d_74(ls139);

/* 74LS151 Series Data Selectors/Multiplexers */

/* 74LS151 1-of-8 Line Data Selector/Multiplexer With Strobe */
dstruct_74(ls151) {
	uint8_t *CBA; /* Select lines in bit order */
	bit_t *G; /* Strobe - G=1 to force W=HI and Y=LO */
	uint8_t *D; /* Data input */
	bit_t *Y, *Y_; /* Non-inverting and inverting outputs */
};
dtd_74(ls151,ls151);
void d_74(ls151);

/* 74LS153 Dual 4-to-1 Line Data Selectors/Multiplexors With Common Selects and Independent Enable Inputs (Active LO) */
dstruct_74(ls153) {
	uint8_t *BA; /* Common select lines */
	bit_t *G1_; /* Gate 1 enable input (Active LO) */
	uint8_t *D1; /* Gate 1 4-bit data input */
	bit_t *Y1; /* Gate 1 output */

	bit_t *G2_; /* Gate 2 enable input (Active LO) */
	uint8_t *D2; /* Gate 2 4-bit data input */
	bit_t *Y2; /* Gate 2 output */
};
dtd_74(ls153,ls153);
void d_74(ls153);

/* 74LS157 Series Quad 2-Line to 1-Line Data Selectors/Multiplexors With Common Select and Strobe */
dstruct_74(ls157) {
	bit_t *BA_, *G_; /* Input select A=LO, B=HI and Enable/Strobe (Active LO) */
	uint8_t *D1, *D2; /* Data input 4-bit words */
	uint8_t *Y; /* Data output 4-bit word */
};
/* 74LS157 Quad 2-Line to 1-Line Data Selectors/Multiplexors With Common Select and Strobe */
dtd_74(ls157,ls157);
void d_74(ls157);


/* 74LS158 Inverting Quad 2-Line to 1-Line Data Selectors/Multiplexors With Common Select and Strobe */
dtd_74(ls157,ls158);
void d_74(ls158);


/* 74LS168 Series Presettable Synchronous 4-bit Up/Down Binary Counters */
dstruct_74(ls168) {
	clock_state_t *clk;
	nibble_t *D; /* Preset Data Input, read when LD_=0 */
	nibble_t *Q; /* Data Output, latched on rising edge of clk */
	bit_t *UD_; /* Direction Up=HI, Down=LO */
	bit_t *LD_; /* (Active LO) Load Data; inhibit counting */
	bit_t *ENP_; /* (Active LO) Count Enable Parallel */
	bit_t *ENT_; /* (Active LO) Count Enable Trickle (RCO_ Output Enable) */
	bit_t *RCO_; /* Ripple Carry Output, usually chained to ENT_ input of next higher counter */
	nibble_t q; /* Internal state of Output flip-flops */
};
dtd_74(ls168,ls168);

/* 74LS168 Series Presettable Synchronous 4-bit BCD Up/Down Binary Counter */
void d_74(ls168);

/* 74LS168 Series Presettable Synchronous 4-bit Modulo-16 Up/Down Binary Counter */
dtd_74(ls168,ls169);
void d_74(ls169);



/* 74LS173 4-Bit D-Type Registers With 3-State Outputs */
/* Latch data from D on rising edge if both G1_ and G2_ are HI, outputs HiZ unless OC1_ & OC2_ =0 (Active LO) */
dstruct_74(ls173) {
	clock_state_t *clk; /* Clock state */
	bit_t *G1, *G2; /* Latch input enables, enabled when both Active HI */
	bit_t *CLR; /* Master reset, when HI, force Q to LO */
	uint8_t *D; /* Data word IN */
	uint8_t *Q; /* Data word OUT */
	bit_t *OE1_, *OE2_; /* Output controls, enabled when both Active LO */
	uint8_t q; /* Internal state of register */
};
dtd_74(ls173,ls173);
void d_74(ls173);

/* 74LS174 Hex D-Type Flip-Flop With Clear */
/* Latch data from D on rising edge, Clear when CLR_ is LO */
dstruct_74(ls174) {
	clock_state_t *clk; /* Clock state */
	bit_t *CLR_; /* Master reset (Active LO), When LO, force Q to LO */
	uint8_t *D; /* Data word IN */
	uint8_t *Q, *Q_; /* Data word (Q) and complement (Q_) OUT */
	uint8_t q; /* Internal state of register */
};
dtd_74(ls174,ls174);
void d_74(ls174);

/* 74LS175 Quad D-Type Flip-Flop With Clear and Complemented Outputs */
/* Latch data from D on rising edge, Clear when CLR_ is LO */
dtd_74(ls174,ls175);
void d_74(ls175);

/* 74LS240 Series Octal 3-STATE Buffers/Line Drivers/Line Receivers */

/* 74LS240 Octal (Dual 4-Bit) 3-STATE Buffer/Line Driver/Line Receiver */
/* Enable inverting outputs: G1=0->Y1 (Active LO), G2=0->Y2 (Active LO) */
dstruct_74(ls240) {
	bit_t *G1_; /* Gate 1 enable input (Active LO) */
	uint8_t *A1, *Y1; /* Gate 1 input and inverting output */

	bit_t *G2_; /* Gate 2 enable input (Active LO) */
	uint8_t *A2, *Y2; /* Gate 2 input and inverting output */
};
dtd_74(ls240,ls240);
void d_74(ls240);


/* 74LS241 Octal (Dual 4-Bit) 3-STATE Buffer/Line Driver/Line Receiver */
/* Enable non-inverting outputs: G1=0->Y1 (Active LO), G2=1->Y2 (Active HI) */
dstruct_74(ls241) {
	bit_t *G1_; /* Gate 1 enable input (Active LO) */
	uint8_t *A1, *Y1; /* Gate 1 input and inverting output */

	bit_t *G2; /* Gate 2 enable input (Active HI) */
	uint8_t *A2, *Y2; /* Gate 2 input and inverting output */
};
dtd_74(ls241,ls241);
void d_74(ls241);


/* 74LS253 Dual 4-to-1 Line Data Selectors/Multiplexors With Common Selects, Independent Enables (Active LO), and 3-State Outputs */
dtd_74(ls153,ls253);
void d_74(ls253);






/* 74LS259 8-Bit Addressable Latch */
dstruct_74(ls259) {
	uint8_t *CBA; /* Select lines in bit order */
	bit_t *G_; /* Enable Input, Active LO */
	bit_t *CLR_; /* Clear (Active LO), When LO, if G_=1, force all Qi to LO, if G_=0, for non-selected Qi to LO */
	bit_t *D; /* Data input to addressed latch */
	uint8_t *Q; /* Output byte */
	uint8_t q; /* Internal state */
};
dtd_74(ls259,ls259);
void d_74(ls259);


/* 74LS365 Series Hex Bus Drivers With 3-State Outputs */
dstruct_74(ls365) {
	bit_t *G1_, *G2_; /* Enable inputs, Active LO */
	uint8_t *A,*Y; /* Inputs and Tristate Outputs */
};

/* 74LS365 Hex Bus Drivers With 3-State Outputs */
dtd_74(ls365,ls365);
void d_74(ls365);

/* 74LS365 Hex Bus Drivers With Inverting 3-State Outputs */
dtd_74(ls365,ls366);
void d_74(ls366);

/* 74LS367 Series Hex (4-Bit and 2-Bit) Bus Drivers With 3-State Outputs */
dstruct_74(ls367) {
	bit_t *G1_; /* Gate 1 Enable inputs, Active LO */
	uint8_t *A1,*Y1; /* Gate 1 Inputs and Tristate Outputs */

	bit_t *G2_; /* Gate 2 Enable inputs, Active LO */
	uint8_t *A2,*Y2; /* Gate 2 Inputs and Tristate Outputs */
};

/* 74LS367 Hex (4-Bit and 2-Bit) Bus Drivers With 3-State Outputs */
dtd_74(ls367,ls367);
void d_74(ls367);

/* 74LS368 Hex (4-Bit and 2-Bit) Bus Drivers With 3-State Outputs */
dtd_74(ls367,ls368);
void d_74(ls368);



/* 74LS373 Series Octal Transparent Latches / D-Type Flip-Flops With Output Enable */

/* 74LS373 Octal Transparent Latchs With Output Control */
/* Latch data from D when G=1 (Active HI), outputs HiZ unless OE_=0 (Active LO) */
dstruct_74(ls373) {
	bit_t *G; /* Latch enable, Active HI */
	uint8_t *D; /* Data word IN */
	uint8_t *Q; /* Data word OUT */
	bit_t *OE_; /* Output control, Active LO */
	uint8_t q; /* Internal state of Flip-Flop */
};
dtd_74(ls373,ls373);
void d_74(ls373);



/* 74LS374 Octal D-Type Flip-Flops With Output Control */
/* Latch data from D on rising edge, outputs HiZ unless OE_=0 (Active LO) */
dstruct_74(ls374) {
	clock_state_t *clk; /* Clock state */
	uint8_t *D; /* Data word IN */
	uint8_t *Q; /* Data word OUT */
	bit_t *OE_; /* Output control, Active LO */
	uint8_t q; /* Internal state of Flip-Flop */
};
dtd_74(ls374,ls374);
void d_74(ls374);




/* 74LS377 Series Octal/Hex/Quad D-Type Flip-Flops With Enable */

/* 74LS377 Octal D-Type Flip-Flops With Enable */
/* Latch data from D to Q on rising edge if G_=0 (Active LO) */
dstruct_74(ls377) {
	clock_state_t *clk; /* Clock state */
	bit_t *G_; /* Enable, Active LO */
	uint8_t *D; /* Data word IN */
	uint8_t *Q; /* Data word OUT */
	uint8_t *Q_; /* Data word OUT (inverted) ('ls379 only)*/
	uint8_t q; /* Internal state of Flip-Flop */
};
dtd_74(ls377,ls377);
void d_74(ls377);

/* 74LS378 Hex D-Type Flip-Flops With Enable And Single-Rail Outputs*/
/* Latch data from D to Q on rising edge if G_=0 (Active LO) */
dtd_74(ls377,ls378);
void d_74(ls378);

/* 74LS379 Quad D-Type Flip Flops With Enable and Double-Rail Outputs */
/* Latch data from D to Q and Q_(inverted) on rising edge if G_=0 (Active LO) */
dtd_74(ls377,ls379);
void d_74(ls379);

