#include <stdint.h>
#include "logic-common.h"
#include "ginsumatic.h"
#include "am2901.h"
#include "am2909.h"
#include "prom76161.h"
#include "prom6309.h"
#include "74ls.h"
#include "bus-xcvr-misc.h"
#include "ram-misc.h"

struct cpu6_signals {
	/* Signals for machine state */
	struct state {
		bit_t INT_; /* Interrupt? */
	} state;

	/* Bus signals on the card */
	struct bus {
		bit_t RESET_; /* (Active LO) Bus RESET */
	} bus;

	/* Microcode ROMs */
	struct uROM {
		twelvebit_t uA; /* uA0-uA10 Microcode ROM Address lines */
		byte_t uD[7]; /* uD0-uD55 Microcode ROM Data lines */
		bit_t CE1_, CE2, CE3; /* Chip enable lines, enabled when CE1_=0, CE2=1, CE3=1 */
	} uROM;

	/* Decode logic for output from UE3 (ROM6) of instruction word uROM (logical bits 0-8) */
	struct uIW6D {
		bitsalad_bag_t DinSalad[3];
		octal_t D210; /* Address to DeMUX on UD3 (3-8) and UD2A (2-4) */
		bit_t DeMUXSel; /* (D3) Select between DeMUX UD3 (HI) and UD2A (LO) */
		nibble_t D7654; /* High nibble to latch in UE5 */
		nibble_t uIW6D0O; /* Eye-test! Output of UD2A */
		sixbit_t uIW6D1O; /* Output of UD3 */
	} uIW6D;

	/* Microcode Instruction Word Register */
	struct uIWR {
		/* Latches 6 bytes from ROM0-ROM5 with bitsalad=0x76234510 on incoming data lines */
		bitsalad_bag_t uDinSalad[6];
		byte_t uD[6];
		byte_t uQ[6];
		/* Latches 3 hex-words (only 14 bits used?) from UE3 (ROM6) and decoders UD2A and UD3 */
		bitsalad_bag_t uD6inSalad[3];
		sixbit_t uD6[3];
		sixbit_t uQ6[3];
		/* Bitblender to recombine decoded uIW6 into a word and from there split back to bytes */
		bitblender_t uIWQ6_toword_blender;
		word_t uQ6_cword;
		byte_t uQ6_cbyte[2];
		/* A mouthful of bytes containing the full decoded instruction word */
		mouthful_t uIWR_cmouthful;

		/* Signals with glue logic needing bitmangling */
		bit_t uIWSeqsFE; /* UL5.Q3 */
		bit_t uIWSeqsPUP; /* UL5.Q4 */
		bit_t uIWSeq0S0; /* UL5.Q5 -> A.UL6A.Y -> Seq0.S0 */
		bit_t uIWSeq0S1; /* UL5.Q1 -> A.UL6D.Y -> Seq0.S1 */
		bit_t uIWSeqs12S0; /* UL5.Q0 -> B.UK6A.Y -> Seq1.S0 / Seq2.S0 */
		bit_t uIWSeqs12S1; /* UH5.Q7 -> (A.UK6B.Y -> Seq2.S1) / (A.UJ6C.Y -> B.UK6D.Y -> B.UK6C.Y -> Seq1.S1) */
		bit_t uIWSeq1S1EN; /* UK5.Q1 - Enable control of Seq1.S1 by UH5.Q7, otherwise HI to NAND UK6C */

		

	} uIWR;

	/* 12 bit Sequencer made with 2 AM2909 and 1 AM2911 4-bitslice sequencers */
	struct Seqs {
		/* Sequencer Glue */

		/* UH5.Q7 -> A.UJ6C.Y -> B.UK6D.Y -> B.UK6C.Y -> Seq1.S1 */
		/* (UK5.Q1) (UH5.Q7 -> INV) NAND*/
		/*       0        0      1     1
		 *       0        1      0     1
		 *       1        0      1     0
		 *       1        1      0     1
		 * ~UK5.Q1 | UH5.Q7 */    

		bit_t GlueNAND0AY, GlueNAND0BY, GlueNAND0CY, GlueNAND0DY;
		bit_t GlueNAND1AY, GlueNAND1BY, GlueNAND1CY, GlueNAND1DY;
		bit_t GlueINV0AY, GlueINV0BY, GlueINV0CY, GlueINV0DY;



