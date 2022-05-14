#include <stdint.h>
#include <stdio.h>
#include "../common/logic-common.h"
#include "../common/clockline.h"
#include "74ls.h"


/* 74LS00 Series NAND/NOR Gates */

/* 74LS00 Quad 2-Input NAND Gate */
void d_74(ls00){
	S_(Y1)= ~( S_(A1) & S_(B1) ) & 0x1;
	S_(Y2)= ~( S_(A2) & S_(B2) ) & 0x1;
	S_(Y3)= ~( S_(A3) & S_(B3) ) & 0x1;
	S_(Y4)= ~( S_(A4) & S_(B4) ) & 0x1;
}

/* 74LS01 Quad 2-Input NAND Gate With Open Collector Outputs*/
void d_74(ls01){
	OC_OUTPUT(S_(Y1), ~( S_(A1) & S_(B1) ) ) & 0x1;
	OC_OUTPUT(S_(Y2), ~( S_(A2) & S_(B2) ) ) & 0x1;
	OC_OUTPUT(S_(Y3), ~( S_(A3) & S_(B3) ) ) & 0x1;
	OC_OUTPUT(S_(Y4), ~( S_(A4) & S_(B4) ) ) & 0x1;
}

/* 74LS02 Quad 2-Input NOR Gate */
void d_74(ls02){
	S_(Y1)= ~( S_(A1) | S_(B1) ) & 0x1;
	S_(Y2)= ~( S_(A2) | S_(B2) ) & 0x1;
	S_(Y3)= ~( S_(A3) | S_(B3) ) & 0x1;
	S_(Y4)= ~( S_(A4) | S_(B4) ) & 0x1;
}

/* 74LS03 Quad 2-Input NAND Gate with Open Collector Outputs */
/* Alias for 74LS01 */
void dalias_74(ls01,ls03);


/* 74LS04 Series Hex Inverters */

/* 74LS04 Hex Inverter */
void d_74(ls04) {
	S_(Y1)= ~S_(A1) & 0x1;
	S_(Y2)= ~S_(A2) & 0x1;
	S_(Y3)= ~S_(A3) & 0x1;
	S_(Y4)= ~S_(A4) & 0x1;
	S_(Y5)= ~S_(A5) & 0x1;
	S_(Y6)= ~S_(A6) & 0x1;
}

/* 74LS05 Hex Inverter with Open Collector outputs */
void d_74(ls05) {
	OC_OUTPUT(S_(Y1), ~S_(A1) & 0x1 );
	OC_OUTPUT(S_(Y2), ~S_(A2) & 0x1 );
	OC_OUTPUT(S_(Y3), ~S_(A3) & 0x1 );
	OC_OUTPUT(S_(Y4), ~S_(A4) & 0x1 );
	OC_OUTPUT(S_(Y5), ~S_(A5) & 0x1 );
	OC_OUTPUT(S_(Y6), ~S_(A6) & 0x1 );
}

/* 74LS06 Hex Inverter Buffers/Drivers with Open Collector outputs */
void dalias_74(ls05,ls06);

/* 74LS07 Hex Buffers/Drivers (non-inverting) with Open Collector outputs */
void d_74(ls07) {
	OC_OUTPUT(S_(Y1), S_(A1) & 0x1 );
	OC_OUTPUT(S_(Y2), S_(A2) & 0x1 );
	OC_OUTPUT(S_(Y3), S_(A3) & 0x1 );
	OC_OUTPUT(S_(Y4), S_(A4) & 0x1 );
	OC_OUTPUT(S_(Y5), S_(A5) & 0x1 );
	OC_OUTPUT(S_(Y6), S_(A6) & 0x1 );
}

/* 74LS08 Quad 2-Input AND Gate */
void d_74(ls08){
	S_(Y1)=( S_(A1) & S_(B1) ) & 0x1;
	S_(Y2)=( S_(A2) & S_(B2) ) & 0x1;
	S_(Y3)=( S_(A3) & S_(B3) ) & 0x1;
	S_(Y4)=( S_(A4) & S_(B4) ) & 0x1;
}