		/* Sequencer inputs and outputs */
		twobit_t S[2]; /* Source select (2 bits)*/
		bit_t FE_; /* (Active LO) File enable, HI=Stack HOLD, LO=Push/Pop enabled */
		bit_t PUP; /* Push/Pop select, HI=Push, LO=Pop */
		bit_t Cin,C4,C8,Cout; /* Incrementer Carry-in, Carry-low, Carry-high, Carry-out */
		nibble_t Di[3]; /* Direct Di inputs to MUX */
		bit_t RE_; /* (Active LO) Register Input Enable */
		nibble_t Ri[2]; /* Register direct inputs (am2911 internally ties to Di) */
		nibble_t Y[3]; /* Outputs, when enabled by OE_=0, HiZ when OE_=1 */
		nibble_t ORi[2]; /* Force set bits high (N/A on am2911) */
		bit_t ZERO_[2]; /* (Active LO) Zero all output bits */
		bit_t OE_; /* (Active LO) Enable Y outputs when OE_=0, Tristate when OE_=1 */
	} Seqs;

	/* 8 bit ALU made with 2x4-bit slice AM2901s */
	struct ALUs {
		bitsalad_bag_t I876Salad, I543Salad, I210Salad; /* Salads for instruction word */
		bitsalad_bag_t ADDR_BSalad; /* B will take Salad */
		bitblender_t ADDR_AShake; /* A wants a Shake */
		octal_t I876, I543, I210; /* ALU instruction word made of three octal pieces */
		nibble_t ADDR_A, ADDR_B; /* Internal register selection */
		nibble_t D[2]; /* External data input */
		bit_t Cin,Chalf,Cout; /* Carry-in, Half-carry, Carry-out */
		bit_t Q0[2],Q3[2]; /* Q shifer carry lines */
		bit_t RAM0[2],RAM3[2]; /* RAM shifer carry lines */
		bit_t P_[2], G_[2]; /* (Active LO) Carry lookahead flags */
		bit_t OVR[2]; /* Overflow flags */
		bit_t FZ[2]; /* Zero flags */
		bit_t F3[2]; /* High bit flags, Sign flag if MSB */
		nibble_t Y[2]; /* Outputs (Tri-state when OE_=1) */
		bit_t OE_[2]; /* (Active LO) Output enables */
		byte_t Ycombiner; /* Combined output byte */
	} ALUs;



	/* Card edge connection to backplane */
	struct  card_edge {
		word_t A; /* A0-A15 Primary Address lines */
		twobit_t Ax; /* A16-A17? Extended address lines (banking) */
		byte_t D; /* D0-D7 Data lines */
	};
};


struct cpu6_components {
	/* ROMS */
	union {
		struct { prom76161_device_t UM3,      UL3,      UK3,      UJ3,      UH3,      UF3,      UE3; };
		struct { prom76161_device_t ROM0,     ROM1,     ROM2,     ROM3,     ROM4,     ROM5,     ROM6; };
		struct { prom76161_device_t ROM_E,    ROM_D,    ROM_A,    ROM_F,    ROM_C,    ROM_B,    ROM_M; };
		struct { prom76161_device_t ROM_0x30, ROM_0x08, ROM_0x10, ROM_0x28, ROM_0x00, ROM_0x18, ROM_0x20; };
	};

	/* uI Latches - Direct (ROM0-ROM5) */
	union {
		struct { logic_74ls377_device_t UM5,  UL5,  UK5,  UJ5,  UH5,  UF5;  };
		struct { logic_74ls377_device_t uIL0, uIL1, uIL2, uIL3, uIL4, uIL5; };
	};

	/* uIW6 Decoder (ROM6)*/
	union {
		struct { logic_74ls138_device_t UD2; logic_74ls139_device_t UD3; };
		struct { logic_74ls138_device_t uIW6D1; logic_74ls139_device_t uIW6D0; };
	};

	/* uIW6 Decoder Output latches  */
	union {
		struct { logic_74ls174_device_t UD4, UD5, UE5; };
		struct { logic_74ls174_device_t uIW6DL0, uIW6DL1, uIW6DL2; };
	};

	/* uIWR -> Sequencer Glue */
	union {
		struct { logic_74ls00_device_t UK6, UL6; logic_74ls04_device_t UJ6; };
		struct { logic_74ls00_device_t SeqInGlueNAND0, SeqInGlueNAND1; logic_74ls04_device_t SeqInGlueINV0; };
	};
	/* Sequencers */
	union {
		struct { am2909_device_t ULJ10,UL10; am2911_device_t UL9; };
		struct { am2909_device_t Seq0,Seq1; am2911_device_t Seq2; };
	};

	/* 256-byte Register File Address Decode PROM */
	union {
		struct { prom6309_device_t UB13; };
		struct { prom6309_device_t RFADecodeROM; };
	};

	/* Register File Address Latch */
	union {
		struct { logic_74ls377_device_t UC13; };
		struct { logic_74ls377_device_t RegFileAL; };
	};