/* 74LS09 Quad 2-Input AND Gate With Open Collector Outputs*/
void d_74(ls09){
	OC_OUTPUT(S_(Y1), S_(A1) & S_(B1) ) & 0x1;
	OC_OUTPUT(S_(Y2), S_(A2) & S_(B2) ) & 0x1;
	OC_OUTPUT(S_(Y3), S_(A3) & S_(B3) ) & 0x1;
	OC_OUTPUT(S_(Y4), S_(A4) & S_(B4) ) & 0x1;
}


/* 74LS10 Series 3-Input Logic Gates */

/* 74LS10 Triple 3-Input NAND Gate */
void d_74(ls10){
	S_(Y1)= ~( S_(A1) & S_(B1) & S_(C1) ) & 0x1;
	S_(Y2)= ~( S_(A2) & S_(B2) & S_(C2) ) & 0x1;
	S_(Y3)= ~( S_(A3) & S_(B3) & S_(C3) ) & 0x1;
}

/* 74LS10 Triple 3-Input AND Gate */
void d_74(ls11){
	S_(Y1)= ( S_(A1) & S_(B1) & S_(C1) ) & 0x1;
	S_(Y2)= ( S_(A2) & S_(B2) & S_(C2) ) & 0x1;
	S_(Y3)= ( S_(A3) & S_(B3) & S_(C3) ) & 0x1;
}


/* 74LS20 Series 4-Input Logic Gates */


/* 74LS20 Double 4-Input NAND Gate */
void d_74(ls20){
	S_(Y1)= ~( S_(A1) & S_(B1) & S_(C1) & S_(D1) ) & 0x1;
	S_(Y2)= ~( S_(A2) & S_(B2) & S_(C2) & S_(D2) ) & 0x1;
}

/* 74LS20 Double 4-Input AND Gate */
void d_74(ls21){
	S_(Y1)= ( S_(A1) & S_(B1) & S_(C1) & S_(D1) ) & 0x1;
	S_(Y2)= ( S_(A2) & S_(B2) & S_(C2) & S_(D2) ) & 0x1;
}


/* 74LS30 Single 8-Input NAND Gate */
void d_74(ls30){
	S_(Y)= ~( S_(A) & S_(B) & S_(C) & S_(D) & S_(E) & S_(F) & S_(G) & S_(H) ) & 0x1;
}


/* 74LS37 Quad 2-Input NAND Buffer */
/* Alias for 74LS00 */
void dalias_74(ls00,ls37);

/* 74LS38 Quad 2-Input NAND Buffer With Open Collector outputs */
/* Alias for 74LS01 */
void dalias_74(ls01,ls38);

/* 74LS74 Dual Positive-Edge-Triggered D Flip-Flops with Preset, Clear, and Complementary Outputs */
void d_74(ls74) {
	if( S_(PRE1_) && S_(CLR1_) && S_(clk1) == CLK_LH) { I_(q1)= S_(D1) & 0x1 ; I_(q1_)= I_(q1) ^ 0x1; }
	else if ( !S_(PRE1_) && S_(CLR1_) ) { I_(q1)= HI; I_(q1_)= LO; }
	else if ( !S_(CLR1_) && S_(PRE1_) ) { I_(q1)= LO; I_(q1_)= HI; }
	else { I_(q1)= HI; I_(q2)= HI; }
	S_(Q1)= I_(q1); S_(Q1_)= I_(q1_);
	
	if( S_(PRE2_) && S_(CLR2_) && S_(clk2) == CLK_LH) { I_(q2)= S_(D2) & 0x1 ; I_(q2_)= I_(q2) ^ 0x1; }
	else if ( !S_(PRE2_) && S_(CLR2_) ) { I_(q2)= HI; I_(q2_)= LO; }
	else if ( !S_(CLR2_) && S_(PRE2_) ) { I_(q2)= LO; I_(q2_)= HI; }
	else { I_(q2)= HI; I_(q2)= HI; }
	S_(Q2)= I_(q2); S_(Q2_)= I_(q2_);
}

/* 74LS138 Series Decoders/Demultiplexers */

/* 74LS138 3 bit 1-of-8 Line Decoder/Demultiplexer With Three Enable Inputs (2 Active LO, 1 Active HI) */
void d_74(ls138) {
	if( S_(G1) && !( S_(G2A_) | S_(G2B_) ) ) { S_(Y)= 0xff; }
	else { S_(Y)= ( 1<<(S_(CBA) & 0x7) ) ^ 0xff ; }
}

/* 74LS139 Dual 2 bit 1-of-4 Line Decoders/Demultiplexers With Independent Enable Inputs (Active LO) */
void d_74(ls139) {
	S_(Y1)= S_(G1_) ? 0x0f : ( 1<<(S_(BA1) & 0x3) ^ 0x0f );
	S_(Y2)= S_(G2_) ? 0x0f : ( 1<<(S_(BA2) & 0x3) ^ 0x0f );
}

/* 74LS151 Series Data Selectors/Multiplexers */

/* 74LS151 1-of-8 Line Data Selector/Multiplexer With Strobe */
void d_74(ls151) {
	if( S_(G) ) { S_(Y)= HI; S_(Y_)= LO; }
	else { S_(Y)= ( S_(D) & 1<<(S_(CBA) & 0x7) ) & 0x1 ; S_(Y_)= ~S_(Y) & 0x1; }
}

/* 74LS153 Dual 4-to-1 Line Data Selectors/Multiplexors With Common Selects and Independent Enable Inputs (Active LO) */
void d_74(ls153) {
	S_(Y1)= S_(G1_) ? 0 : ( 1<<(S_(BA) & 0x3) & S_(D1) ) ? 1 : 0;
	S_(Y2)= S_(G2_) ? 0 : ( 1<<(S_(BA) & 0x3) & S_(D2) ) ? 1 : 0;
}

/* 74LS157 Series Quad 2-Line to 1-Line Data Selectors/Multiplexors With Common Select and Strobe */
/* 74LS157 Quad 2-Line to 1-Line Data Selectors/Multiplexors With Common Select and Strobe */
void d_74(ls157) {
	S_(Y)=(S_(G_)?0:S_(BA_)?S_(D2):S_(D1)) & 0xf;
}

/* 74LS158 Inverting Quad 2-Line to 1-Line Data Selectors/Multiplexors With Common Select and Strobe */
void d_74(ls158) {
	S_(Y)=~(S_(G_)?0:S_(BA_)?S_(D2):S_(D1)) & 0xf;
}

/* 74LS168 Presettable Synchronous 4-bit BCD Up/Down Binary Counter */
void d_74(ls168) {
	nibble_t next;

	/* Calculate next counter value; Update RCO_ if ENT_=0 */
	if(S_(UD_)) { /* Counting up */
		if( !S_(ENT_) ) { S_(RCO_)=I_(q)&0x9?1:0; }
		switch( I_(q) ){
			case 9: case 0xf: next=0x0; break;
			case 0xb: case 0xd: next=0x4; break;
			default: next=I_(q)+1; break;
		}
	} else { /* Counting down */
		if( !S_(ENT_) ) { S_(RCO_)=I_(q)?0:1; }
		switch( I_(q) ) {
			case 0: next=0x9; break;
			case 0xa: next=0x1; break;
			case 0xc: next=0x3; break;
			case 0xe: next=0x5; break;
			default: next=I_(q)-1; break;
		}
	}

	/* Latch data on rising edge of clock */
	if( S_(clk) == CLK_LH ) {
		/* Load data from D if LD_=0 */
		if( !S_(LD_) ) { I_(q)=S_(D); }
		/* Otherwise, latch counter result if ENP_=0 & ENT_0 */
		else if( !S_(ENP_) && !S_(ENT_) ) { I_(q)=next; }
	}

	/* Update output state to reflect internal state of flip-flops */
	S_(Q)=I_(q);
}