	/* Interrupt Level Counter / Latch w/ Inverter on TC_->CEP_ to enable counting but no wrapping */
	union {
		struct { logic_74ls169_device_t UC15; logic_74ls04_device_t UA15C; };
		struct { logic_74ls169_device_t ILCnt; logic_74ls04_device_t ILCntEN; };
	};

	/* Register File Interrupt Level MUX */
	union {
		struct { logic_74ls157_device_t UC14; };
		struct { logic_74ls157_device_t RegFileILMUX; };
	};

	/* 256 Byte (2x256x4) Register File - 16 bytes in 16 interrupt levels */
	union {
		struct { ram_93l422_device_t UD14, UD15; };
		struct { ram_93l422_device_t RegFile0, RegFile1; };
	};

	/* Register File Output Latch */
	union {
		struct { logic_74ls374_device_t UD13; };
		struct { logic_74ls374_device_t RegFileOL; };
	};

	/* ALU Link (carry) Control MUX */
	union {
		struct {logic_74ls153_device_t UF6; };
		struct {logic_74ls153_device_t ALULMUX; };
	};

	/* ALU Shift Control MUX */
	union {
		struct {logic_74ls253_device_t UH6; };
		struct {logic_74ls253_device_t ALUSMUX; };
	};

	/* ALUs */
	union {
		struct { am2901_device_t UF10,UF11; };
		struct { am2901_device_t ALU0,ALU1; };
	};

	/* ALU Status Register */
	union {
		struct {logic_74ls378_device_t UJ9; };
		struct {logic_74ls378_device_t ALUSR; };
	};

	/* Address Latch Stage Sync */
	union {
		struct { logic_74ls157_device_t UD6; };
		struct { logic_74ls157_device_t ALSync; };
	};

	/* Address Latch Source MUXes */
	union {
		struct { logic_74ls157_device_t UB3,  UC3,  UB4,  UC4; };
		struct { logic_74ls157_device_t ALM0, ALM1, ALM2, ALM3; };
	};

	/* Address Counters/Latches A - Staging */
	union {
		struct { logic_74ls169_device_t UB2,  UC2,  UB5,  UC5; };
		struct { logic_74ls169_device_t AL0a, AL1a, AL2a, AL3a; };
	};
	
	/* Address Counters/Latches B - Output */
	union {
		struct { logic_74ls169_device_t UB1,  UC1,  UB6,  UC6; };
		struct { logic_74ls169_device_t AL0b, AL1b, AL2b, AL3b; };
	};

	/* Address Bus Tranceivers */
	union {
		struct { logic_ds8835_device_t UA4, UA5, UA6, UA8, UA9; };
		struct { logic_ds8835_device_t ABUSxcvr0, ABUSxcvr1, ABUSxcvr2, ABUSxcvr3, ABUSxcvr4; };
	};

	/* Data Bus Tranceivers */
	union {
		struct { logic_am2907_device_t UA11, UA12; };
		struct { logic_am2907_device_t DBUSxcvr0, DBUSxcvr1; };
	};
};

static char *ROM_files[] = {
	"ROMS/CPU_7.rom", /* MWM3.11 - E3.11 */
	"ROMS/CPU_6.rom", /* MWL3.11 - D3.11 */
	"ROMS/CPU_5.rom", /* MWK3.11 - A3.11 */
	"ROMS/CPU_4.rom", /* MWJ3.11 - F3.11 */
	"ROMS/CPU_3.rom", /* MWH3.11 - C3.11 */
	"ROMS/CPU_2.rom", /* MWF3.11 - B3.11 */
	"ROMS/CPU_1.rom"  /* MWE3.11 - ??3.11 */
};

#define C_(comp) (com.comp)