/* 74LS169 Presettable Synchronous 4-bit Modulo-16 Up/Down Binary Counter */
void d_74(ls169) {
	nibble_t next;

	/* Calculate next counter value; Update RCO_ if ENT_=0 */
	if(S_(UD_)) { /* Counting up */
		if( !S_(ENT_) ) { S_(RCO_)=I_(q)&0xf?1:0; }
		next=I_(q)==0xf?0:I_(q)+1;
	} else { /* Counting down */
		if( !S_(ENT_) ) { S_(RCO_)=I_(q)?0:1; }
		next=I_(q)==0x0?0xf:I_(q)-1;
	}

	/* Latch data on rising edge of clock */
	if( S_(clk) == CLK_LH ) {
		/* Load data from D if LD_=0 */
		if( !S_(LD_) ) { I_(q)=S_(D); }
		/* Otherwise, latch counter result if ENP_=0 & ENT_0 */
		else if( !S_(ENP_) && !S_(ENT_) ) { I_(q)=next; }
	}

	/* Update output state to reflect internal state of flip-flops */
	S_(Q)=I_(q);
}


/* 74LS173 4-Bit D-Type Registers With 3-State Outputs */
/* Latch data from D on rising edge if both G1_ and G2_ are HI, outputs HiZ unless OC1_ & OC2_ =0 (Active LO) */
void d_74(ls173) {
	if( S_(CLR) ) { I_(q)= 0x0; }
	else if( S_(G1) && S_(G2) && S_(clk) == CLK_LH ) { I_(q)= S_(D) & 0xf; }
	TRI_OUTPUT(S_(Q), (!S_(OE1_) && !S_(OE2_)), I_(q));
} 

/* 74LS174 Hex D-Type Flip-Flop With Clear */
/* Latch data from D on rising edge, Clear when CLR_ is LO */
void d_74(ls174) {
	if( S_(CLR_) ) { I_(q)= 0x0; }
	else if( S_(clk) == CLK_LH ) { I_(q)= S_(D) & 0x3f; }
	S_(Q)= I_(q);
} 

/* 74LS175 Quad D-Type Flip-Flop With Clear and Complemented Outputs */
/* Latch data from D on rising edge, Clear when CLR_ is LO */
void d_74(ls175) {
	if( S_(CLR_) ) { I_(q)= 0x0; }
	else if( S_(clk) == CLK_LH ) { I_(q)= S_(D) & 0xf; }
	S_(Q)= I_(q); S_(Q_)= ~I_(q) & 0xf;
} 

/* 74LS240 Series Octal 3-STATE Buffers/Line Drivers/Line Receivers */

/* 74LS240 Octal (Dual 4-Bit) 3-STATE Buffer/Line Driver/Line Receiver */
/* Enable inverting outputs: G1=0->Y1 (Active LO), G2=0->Y2 (Active LO) */
void d_74(ls240) {
	if( !S_(G1_) ) { S_(Y1)= ~S_(A1) & 0xf; }
	if( !S_(G2_) ) { S_(Y2)= ~S_(A2) & 0xf; }
}


/* 74LS241 Octal (Dual 4-Bit) 3-STATE Buffer/Line Driver/Line Receiver */
/* Enable non-inverting outputs: G1=0->Y1 (Active LO), G2=1->Y2 (Active HI) */
void d_74(ls241) {
	if( !S_(G1_) ) { S_(Y1)= S_(A1) & 0xf; }
	if(  S_(G2)  ) { S_(Y2)= S_(A2) & 0xf; }
}

/* 74LS253 Dual 4-to-1 Line Data Selectors/Multiplexors With Common Selects, Independent Enables (Active LO), and 3-State Outputs */
void d_74(ls253) {
	TRI_OUTPUT(S_(Y1), S_(G1_), ( 1<<(S_(BA) & 0x3) & S_(D1) ) ? 1 : 0 );
	TRI_OUTPUT(S_(Y2), S_(G2_), ( 1<<(S_(BA) & 0x3) & S_(D2) ) ? 1 : 0 );
}






/* 74LS259 8-Bit Addressable Latch */
void d_74(ls259) {
	if( !S_(CLR_) && S_(G_) ) { I_(q)=0x0; } /* Clear Mode, clear all bits regardless of inputs or address */
	else if( !S_(CLR_) ) { I_(q)= 1<<(S_(CBA)&0x7); } /* Demux mode, set addressed bit HI, all others LO */
	/* Addressable Latch Mode, set addressed bit in latch to input from D */
	else if( !S_(G_) ) { I_(q)= ( I_(q) & ~( 1<<(S_(CBA)&0x7) ) ) | ( (S_(D)&0x1)<<(S_(CBA)&0x7) ); }
	else {;} /* Memory Mode, hold value in Q */
	S_(Q)=I_(q);
}


/* 74LS365 Series Hex Bus Drivers With 3-State Outputs */

/* 74LS365 Hex Bus Drivers With 3-State Outputs */
void d_74(ls365) { TRI_OUTPUT(S_(Y), !S_(G1_) && !S_(G2_), S_(Y) & 0x3f); }

/* 74LS365 Hex Bus Drivers With Inverting 3-State Outputs */
void d_74(ls366) { TRI_OUTPUT(S_(Y), !S_(G1_) && !S_(G2_), ~S_(Y) & 0x3f); }

/* 74LS367 Series Hex (4-Bit and 2-Bit) Bus Drivers With 3-State Outputs */

/* 74LS367 Hex (4-Bit and 2-Bit) Bus Drivers With 3-State Outputs */
void d_74(ls367) {
	TRI_OUTPUT(S_(Y1), !S_(G1_), S_(Y1) & 0xf);
	TRI_OUTPUT(S_(Y2), !S_(G2_), S_(Y2) & 0x3);
}

/* 74LS368 Hex (4-Bit and 2-Bit) Bus Drivers With 3-State Outputs */
void d_74(ls368) {
	TRI_OUTPUT(S_(Y1), !S_(G1_), ~S_(Y1) & 0xf);
	TRI_OUTPUT(S_(Y2), !S_(G2_), ~S_(Y2) & 0x3);
}



/* 74LS373 Series Octal Transparent Latches / D-Type Flip-Flops With Output Enable */

/* 74LS373 Octal Transparent Latchs With Output Control */
/* Latch data from D when G=1 (Active HI), outputs HiZ unless OE_=0 (Active LO) */
void d_74(ls373) {
	if( S_(G) ) { I_(q)= S_(D); }
	TRI_OUTPUT(S_(Q),!S_(OE_),I_(q));
} 



/* 74LS374 Octal D-Type Flip-Flops With Output Control */
/* Latch data from D on rising edge, outputs HiZ unless OE_=0 (Active LO) */
void d_74(ls374) {
	if( S_(clk) == CLK_LH ) { I_(q)= S_(D); }
	TRI_OUTPUT(S_(Q),!S_(OE_),I_(q));
} 



/* 74LS377 Series Octal/Hex/Quad D-Type Flip-Flops With Enable */

/* 74LS377 Octal D-Type Flip-Flops With Enable */
/* Latch data from D to Q on rising edge if G_=0 (Active LO) */
void d_74(ls377) {
	if( S_(clk) == CLK_LH && !S_(G_) ) { I_(q)= S_(D); }
	S_(Q)=I_(q);
} 

/* 74LS378 Hex D-Type Flip-Flops With Enable And Single-Rail Outputs*/
/* Latch data from D to Q on rising edge if G_=0 (Active LO) */
void d_74(ls378) {
	if( S_(clk) == CLK_LH && !S_(G_) ) { I_(q) = S_(D) & 0x3f; }
	S_(Q)=I_(q);
} 

/* 74LS379 Quad D-Type Flip Flops With Enable and Double-Rail Outputs */
/* Latch data from D to Q and Q_(inverted) on rising edge if G_=0 (Active LO) */
void d_74(ls379) {
	if( S_(clk) == CLK_LH && !S_(G_) )  {I_(q)= S_(D) & 0x0f; }
	S_(Q) = I_(q);
	S_(Q_) = ~I_(q) & 0xf;
}


int main_74ls() {
	bit_t a1,a2,a3,a4,b1,b2,b3,b4,y1,y2,y3,y4;
	dt_74(ls03) ls03_1 = { &a1,&b1,&y1,&a2,&b2,&y2,&a3,&b3,&y3,&a4,&b4,&y4};
	a1=1; b1=1; y1=1; a2=LO; b2=LO; y2=HI;
	_74(ls03)(&ls03_1);
	printf("%u & %u -> %u\n", a1,b1,y1);
	printf("%u & %u -> %u\n", a2,b2,y2);
	return(0);
}