int main(int argc, char *argv[]) {
	struct cpu6_signals sig;
	struct cpu6_components com;
	clockline_t clockline_uIR, *cl_uIR;
	clockline_t clockline_ALU, *cl_ALU;
	am2901_device_t alu0, alu1;
	am2909_device_t seq0, seq1, seq2;

	bit_t NullBit=0;
	twobit_t NullTwobit=0;
	octal_t NullOctal=0;
	nibble_t NullNibble=0;
	byte_t NullByte=0;
	word_t NullWord=0;

	bit_t OneBit=1;
	twobit_t OneTwobit=1;
	octal_t OneOctal=1;
	nibble_t OneNibble=1;
	byte_t OneByte=1;
	word_t OneWord=1;
	
	bit_t UnaBit=0;
	twobit_t UnaTwobit=0;
	octal_t UnaOctal=0;
	nibble_t UnaNibble=0;
	byte_t UnaByte=0;
	word_t UnaWord=0;

	/* Setup bitblenders to combine multiple sources/outputs to a single output */
	byte_ptr_list_t seq_output_list={&sig.Seqs.Y[0], &sig.Seqs.Y[1], &sig.Seqs.Y[2]};
	char *seq_output_order="\x00\x01\x02\x03\xff\xff\xff\xff\x04\x05\x06\x07\xff\xff\xff\xff\x08\x09\x0a\x0b";
	bitblender_t seq_output_blender={{.w=&sig.uROM.uA},11,seq_output_order,&seq_output_list};

	/* Setup clocklines */
	cl_uIR=&clockline_uIR;
	cl_ALU=&clockline_ALU;

	sig.uROM.CE1_=0;
	sig.uROM.CE2=1;
	sig.uROM.CE3=1;

	/* Load and initialize 76161 PROMS */
	prom76161_init(&com.ROM0,ROM_files[0], &sig.uROM.uA, &sig.uROM.uD[0], &sig.uROM.CE1_, &sig.uROM.CE2, &sig.uROM.CE3);
	prom76161_init(&com.ROM1,ROM_files[1], &sig.uROM.uA, &sig.uROM.uD[1], &sig.uROM.CE1_, &sig.uROM.CE2, &sig.uROM.CE3);
	prom76161_init(&com.ROM2,ROM_files[2], &sig.uROM.uA, &sig.uROM.uD[2], &sig.uROM.CE1_, &sig.uROM.CE2, &sig.uROM.CE3);
	prom76161_init(&com.ROM3,ROM_files[3], &sig.uROM.uA, &sig.uROM.uD[3], &sig.uROM.CE1_, &sig.uROM.CE2, &sig.uROM.CE3);
	prom76161_init(&com.ROM4,ROM_files[4], &sig.uROM.uA, &sig.uROM.uD[4], &sig.uROM.CE1_, &sig.uROM.CE2, &sig.uROM.CE3);
	prom76161_init(&com.ROM5,ROM_files[5], &sig.uROM.uA, &sig.uROM.uD[5], &sig.uROM.CE1_, &sig.uROM.CE2, &sig.uROM.CE3);
	prom76161_init(&com.ROM6,ROM_files[6], &sig.uROM.uA, &sig.uROM.uD[6], &sig.uROM.CE1_, &sig.uROM.CE2, &sig.uROM.CE3);

	/* Split up byte from UE3 into D210, DeMUXSel(D3), D7654 */
	bitsalad_prep_small( &sig.uIW6D.DinSalad[0], 3, &sig.uROM.uD[6], &sig.uIW6D.D210, 0x77777210 );
	bitsalad_prep_small( &sig.uIW6D.DinSalad[1], 1, &sig.uROM.uD[6], &sig.uIW6D.DeMUXSel, 0x77770777 );
	bitsalad_prep_small( &sig.uIW6D.DinSalad[2], 4, &sig.uROM.uD[6], &sig.uIW6D.D7654, 0x32107777 );


	/* Initialize uIW6 Decoders */
	com.uIW6D0 = (dt_74(ls139)) { .BA1= &sig.uIW6D.D210, .G1_= &sig.uIW6D.DeMUXSel, .Y1=&sig.uIW6D.uIW6D0O,
		.BA2= &UnaNibble, .G2_= &UnaBit, .Y2=&UnaOctal,};

	com.uIW6D1 = (dt_74(ls138)) { .CBA= &sig.uIW6D.D210, G1: &sig.uIW6D.DeMUXSel, Y: &sig.uIW6D.uIW6D1O,
		.G2A_=&NullBit, .G2B_=&NullBit };

	/* Initialize bitsalad_bags to swap signal order between uROMs data out and uILs data in */
	// ROM  : 7 6 5 4 3 2 1 0
	// '377 : 0 1 5 4 3 2 6 7
	for(int k=0; k<6; k++) { bitsalad_prep_small( &sig.uIWR.uDinSalad[k], 8, &sig.uROM.uD[k], &sig.uIWR.uD[k], 0x01543267 ); }

	/* Initialize uIW Latches uIL0-uIL5 */
	com.uIL0 = (dt_74(ls377)) { .clk=&cl_uIR->clk, .D=&sig.uIWR.uD[0], .Q=&sig.uIWR.uQ[0], .G_=&NullBit };
	com.uIL1 = (dt_74(ls377)) { .clk=&cl_uIR->clk, .D=&sig.uIWR.uD[1], .Q=&sig.uIWR.uQ[1], .G_=&NullBit };
	com.uIL2 = (dt_74(ls377)) { .clk=&cl_uIR->clk, .D=&sig.uIWR.uD[2], .Q=&sig.uIWR.uQ[2], .G_=&NullBit };
	com.uIL3 = (dt_74(ls377)) { .clk=&cl_uIR->clk, .D=&sig.uIWR.uD[3], .Q=&sig.uIWR.uQ[3], .G_=&NullBit };
	com.uIL4 = (dt_74(ls377)) { .clk=&cl_uIR->clk, .D=&sig.uIWR.uD[4], .Q=&sig.uIWR.uQ[4], .G_=&NullBit };
	com.uIL5 = (dt_74(ls377)) { .clk=&cl_uIR->clk, .D=&sig.uIWR.uD[5], .Q=&sig.uIWR.uQ[5], .G_=&NullBit };

	/* Initialize bitsalad_bags to reorder signal going into uIW6D Latches */
	/* Reorder UD2A->UD4, UD3->UD5, D7654(UE3)->UE5 */
	bitsalad_prep_small( &sig.uIWR.uD6inSalad[0], 4, &sig.uIW6D.uIW6D0O, &sig.uIWR.uD6[0], 0x703271 );
	bitsalad_prep_small( &sig.uIWR.uD6inSalad[1], 6, &sig.uIW6D.uIW6D1O, &sig.uIWR.uD6[1], 0x235410 );
	bitsalad_prep_small( &sig.uIWR.uD6inSalad[2], 4, &sig.uIW6D.D7654, &sig.uIWR.uD6[2], 0x320771 );

	/* Initialize uIW6D section Latches */
	com.uIW6DL0= (dt_74(ls174)) { .clk= &cl_uIR->clk, .CLR_= &sig.bus.RESET_, .D= &sig.uIWR.uD6[0], .Q=&sig.uIWR.uQ6[0] };
	com.uIW6DL1= (dt_74(ls174)) { .clk= &cl_uIR->clk, .CLR_= &sig.bus.RESET_, .D= &sig.uIWR.uD6[1], .Q=&sig.uIWR.uQ6[1] };
	com.uIW6DL2= (dt_74(ls174)) { .clk= &cl_uIR->clk, .CLR_= &sig.bus.RESET_, .D= &sig.uIWR.uD6[2], .Q=&sig.uIWR.uQ6[2] };

	/* Create a combined output from the uIWD latches */
	/* Mapping from output of UE3 to input of UD4/UD5 */
	/* 0_00->D4.4, 0_01->D4.0, 0_10->D4.2, 0_11->D4.3 */
	/* 1000->D5.0, 1001->D5.1, 1010->D5.5, 1011->D5.4 */
	/* 1100->D5.2, 1101->D5.3                         */
	/* D4->E5.3, D5->E5.0, D6->E5.4, D7->E5.5 */
	
	/* Setup bitblenders to combine multiple sources/outputs to a single output */
	sig.uIWR.uIWQ6_toword_blender= (bitblender_t) {
		.w=&sig.uIWR.uQ6_cword, .bits=16,
		.order= "\x00\x01\x02\x03\x04\x05\xff\xff" "\x06\x07\x08\x09\x0a\xb\xff\xff" "\x0c\xff\xff\x0d\x0e\x0f\xff\xff",
		.sources= &(byte_ptr_list_t) {&sig.uIWR.uD6[0],&sig.uIWR.uD6[1],&sig.uIWR.uD6[2]}
	};

	/*
	 * uIWR Output Glue
	 */
	/* UL5.Q0 -> B.UK6A.Y -> Seq1.S0 / Seq2.S0 */
	/* UH5.Q7 -> (A.UK6B.Y -> Seq2.S1) / (A.UJ6C.Y -> B.UK6D.Y -> B.UK6C.Y -> Seq1.S1) */
	com.SeqInGlueNAND0= (dt_74(ls00)) {
		.A1=&sig.state.INT_, .B1=&sig.uIWR.uIWSeqs12S0, .Y1=&sig.Seqs.GlueNAND0AY,
		.A2=&sig.uIWR.uIWSeqs12S1, .B2=&sig.state.INT_, .Y2=&sig.Seqs.GlueNAND0BY,
		.A3=&sig.state.INT_, .B3=&sig.Seqs.GlueNAND0DY, .Y3=&sig.Seqs.GlueNAND0CY,
		.A4=&sig.uIWR.uIWSeq1S1EN, .B4=&sig.Seqs.GlueINV0CY, .Y4=&sig.Seqs.GlueNAND0DY
	};

	/* UL5.Q5 -> A.UL6A.Y -> Seq0.S0 */
	/* UL5.Q3 -> A.UL6B.Y -> A.UJ6D.Y -> Seqs.FE_*/
	/* UL5.Q1 -> A.UL6D.Y -> Seq0.S1 */
	com.SeqInGlueNAND1= (dt_74(ls00)) {
		.A1=&sig.uIWR.uIWSeq0S0, .B1=&sig.state.INT_, .Y1=&sig.Seqs.GlueNAND1AY,
		.A2=&sig.uIWR.uIWSeqsFE, .B2=&sig.state.INT_, .Y2=&sig.Seqs.GlueNAND1BY,
		.A3=&UnaBit, .B3=&UnaBit, .Y3=&sig.Seqs.GlueNAND1CY,
		.A4=&sig.uIWR.uIWSeq0S1, .B4=&sig.state.INT_, .Y4=&sig.Seqs.GlueNAND1DY,
	};

	/* UH5.Q7 -> (A.UK6B.Y -> Seq2.S1) / (A.UJ6C.Y -> B.UK6D.Y -> B.UK6C.Y -> Seq1.S1) */
	/* UL5.Q3 -> A.UL6B.Y -> A.UJ6D.Y -> Seqs.FE_*/
	com.SeqInGlueINV0= (dt_74(ls04)) {
		.A1=&UnaBit, .Y1=&UnaBit,
		.A2=&UnaBit, .Y2=&UnaBit,
		.A3=&sig.uIWR.uIWSeqs12S1, .Y3=&sig.Seqs.GlueINV0CY,
		.A4=&sig.Seqs.GlueNAND1BY, .Y4=&sig.Seqs.FE_,
		.A5=&UnaBit, .Y5=&UnaBit,
		.A6=&UnaBit, .Y6=&UnaBit
	};
	

	/* Initialize two am2909 and one am2911 sequencers */
	sig.Seqs.Cin=OneBit; /* Force incoming carry on */
	am2909_init(&com.Seq0,"Seq0",&cl_uIR->clk,
		&sig.Seqs.S[0],
		&sig.Seqs.FE_, &sig.Seqs.PUP,
		&sig.Seqs.Di[0],&sig.Seqs.Ri[0],&sig.Seqs.RE_,
		&sig.Seqs.Cin,&sig.Seqs.C4,
		&sig.Seqs.ORi[0],&sig.Seqs.ZERO_[0],&sig.Seqs.OE_,&sig.Seqs.Y[0]
	);

	am2909_init(&com.Seq1,"Seq1",&cl_uIR->clk,
		&sig.Seqs.S[1],
		&sig.Seqs.FE_, &sig.Seqs.PUP,
		&sig.Seqs.Di[1],&sig.Seqs.Ri[1],&sig.Seqs.RE_,
		&sig.Seqs.C4,&sig.Seqs.C8,
		&sig.Seqs.ORi[1],&sig.Seqs.ZERO_[1],&sig.Seqs.OE_,&sig.Seqs.Y[1]
	);

	am2911_init(&com.Seq2,"Seq2",&cl_uIR->clk,
		&sig.Seqs.S[2],
		&sig.Seqs.FE_, &sig.Seqs.PUP,
		&sig.Seqs.Di[2],&sig.Seqs.RE_,
		&sig.Seqs.C8,&sig.Seqs.Cout,
		&sig.Seqs.ZERO_[2],&sig.Seqs.OE_,&sig.Seqs.Y[2]
	);

	/* Bag some salad to assemble ALU instruction word octal words */
	bitsalad_prep_small( &sig.ALUs.I876Salad, 3, &sig.uIWR.uQ[5], &sig.ALUs.I876, 0x01777277 );
	bitsalad_prep_small( &sig.ALUs.I543Salad, 3, &sig.uIWR.uQ[4], &sig.ALUs.I543, 0x77077712 );
	bitsalad_prep_small( &sig.ALUs.I210Salad, 3, &sig.uIWR.uQ[4], &sig.ALUs.I210, 0x77721077 );

	/* Setup bitblenders for ALU inputs where salad ain't enough */
	sig.ALUs.ADDR_AShake= (bitblender_t) {
		.b=&sig.ALUs.ADDR_A, .bits=4,
		.order= "\x00\xff\xff\xff\xff\xff\xff\xff" "\xff\xff\x03\xff\xff\xff\x02\x01",
		.sources= &(byte_ptr_list_t) {&sig.uIWR.uQ[5],&sig.uIWR.uQ[2]}
	};
	bitsalad_prep_small( &sig.ALUs.ADDR_BSalad, 4, &sig.uIWR.uQ[5], &sig.ALUs.ADDR_B, 0x77210737 );


	/* Initialize the two am2901 ALUs */
	am2901_init(&com.ALU0,"ALU0",&cl_ALU->clk,
		&sig.ALUs.I210, &sig.ALUs.I543, &sig.ALUs.I876,
		&sig.ALUs.RAM0[0], &sig.ALUs.RAM3[0],
		&sig.ALUs.ADDR_A, &sig.ALUs.ADDR_B, &sig.ALUs.D[0],
		&sig.ALUs.Cin, &sig.ALUs.P_[0], &sig.ALUs.G_[0], &sig.ALUs.Chalf,
		&sig.ALUs.OVR[0], &sig.ALUs.Q0[0], &sig.ALUs.Q3[0],
		&sig.ALUs.FZ[0], &sig.ALUs.F3[0], &sig.ALUs.Y[0], &sig.ALUs.OE_[0]);

	am2901_init(&com.ALU1,"ALU1",&cl_ALU->clk,
		&sig.ALUs.I210, &sig.ALUs.I543, &sig.ALUs.I876,
		&sig.ALUs.RAM0[1], &sig.ALUs.RAM3[1],
		&sig.ALUs.ADDR_A, &sig.ALUs.ADDR_B, &sig.ALUs.D[1],
		&sig.ALUs.Chalf, &sig.ALUs.P_[1], &sig.ALUs.G_[1], &sig.ALUs.Cout,
		&sig.ALUs.OVR[1], &sig.ALUs.Q0[1], &sig.ALUs.Q3[1],
		&sig.ALUs.FZ[1], &sig.ALUs.F3[1], &sig.ALUs.Y[1], &sig.ALUs.OE_[1]);


	sig.uROM.uA=0;
	for(int j=0; j<120; j++) {

	/* Advance the clock phase */
	clock_advance(cl_uIR);

	/* For now, inc the micro rom address by one each low phase of the clock */
	if(cl_uIR->clk == CLK_LO) {sig.uROM.uA++;}

	/* Update PROM outputs based on address input and CE1_,CE2,CE3 */
	prom76161_update(&com.ROM_A);
	prom76161_update(&com.ROM_B);
	prom76161_update(&com.ROM_C);
	prom76161_update(&com.ROM_D);
	prom76161_update(&com.ROM_E);
	prom76161_update(&com.ROM_F);
	prom76161_update(&com.ROM_M);

	printf("uROM byte 0x%02x -   ", sig.uROM.uA);
	for(int n=0; n<7;n++){
		printf("  0x%02x: 0x%02x", n, sig.uROM.uD[n]);
	}
	printf("\n");

	/* Route uROM 0-5 outputs to directly attached byte sized instruction latches */
	for(int k=0; k<6; k++) { bitsalad_shooter( &sig.uIWR.uDinSalad[k] ); }
	/* Distribute byte from uROM 6 outputs to two decoders w/enable and a nibble to hex latch */
	for(int k=0; k<3; k++) { bitsalad_shooter( &sig.uIW6D.DinSalad[k] ); }
	
	/* Update instruction latches' state */
	_74(ls377)(&com.uIL0);
	_74(ls377)(&com.uIL1);
	_74(ls377)(&com.uIL2);
	_74(ls377)(&com.uIL3);
	_74(ls377)(&com.uIL4);
	_74(ls377)(&com.uIL5);

	printf("uIWR byte 0x%02x -   ", sig.uROM.uA);
	for(int n=0; n<6;n++){
		printf("  0x%02x: 0x%02x", n, sig.uIWR.uQ[n]);
	}
	printf("\n");
	
	/* Decode output of highest order board byte (lowest order logical) of uROM */
	logic_74ls139(&com.uIW6D0);
	logic_74ls138(&com.uIW6D1);

	/* Route inputs from uROM UE3 and attached decoders to their resepective instruction word latches */
	for(int k=0; k<3; k++) { bitsalad_shooter( &sig.uIWR.uD6inSalad[k] ); }

	/* Latch decoded result of of uIW6 */
	logic_74ls174(&com.uIW6DL0);
	logic_74ls174(&com.uIW6DL1);
	logic_74ls174(&com.uIW6DL2);
	

	/*
	 * Provide some instrumentation of what we get out of the combined effective instruciton latch
	 */

	bitblend(&sig.uIWR.uIWQ6_toword_blender);

	/* Split our cword from combined output of uIW6D latches into two bytes */
	word_to_bytes(&sig.uIWR.uQ6_cword, &sig.uIWR.uQ6_cbyte[0], &sig.uIWR.uQ6_cbyte[1]);
	printf("word_to_bytes returned 0x%02x and 0x%02x\n", sig.uIWR.uQ6_cbyte[0], sig.uIWR.uQ6_cbyte[1]);
	/* Combine all of our instruction register output latch values into one mouthful */
	bytes_to_mouthful( &sig.uIWR.uIWR_cmouthful,
		&sig.uIWR.uQ[0], &sig.uIWR.uQ[1], &sig.uIWR.uQ[2], &sig.uIWR.uQ[3],
		&sig.uIWR.uQ[4], &sig.uIWR.uQ[5], &sig.uIWR.uQ6_cbyte[0], &sig.uIWR.uQ6_cbyte[1]);

	printf("Mouthful is:0x%0"PRIx64"\n",sig.uIWR.uIWR_cmouthful);

	/*
	 * ALUs
	 */

	/* Assemble I876 from lines from UF5 */
	bitsalad_shooter(&sig.ALUs.I876Salad);

	/* Assemble I543 and I210 from lines from UH5 */
	bitsalad_shooter(&sig.ALUs.I543Salad);
	bitsalad_shooter(&sig.ALUs.I210Salad);
	
	/* Would you like fries with that? (ADDR_A) */
	bitblend(&sig.ALUs.ADDR_AShake);
	
	/* No thanks, a Salad will be fine! (ADDR_B) */
	bitsalad_shooter(&sig.ALUs.ADDR_BSalad);
	
	/* Bump our ALUclock phase */
	clock_advance(cl_ALU);

	/* Update ALU state */
	am2901_update(&com.ALU0);
	am2901_update(&com.ALU1);

	/* Signals with glue logic needing bitmangling */
		/* UL5.Q3 -> A.UL6B.Y -> A.UJ6D.Y -> Seqs.FE_*/
	bit_of_a_byte(&sig.uIWR.uIWSeqsFE, &sig.uIWR.uQ[1], 3);
		/* UL5.Q4 -> Seqs.PUP (No NAND on this signal?) */
	bit_of_a_byte(&sig.Seqs.PUP, &sig.uIWR.uQ[1], 4);
		/* UL5.Q5 -> A.UL6A.Y -> Seq0.S0 */
	bit_of_a_byte(&sig.uIWR.uIWSeq0S0, &sig.uIWR.uQ[1], 5);
		/* UL5.Q1 -> A.UL6D.Y -> Seq0.S1 */
	bit_of_a_byte(&sig.uIWR.uIWSeq0S1, &sig.uIWR.uQ[1], 1);
		/* UL5.Q0 -> B.UK6A.Y -> Seq1.S0 / Seq2.S0 */
	bit_of_a_byte(&sig.uIWR.uIWSeqs12S0, &sig.uIWR.uQ[1], 0);
		/* UH5.Q7 -> (A.UK6B.Y -> Seq2.S1) / (A.UJ6C.Y -> B.UK6D.Y -> B.UK6C.Y -> Seq1.S1) */
	bit_of_a_byte(&sig.uIWR.uIWSeqs12S1, &sig.uIWR.uQ[3], 7);
		/* UK5.Q1 - Enable control of Seq1.S1 by UH5.Q7, otherwise HI to NAND UK6C */
	bit_of_a_byte(&sig.uIWR.uIWSeq1S1EN, &sig.uIWR.uQ[2], 1);

	/* Propagate signals all the way throug the glue logic (Split gates might be wiser here?) */
	_74(ls04)(&com.SeqInGlueINV0);
	_74(ls00)(&com.SeqInGlueNAND0);
	_74(ls00)(&com.SeqInGlueNAND0);
	_74(ls00)(&com.SeqInGlueNAND1);
	_74(ls04)(&com.SeqInGlueINV0);

	/* Glue output from glue together for sequencer source selects */
	bits_to_twobit(&sig.Seqs.S[0], &sig.Seqs.GlueNAND1AY, &sig.Seqs.GlueNAND1DY);
	bits_to_twobit(&sig.Seqs.S[1], &sig.Seqs.GlueNAND0AY, &sig.Seqs.GlueNAND0CY);
	bits_to_twobit(&sig.Seqs.S[2], &sig.Seqs.GlueNAND0AY, &sig.Seqs.GlueNAND0BY);


	/* Update sequencer state */
	am2909_update(&com.Seq0);
	am2909_update(&com.Seq1);
	am2911_update(&com.Seq2);

	/* Update uROM address inputs from collected nibbles of output from sequencers */ 
//	//nibbles_to_word(&sig.uROM.uA, &sig.Seqs.Y[0], &sig.Seqs.Y[1], &sig.Seqs.Y[2], &NullByte);


//	nibbles_to_byte(&sig.ALUs.Ycombiner, &sig.ALUs.Y[0], &sig.ALUs.Y[1]);

	}
	return(0);
}
