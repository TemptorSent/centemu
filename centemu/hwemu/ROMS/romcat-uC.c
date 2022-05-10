#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "../logic-common.h"
#include "../clockline.h"
#include "../ginsumatic.h"
#include "../am2901.h"
#include "../am2909.h"
#include "../rom-common.h"
#include "./bootstrap_rom.h"

#define NUMROMS 7
#define ROMSIZE 2048

/* Utility functions to extract ranges of bits, with _R reversing the bit order returned */
#define BITRANGE(d,s,n) ((d>>s) & ((1LL<<(n))-1LL) )
#define BITRANGE_R(d,s,n) (bitreverse_64(BITRANGE(d,s,n)) >>(64-n))

typedef struct comment_t { word_t addr; char *comment; } comment_t;
#include "comments_generated.h"

// Remeove me when address bus works
byte_t inject;

static uint8_t allrom[NUMROMS][ROMSIZE];
static uint8_t mergedrom[ROMSIZE][NUMROMS];
static uint64_t iws[ROMSIZE];
static char *ROM_files[NUMROMS] = {
	"CPU_5.rom", /* MWK3.11 - A3.11 */
	"CPU_2.rom", /* MWF3.11 - B3.11 */
	"CPU_3.rom", /* MWH3.11 - C3.11 */
	"CPU_6.rom", /* MWL3.11 - D3.11 */
	"CPU_7.rom", /* MWM3.11 - E3.11 */
	"CPU_4.rom", /* MWJ3.11 - F3.11 */
	"CPU_1.rom"  /* MWE3.11 - ??3.11 */
};

#define MAPROMSIZE 0x100
uint8_t maprom[MAPROMSIZE];
static char *MAPROM_file = "CPU-6309";

/*static char *ROM_files[NUMROMS] = {
	"CPU_1.rom",
	"CPU_2.rom",
	"CPU_3.rom",
	"CPU_4.rom",
	"CPU_5.rom",
	"CPU_6.rom",
	"CPU_7.rom"
};
*/

typedef struct cpu_state_t {
	struct dev {
		am2901_device_t ALU0;
		am2901_device_t ALU1;
		am2909_device_t Seq0;
		am2909_device_t Seq1;
		am2911_device_t Seq2;
	} dev;

	struct Shifter {

		bit_t DownLine, UpLine;
	} Shifter;

	struct ALU {
		clockline_t cl;
		octal_t I876, I543, I210;
		nibble_t ADDR_A, ADDR_B;
		nibble_t Dlow, Dhigh;
		bit_t Cin, Chalf, Cout;
		bit_t RAM3, RAM7;
		bit_t RAM0Q7;
		bit_t Q0, Q3;
		bit_t FZ, F3, F7;
		bit_t P_0,G_0, P_1,G_1;
		bit_t nibbleOVF, OVF;
		nibble_t Flow, Fhigh;
		bit_t OE_;
	} ALU;

	struct Seq {
		clockline_t cl; // Clockline
		nibble_t DiS0, DiS1, DiS2; // uADDR input, split
		nibble_t RiS0, RiS1; // Ri inputs from F-bus
		twobit_t S0S, S1S, S2S; // Sequencer Source Selects
		bit_t FE_, PUP; // Shift controls, File Enable (Active LO), and Push/Pop
		bit_t Cn, C4, C8, Co; // Carry-in input, Intermediate carries C4,C8, Carry Output
		bit_t RE_; // Seq AR input enable
		nibble_t ORiS0, ORiS1; // ORi inputs for S0,S1
		bit_t ZERO_; // Zero ouputs when LO
		nibble_t YS0, YS1, YS2; // Outputs
		bit_t OE_; // Enable outputs when LO, HiZ when HI
	} Seq;

	struct Bus {
		word_t A;
		byte_t iD;
		byte_t xD;
		byte_t F;
		byte_t R;
		struct Sys {
			uint32_t ADDR;
			byte_t DATA;
		} Sys;
	} Bus;

	struct Reg {
		byte_t RIR; // Register Index Register (UC13)
		nibble_t ILR; // Interrupt Level Register (UC15, read w/UH14)
		byte_t SDBOR; // Databus Output Register (UA11/UA12)
		byte_t SDBRL; // Databus Receive Latch (UA11/UA12)
		byte_t DL; // Data Latch (UC11/UC12)
		byte_t SR; // Status Register (UJ9)
		byte_t RR; // Result Register (UC9)
		word_t ALS; // Address Latch, Staging (UB2/UC2 / UB5/UC5)
		word_t ALO; // Address Latch, Output (UB1/UC1 / UB6/UC6)
		byte_t PTBAR; // Page Table Base Address Register (UD11)
		byte_t PTBL[0x100]; // Page Table (2x256x4 SRAMs UB9/UB10)
		byte_t RF[0x100]; // Register File (ABXYZSCPx2x16) (2x256x4 SRAMs UD14/UD15)
		byte_t LUF11; // Addressable latch UF11 - Controls EI_/DI, ExtADB_EN, etc.
	} Reg;

	struct IO {
		byte_t DIPSW;
	} IO;
} cpu_state_t;


typedef struct uIW_t {
	bit_t S1S1_OVR_;
	twobit_t SHCS;
	nibble_t A,B;
	octal_t I876, I543, I210;
	bit_t CASE_;
	twobit_t S0S, S1S, S2S;
	bit_t PUP, FE_;
	word_t uADDR;
	byte_t DATA_;
	octal_t D_E6,D_E7,D_H11,D_K11;
	nibble_t D_D2D3;
} uIW_t;


typedef struct uIW_trace_t {
	word_t uADDR_Prev;
	word_t uADDR;
	word_t uADDR_Next;
	uIW_t uIW;
	nibble_t Seq0Op, Seq1Op, Seq2Op;
	nibble_t D2_Out;
	sixbit_t D3_Out, E6_Out, K11_Out, H11_Out, E7_Out;
	octal_t S_Shift;
	twobit_t S_Carry;
	byte_t iD;
	byte_t F;
	byte_t R;
	nibble_t RIR;
	
} uIW_trace_t;


enum decoder_enum { D_D2, D_D3, D_E6, D_K11, D_H11, D_E7 };
enum decoder_outputs {
	D_D2_0_READ_DL=0x00, D_D2_1_READ_REG=0x02, D_D2_2_READ_BUS_SYS_ADDR_MSB=0x04, D_D2_3_READ_BUS_SYS_ADDR_LSB=0x06,

	D_D3_0_D5_0=0x10, D_D3_1_D5_1=0x12, D_D3_2_READ_BUS_SYS_DATA=0x14, D_D3_3_READ_ILR=0x16,
	D_D3_4_READ_DIPSW_NIB_HIGH=0x18, D_D3_5_READ_uCDATA=0x1a,

	D_E6_0=0x20, D_E6_1_WRITE_RR=0x22, D_E6_2_WRITE_RIR=0x24, D_E6_3_D9_EN=0x26,
	D_E6_4_WRITE_PTBAR=0x28, D_E6_5_ALS_SRC_PC=0x2a, D_E6_5_ALS_SRC_DATA=0x2b, D_E6_6_WRITE_SEQ_AR=0x2c, D_E6_7=0x2e,

	D_K11_0=0x30, D_K11_1=0x32, D_K11_2=0x34, D_K11_3_F11_ENABLE=0x36,
	D_K11_4_WRITE_RF=0x38, D_K11_5=0x3a, D_K11_6=0x3c, D_K11_7_WRITE_BUS_SYS_DATA=0x3e,

	D_H11_0=0x40, D_H11_1_READ_RR=0x42, D_H11_2=0x44, D_H11_3_WRITE_ALS_MSB=0x46,
	D_H11_4=0x48, D_H11_5=0x4a, D_H11_6_READ_MAPROM=0x4c, D_H11_7_READ_ALU_RESULT=0x4d, D_H11_7=0x4e,

	D_E7_0=0x50, D_E7_1_UE14_CLK_EN=0x52, D_E7_2_WRITE_SR=0x54, D_E7_3=0x56,
	D_E7_4_UNUSED=0x58, D_E7_5_UNUSED=0x5a, D_E7_6_UNUSED=0x5c, D_E7_7_UNUSED=0x5e
};


static char *decoded_sig[6][8][2] = {
	{ // Decoder D2 -> latch UD4
		{"READ DATA LATCH -> (iD-Bus)",""}, // C12p1
		{"READ REGISTER FILE -> (iD-Bus)",""}, // D13p1
		{"READ ADDRESS BUS MSB (A-Bus) -> (iD-Bus)",""}, // A6p7
		{"READ ADDRESS BUS LSB (A-Bus)-> (iD-Bus)",""}, // A4p7
		{"",""},{"",""},
		{"",""},{"",""}
	},
	{ // Decoder D3 -> latch UD5
		{"D3.0 D5.0 A9p7 (Read AB PA11-PA13?) -> (iD-Bus)", ""}, // A9p7
		{"D3.1 D5.1 M7p19 (Read dipsw low?) -> (iD-Bus)",""}, // M7p19
		{"READ EXTERNAL DATA BUS (xD-Bus) -> (iD-Bus)",""}, // A12p11
		{"READ IL REGISTER -> (iD-Bus)",""}, // A8p7
		{"READ DIP SWITCHES HIGH NIBBLE (iD-Bus)",""}, // M7p1
		{"READ uC DATA CONSTANT (uIW) -> (iD-Bus)",""}, // M6p18
		{"",""},{"",""}

	},
	{ // Decoder E6 (from latch E5)
		{"E6.0",""},
		{"WRITE RESULT REGISTER (R-Bus) <- (F-Bus)",""}, // C9p1
		{"WRITE REGISTER INDEX SELECTION REGISTER <- (F-Bus)",""}, // C13p1
		{"E6.3 (D9 Enable?)",""}, // D9p1
		{"WRITE PAGETABLE BASE ADDRESS REGISTER <- (F-Bus)",""},  // D11p1
		{"STAGING ADDRESS LATCH SOURCE = CURRENT PC <- (A-Bus)","STAGING ADDRESS LATCH SOURCE = RESULT <- (R-Bus)"}, // C4p1
		{"WRITE DATA TO SEQUENCERS ADDRESS REGISTER <- (F-Bus)",""}, // AM2909p1 (RE_)
		{"E6.7 (Load Condition Code Reg M12?)",""} // M12p1
	},
	{ // Decoder K11
		{"K11.0",""},
		{"K11.1",""},
		{"K11.2 (M13 Gate?)",""},
		{"K11.3 (F11 Enable)",""},
		{"K11.4 (Write Register File?) <- (R-Bus)",""}, // Towards WRITE REGFILE K11.4 -> ?.H13Bp8 -> UD14.WE_/UD15.WE_
		{"K11.5",""},
		{"K11.6",""},
		{"WRITE EXTERNAL DATA BUS REGISTER <- (R-Bus)",""}
	},
	{ // Decoder H11
		{"H11.0",""},
		{"H11.1 (X Read Result Register? -> (R-Bus))",""},
		{"H11.2",""},
		{"WRITE STAGING ADDRESS LATCH MSB <- (A-Bus/R-Bus)",""},
		{"H11.4",""},
		{"H11.5",""},
		{"READ MAPPING PROM -> (F-Bus)","READ ALU RESULT -> (F-Bus)"},
		{"H11.7",""}
	},
	{ // Decoder E7
		{"E7.0",""},
		{"E7.1 (E14 Clock enable?)",""},
		{"WRITE STATUS REGISTER",""},
		{"E7.3",""},
		{"",""},
		{"",""},
		{"",""},
		{"",""}
	}

};

static char *shifter_ops[8]= {
	"S->RAM7", "?Up1(0?)->RAM7", "Q0->RAM7", "C->RAM7",
	"RAM7->Q0", "?Dn1(0?)->Q0", "S->Q0", "?Dn3(C?)->Q0"
};

static char *carry_ops[4]= {
	"?0->Cin", "?1->Cin", "?C->Cin", "?->Cin"
};

int read_roms() {
	int r;
	for(int i=0; i<NUMROMS; i++) {
		r=rom_read_file(ROM_files[i],ROMSIZE,allrom[i]);
		if(r) { printf("Error %x reading file %s\n", r, ROM_files[i]); return(r); }
	}
	r=rom_read_file(MAPROM_file, MAPROMSIZE, maprom);
	return(r);
/*
	FILE *fp;
	size_t ret_code;

	for(int  i=0; i<NUMROMS; i++) {
		fp=fopen(ROM_files[i],"rb");
		ret_code=fread(allrom[i],1,ROMSIZE,fp);
		if(ret_code != ROMSIZE) {
			if(feof(fp)) {
				printf("Unexpected EOF while reading %s: Only got %u byte of an expected %s.\n",
					ROM_files[i], ret_code, ROMSIZE);
			} else if(ferror(fp)){
				printf("Failed while reading %s!\n",ROM_files[i]);
			}
			fclose(fp);
			return(-1);
		}
		fclose(fp);

		//printf("firstval: %x\n",allrom[i][0]);
	}
	return((int)ret_code);
*/
}

int merge_roms() {
	uint64_t iw;
	for(int i=0; i<ROMSIZE; i++) {
		//printf("\n%04x",i);
		for(int j=0; j<NUMROMS; j++) {
			mergedrom[i][NUMROMS-j-1]=allrom[j][i];
			//printf(" %02x",mergedrom[i][j]);
		}
		iw=concat_bytes_64(NUMROMS,mergedrom[i]);
		iws[i]=iw;
		/*
		printf("\tI:%03o A':%x A:%x  %#018"PRIx64"",
			BITRANGE(iw,34,9),
			BITRANGE(iw,44,4),
			BITRANGE_R(iw,44,4),
			iw
		);
		*/
	}

	return(ROMSIZE);
}

void print_comments(word_t addr) {
	comment_t *cmt=comments;
	while(cmt->comment) {
		if(cmt->addr == addr) { printf("// %s\n",cmt->comment);}
		cmt++;
	}
}

byte_t sysbus_read_data(word_t address) {
	// Bootstrap ROM
	if(address >= 0xfc00 && address < 0xfe00) {
		return(bootstrap_rom[address-0xfc00]);
	}
	return(0);
}


void parse_uIW(uIW_t *uIW, uint64_t in) {

	uIW->S1S1_OVR_=BITRANGE(in,54,1); /* Override bit1=1 of S1S when LO, otherwise S1S=S2S */
	uIW->SHCS=BITRANGE(in,51,2); /* Shifter/Carry Control Select */
	uIW->A=BITRANGE(in,47,4); /* ALU A Address (Source) */
	uIW->B=BITRANGE(in,43,4); /* ALU B Address (Source/Dest) */
	uIW->I876=BITRANGE(in,40,3); /* ALU I876 - Destination Control */
	uIW->I543=BITRANGE(in,37,3); /* ALU I543 - Operation */
	uIW->I210=BITRANGE(in,34,3); /* ALU I210 - Operand Source Select */
	uIW->CASE_=BITRANGE(in,33,1); /* Enable conditoinal logic when LO */
	uIW->S2S=BITRANGE(in,31,2); /* Sequencer2 Source Select */
	uIW->S0S=BITRANGE(in,29,2); /* Sequencer0 Source Select */
	uIW->PUP=BITRANGE(in,28,1); /* Sequencers Pop/Push Direction Select */
	uIW->FE_=BITRANGE(in,27,1); /* Sequencers (Push/Pop) File Enable (Active LO) */
	uIW->uADDR=BITRANGE(in,16,11); /* uC Address (11bits, lower 8 multiplex with DATA_) */
	uIW->DATA_=BITRANGE(in,16,8); /* Data (inverted) (=low 8 bits of uC Address) */
	//uIW->??/=BITRANGE(in,15,1); /* MUX UK9p7 */
	uIW->D_E7=BITRANGE(in,13,2); /* Decoder UE7 */
	uIW->D_H11=BITRANGE(in,10,3); /* Decoder UH11 */
	//uIW->NAND4_H13B_C=BITRANGE(in,9,1); /* Quad NAND Gate input C of UH13Bp12 */
	//uIW->NOR_K12C_A=BITRANGE(in,8,1); /* NOR Gate input A of UK12Cp8 */
	uIW->D_K11=BITRANGE(in,7,3); /* Decoder UK11 */
	uIW->D_E6=BITRANGE(in,4,3); /* Decoder UE6 */
	uIW->D_D2D3=BITRANGE(in,0,4); /* Decoders UD2(bit3=0) (low nibble out) and UD3(bit3=1) (high byte out) */
	
	
}

void do_read_sources(cpu_state_t *st, uIW_trace_t *t) {
	byte_t v,e;
	for(int i=0; i<7; i++) {
		switch(i) {
			case 0: v=t->D2_Out; break;
			case 1: v=t->D3_Out; break;
			case 2: v=t->E6_Out; break;
			case 3: v=t->K11_Out; break;
			case 4: v=t->H11_Out; break;
			case 5: v=t->E7_Out; break;
		}
		for(int j=0; j<8; j++) {
			e= (i<<4) | (j<<1) | ( (v&(1<<j))? 1:0);
			switch(e) {
				case D_D2_0_READ_DL:
					st->Bus.iD= st->Reg.DL;
					deroach("Read 0x%02x from Data Latch to iD-Bus\n", st->Bus.iD);
					break;
				case D_D2_1_READ_REG: { // This needs an enable to toggle between IL and RIR for high nibble
					nibble_t rIL= st->Reg.ILR;
					nibble_t rREG= st->Reg.RIR&0x0f;
					byte_t raddr= st->Reg.RIR; //(rILR<<4) | (rREG&0x0f) ;
					st->Bus.iD= st->Reg.RF[raddr];
					deroach("Read 0x%02x from Register File address 0x%02x to iD-Bus\n",
						st->Bus.iD, raddr);
					}; break;
				case D_D2_2_READ_BUS_SYS_ADDR_MSB:
					st->Bus.iD= (st->Bus.Sys.ADDR & 0xff00)>>8;
					deroach("Read 0x%02x from MSB of system Address Bus to iD-Bus\n", st->Bus.iD);
					break;
				case D_D2_3_READ_BUS_SYS_ADDR_LSB: break;
					st->Bus.iD= (st->Bus.Sys.ADDR & 0x00ff)>>0;
					deroach("Read 0x%02x from MSB of system Address Bus to iD-Bus\n", st->Bus.iD);
					break;
				case D_D3_2_READ_BUS_SYS_DATA:
					//st->Bus.iD=st->Reg.SDBRL;
					st->Bus.iD=sysbus_read_data(st->Bus.Sys.ADDR);
					st->Bus.iD=0x01; // Force NOP
					st->Bus.iD=inject; // Force BL
					deroach("Read 0x%02x from External Data Bus to iD-Bus\n", st->Bus.iD);
					break;
				case D_D3_3_READ_ILR:
					st->Bus.iD=st->Reg.ILR;
					deroach("Read 0x%02x from Interrupt Level Register to iD-Bus\n", st->Bus.iD);
					break;
				case D_D3_4_READ_DIPSW_NIB_HIGH:
					st->Bus.iD= (~st->IO.DIPSW>>4)&0x0f;
					deroach("Read 0x%01x from high nibble of Dip Switches to iD-Bus\n", st->Bus.iD);
					break;
				case D_D3_5_READ_uCDATA:
					st->Bus.iD= ~t->uIW.DATA_;
					deroach("Read 0x%02x of uCDATA to iD-Bus\n",st->Bus.iD);
					break;
				case D_H11_1_READ_RR:
					st->Bus.R= st->Reg.RR;
					deroach("Read 0x%02x from Result Register to R-Bus\n", st->Bus.R);
					break;
				case D_H11_6_READ_MAPROM:
					st->Bus.F= maprom[st->Bus.iD];
					st->ALU.OE_=1; 
					deroach("Read 0x%02x from MAPROM address 0x%02x to F-Bus\n", st->Bus.F, st->Bus.iD);
					break;
				case D_H11_7_READ_ALU_RESULT:
					st->ALU.OE_=0;
					deroach("Reading ALU Y outputs to F-Bus enabled\n");
					break; 
				default: break;

			}
		}
	}
}


void do_write_dests(cpu_state_t *st, uIW_trace_t *t) {
	byte_t v,e;


	/* Set/Clear default as needed first: */
	st->Seq.RE_=1;

	for(int i=0; i<7; i++) {
		switch(i) {
			case 0: v=t->D2_Out; break;
			case 1: v=t->D3_Out; break;
			case 2: v=t->E6_Out; break;
			case 3: v=t->K11_Out; break;
			case 4: v=t->H11_Out; break;
			case 5: v=t->E7_Out; break;
		}
		//deroach("WRITE %0x: ",i);
		for(int j=0; j<8; j++) {
			e= (i<<4) | (j<<1) | ( (v&(1<<j))? 1:0);
			//deroach(" 0x%2x",e);
			switch(e) {
				case D_E6_1_WRITE_RR:
					st->Reg.RR=st->Bus.F;
					deroach("Wrote 0x%02x to Result Register (RR)\n", st->Reg.RR);
					break;
				case D_E6_2_WRITE_RIR:
					st->Reg.RIR=st->Bus.F;
					deroach("Register 0x%02x selected(RIR)\n", st->Reg.RIR);
					break;
				case D_E6_4_WRITE_PTBAR:
					st->Reg.PTBAR=st->Bus.F;
					deroach("Page Table Base Address set to 0x%02x\n", st->Reg.PTBAR);
					break;
				case D_E6_6_WRITE_SEQ_AR:
					st->Seq.RiS0= (st->Bus.F&0x0f)>>0;
					st->Seq.RiS1= (st->Bus.F&0xf0)>>4;
					st->Seq.RE_=0; // Cheat for now, but just this would be the 'proper' way.
					deroach("Wrote Seq{1,0} AR=0x%01x%01x\n", st->Seq.RiS1, st->Seq.RiS0);
					//deroach("Sequencer AR latching enabled\n");
					break;
				case D_K11_3_F11_ENABLE: {
					bit_t D=t->uIW.B&0x1;
					octal_t A=(t->uIW.B&0xe)>>1;
					st->Reg.LUF11= ( st->Reg.LUF11 & (~(1<<A)&0xff) ) | (D<<A);
					deroach("UF11 latched bit %0x=%0x, now contains 0x%02x\n", A, D, st->Reg.LUF11);
					}; break;
				case D_K11_4_WRITE_RF: // Write Register File indexed by RIR
					st->Reg.RF[st->Reg.RIR]= st->Bus.R;
					deroach("Wrote 0x%02x to Register File address 0x%02x\n",
							st->Reg.RF[st->Reg.RIR], st->Reg.RIR);
					break;
				case D_K11_7_WRITE_BUS_SYS_DATA:
					st->Reg.SDBOR=st->Bus.R;
					deroach("Wrote 0x%02x to System Data Bus Output Register (SDBOR)\n", st->Reg.SDBOR);
					break;
				case D_H11_3_WRITE_ALS_MSB:
					st->Reg.ALS= (st->Reg.ALS&0x00ff) | (st->Bus.iD<<8);
					deroach("Wrote 0x%02x to MSB of Staging Address Latch, now holds 0x%04x (ALS)\n",
						st->Bus.iD, st->Reg.ALS);
					break;
				case D_E7_2_WRITE_SR:
					deroach("This would write the Status Register if we had it setup\n");
					break;

				default: break;

			}
		}
		//deroach("\n");
	}
}

void uIW_trace_run_Seqs(cpu_state_t *st, uIW_trace_t *t ) {
	st->Seq.Cn=1; // Always increment (unless we find logic to disable)

	st->Seq.S0S= (~t->uIW.S0S)&0x3;
	st->Seq.S1S= (~t->uIW.S1S)&0x3;
	st->Seq.S2S= (~t->uIW.S2S)&0x3;
	printf("S0S=%0x  S1S=%0x  S2S=%0x\n",st->Seq.S0S, st->Seq.S1S, st->Seq.S2S);

	st->Seq.FE_= t->uIW.FE_&0x1;
	st->Seq.PUP= t->uIW.PUP&0x1;

	st->Seq.DiS0= (t->uIW.uADDR&0x00f)>>0;
	st->Seq.DiS1= (t->uIW.uADDR&0x0f0)>>4;
	st->Seq.DiS2= (t->uIW.uADDR&0x700)>>8;
	
	//st->Seq.RiS0= (st->Bus.F&0x0f)>>0;
	//st->Seq.RiS1= (st->Bus.F&0xf0)>>4;

	// Force for now until we find logic connectoins for these
	st->Seq.OE_= 0;
	st->Seq.ZERO_= 1;

	// Try some logic on for size
	if(!t->uIW.CASE_) {
		printf("Let's try a conditoinal!\n");
		printf("OR1S0=ALU.FZ=%0x  ",st->ALU.FZ);
		st->Seq.ORiS0= t->uIW.CASE_?0x0:( (st->ALU.FZ <<1) ) ;
		printf("Seq.ORiS0=%0x\n",st->Seq.ORiS0);
	} else { st->Seq.ORiS0=0; }

	st->Seq.cl.clk=CLK_LO;
	do{
		am2909_update(&st->dev.Seq0);
		am2909_update(&st->dev.Seq1);
		am2911_update(&st->dev.Seq2);
		clock_advance(&st->Seq.cl);
	} while(st->Seq.cl.clk);

	t->uADDR_Next= ((st->Seq.YS2&0x7)<<8) | ((st->Seq.YS1&0xf)<<4) | ((st->Seq.YS0&0xf)<<0);
}

void uIW_trace_run_ALUs(cpu_state_t *st, uIW_trace_t *t ) {
	st->ALU.FZ=1; // Pull FZ high at beginning of cycle, OC outputs will pull low if not zero
	st->ALU.I876= ((octal_t)t->uIW.I876)&0x7;
	st->ALU.I543= ((octal_t)t->uIW.I543)&0x7;
	st->ALU.I210= ((octal_t)t->uIW.I210)&0x7;
	st->ALU.ADDR_A= t->uIW.A&0xf;
	st->ALU.ADDR_B= t->uIW.B&0xf;
	st->ALU.Dlow=  (st->Bus.iD&0x0f)>>0;
	st->ALU.Dhigh= (st->Bus.iD&0xf0)>>4;
	st->ALU.Cin= (t->S_Carry==3?0:t->S_Carry==2?st->ALU.Cout:t->S_Carry?1:0);

	if(t->S_Shift&0x4) {
		st->Shifter.UpLine= (t->S_Shift==7?st->ALU.Cout:t->S_Shift==6?st->ALU.F7:t->S_Shift==5?0:st->ALU.RAM7);
		st->ALU.Q0=st->Shifter.UpLine;
	} else {
		st->Shifter.DownLine= (t->S_Shift==3?st->ALU.Cout:t->S_Shift==2?st->ALU.Q0:t->S_Shift?0:st->ALU.F7);
		st->ALU.RAM7=st->Shifter.DownLine;
	}

	st->ALU.cl.clk=CLK_LH;
	do{
		am2901_update(&st->dev.ALU0);
		am2901_update(&st->dev.ALU1);
		clock_advance(&st->ALU.cl);
	} while(st->ALU.cl.clk!=CLK_LH);

	am2901_print_state(&st->dev.ALU0);
	am2901_print_state(&st->dev.ALU1);

	if(!st->ALU.OE_) {
		st->Bus.F= (((st->ALU.Fhigh&0x0f)<<4)&0xf0) | ((st->ALU.Flow<<0)&0x0f);
		deroach("Read ALU Y=0x%02x to F-Bus\n", st->Bus.F);
	}
}


void trace_uIW(cpu_state_t *cpu_st, uIW_trace_t *t, uint16_t addr, uint64_t in) {
	t->uADDR=addr;
	parse_uIW(&(t->uIW), in);

	/* Fill in S1S from logic on S2S and S1S1_OVR_ signals */
	/* S1S0 = S2S0, S1S1 = S2S1 -> INV -> NAND S1S1_OVR_ */
	/* S1S =    NAND( INV(S2S1),                 S1S1_OVR_<<1       ) OR     S1S0 */
	t->uIW.S1S= ( (~( ((~t->uIW.S2S)&0x2) & (t->uIW.S1S1_OVR_<<1) ))&0x2) | ((t->uIW.S2S)&0x1) ;

	/* Assemble complete sequencer operations for each sequencer */
	/* S0&S2 S{01} -> (NAND INT_), FE_ -> (NAND INT_) -> INV (cancels), PUP has no NAND */
	t->Seq0Op=  ( ((~t->uIW.S0S)&0x3)<<2) | (t->uIW.FE_<<1) | (t->uIW.PUP<<0) ;
	t->Seq1Op=  ( ((~t->uIW.S1S)&0x3)<<2) | (t->uIW.FE_<<1) | (t->uIW.PUP<<0) ;
	t->Seq2Op=  ( ((~t->uIW.S2S)&0x3)<<2) | (t->uIW.FE_<<1) | (t->uIW.PUP<<0) ;



	/* Decode decoder codes (Active LO) */
	t->D2_Out= ~(  ( t->uIW.D_D2D3&0x8)? 0 : 1<<((t->uIW.D_D2D3)&0x3) )&0xf;
	t->D3_Out= ~( (t->uIW.D_D2D3&0x8)? 1<<((t->uIW.D_D2D3)&0x7) : 0 )&0x3f;
	t->E6_Out= ~( 1<<(t->uIW.D_E6) )&0xff;
	t->K11_Out= ~( 1<<(t->uIW.D_K11) )&0xff;
	t->H11_Out= ~( 1<<(t->uIW.D_H11) )&0xff;
	t->E7_Out= ~( 1<<(t->uIW.D_E7) )&0x0f;

	/* UF6 - Carry Control *
	 * Select from 
	 * Output enables driven by ?
	 * Carry control input connections:
	 *                         SHCS=0    SHCS=1     SHCS=2    SHCS=3
	 *     ??=0: (Za Enabled)  I0a=?     I1a=?      I2a=?     I3a=?
	 *     ??=1: (Zb Enabled)  I0b=?     I1b=?      I2b=?     I3b=?
	 *
	 * Carry control output connections:
	 *  Za -> ALU0.Cn (UF10)
	 *
	 *  0,0 -> ? Zero?
	 *  0,1 -> ? One?
	 *  0,2 -> ? Carry?
	 *  0,3 -> ? ??
	 */
	t->S_Carry=t->uIW.SHCS;


	/* UH6 - Shift Control *
	 * Select from 
	 * Output enables driven by ALU.I7 -> OEa_, -> INV -> OEb_
	 * ALU.I7=0 -> Shift Down, ALU.I7=1 Shift Up
	 * Shifter input connections:
	 *                         SHCS=0     SHCS=1     SHCS=2     SHCS=3
	 * ALU.I7=0: (Za Enabled)  I0a=S(2b)  I1a=?(1b)  I2a=Q0     I3a=C
	 * ALU.I7=1: (Zb Enabled)  I0b=RAM7   I1b=?(1a)  I2b=S(0a)  I3b=
	 *
	 * Shifter output connections:
	 * Za -> RAM7[ALU1.RAM3] (UF11), UJ10.I5
	 * Zb -> Q0[ALU0.Q0] (UF10), UJ10.I7
	 *
	 * 0,0:    S->RAM7  SRA (Sign extend S->MSB)
	 * 0,1:    ?->RAM7  SRL? (Zero?)
	 * 0,2:   Q0->RAM7  (SRA/RRR) Half-word (Q0 of High byte shifts into MSB of Low byte)
	 * 0,3:    C->RAM7  RRR (Rotate carry into MSB)
	 *
	 * 1,0: RAM7->Q0    RLR/SLR Half-word? (RAM7->Q0)
	 * 1,1:    ?->Q0    SLR? (Zero?)
	 * 1,2:    S->Q0    ?
	 * 1,3:    ?->Q0    (C?) RLR?
	 *
	 *
	 * See microcode @ 0x0d2-0x0e2 for 16 bit shift up through Q
	 */
	t->S_Shift=(t->uIW.I876&0x2?4:0)|t->uIW.SHCS;

	t->R= cpu_st->Bus.R;
	t->F= cpu_st->Bus.F;
	t->iD=cpu_st->Bus.iD;

	deroach("\nRunning READS:\n");
	do_read_sources(cpu_st, t);
	t->R= cpu_st->Bus.R;
	t->F= cpu_st->Bus.F;
	t->iD=cpu_st->Bus.iD;

	deroach("\nRunning ALUs:");
	uIW_trace_run_ALUs(cpu_st, t);
	t->R= cpu_st->Bus.R;
	t->F= cpu_st->Bus.F;
	t->iD=cpu_st->Bus.iD;

	deroach("\nRunning SEQs:\n");
	uIW_trace_run_Seqs(cpu_st, t);

	deroach("\nRunning WRITES:\n");
	do_write_dests(cpu_st, t);
	t->R= cpu_st->Bus.R;
	t->F= cpu_st->Bus.F;
	t->iD=cpu_st->Bus.iD;
	

	t->RIR=cpu_st->Reg.RIR;
	deroach("\nCycle complete, here's your trace:\n");
}

void init_ALUs(cpu_state_t *st) {
	st->ALU.I876=0;
	st->ALU.I543=0;
	st->ALU.I210=0;
	st->ALU.ADDR_A=0;
	st->ALU.ADDR_B=0;
	st->ALU.Dlow=0;
	st->ALU.Dhigh=0;

	st->ALU.Cin=0;
	st->ALU.Chalf=0;
	st->ALU.Cout=0;

	st->ALU.Q0=0;
	st->ALU.Q3=0;
	st->ALU.RAM0Q7=0;
	st->ALU.RAM3=0;
	st->ALU.RAM7=0;


	st->ALU.P_0=0;
	st->ALU.G_0=0;
	st->ALU.P_1=0;
	st->ALU.G_1=0;


	st->ALU.FZ=1;
	st->ALU.F3=0;
	st->ALU.F7=0;
	st->ALU.nibbleOVF=0;
	st->ALU.OVF=0;

	st->ALU.Flow=0;
	st->ALU.Fhigh=0;

	st->ALU.OE_=1;

	am2901_init(&st->dev.ALU0, "ALU0", &st->ALU.cl.clk,
		&st->ALU.I210, &st->ALU.I543, &st->ALU.I876,
		&st->ALU.RAM0Q7, &st->ALU.RAM3,
		&st->ALU.ADDR_A, &st->ALU.ADDR_B,
		&st->ALU.Dlow, &st->ALU.Cin,
		&st->ALU.P_0, &st->ALU.G_0,
		&st->ALU.Chalf, &st->ALU.nibbleOVF,
		&st->ALU.Q0, &st->ALU.Q3,
		&st->ALU.FZ, &st->ALU.F3,
		&st->ALU.Flow, &st->ALU.OE_);

	am2901_init(&st->dev.ALU1, "ALU1", &st->ALU.cl.clk,
		&st->ALU.I210, &st->ALU.I543, &st->ALU.I876,
		&st->ALU.RAM3, &st->ALU.RAM7,
		&st->ALU.ADDR_A, &st->ALU.ADDR_B,
		&st->ALU.Dhigh, &st->ALU.Chalf,
		&st->ALU.P_1, &st->ALU.G_1,
		&st->ALU.Cout, &st->ALU.OVF,
		&st->ALU.Q3, &st->ALU.RAM0Q7,
		&st->ALU.FZ, &st->ALU.F7,
		&st->ALU.Fhigh, &st->ALU.OE_);
}

void init_Seqs(cpu_state_t *st) {
	// Clear initial values
	st->Seq.ORiS0=0; st->Seq.ORiS1=0;
	st->Seq.DiS0=0; st->Seq.DiS1=0; st->Seq.DiS2=0;
	st->Seq.RiS0=0; st->Seq.RiS1=0;
	st->Seq.YS0=0; st->Seq.YS1=0; st->Seq.YS2=0;
	st->Seq.S0S=0; st->Seq.S1S=0; st->Seq.S2S=0;

	am2909_init(&st->dev.Seq0, "Seq0",
		&st->Seq.cl.clk, /* Clock state from clockline */
		&st->Seq.S0S, &st->Seq.FE_, &st->Seq.PUP, /* Select operation */
		&st->Seq.DiS0, /* Direct inputs */
		&st->Seq.RiS0, &st->Seq.RE_, /* AR inputs (tied to Di on am2911) (when RE_ is LO) inputs */
		&st->Seq.Cn, &st->Seq.C4, /* Incrementer carry in and carry out, Cn=1 increments uPC, Cn=0 repeats current op */
		&st->Seq.ORiS0, &st->Seq.ZERO_, &st->Seq.OE_, /* Outputs overrides: OE_=1->HiZ, ZERO_=0->Y=0, ORi=1->Yi=1 */
		&st->Seq.YS0 /* Outputs Yi of Y if not overridden by above */
	);

	am2909_init(&st->dev.Seq1, "Seq1",
		&st->Seq.cl.clk, /* Clock state from clockline */
		&st->Seq.S1S, &st->Seq.FE_, &st->Seq.PUP, /* Select operation */
		&st->Seq.DiS1, /* Direct inputs */
		&st->Seq.RiS1, &st->Seq.RE_, /* AR inputs (tied to Di on am2911) (when RE_ is LO) inputs */
		&st->Seq.C4, &st->Seq.C8, /* Incrementer carry in and carry out, Cn=1 increments uPC, Cn=0 repeats current op */
		&st->Seq.ORiS1, &st->Seq.ZERO_, &st->Seq.OE_, /* Outputs overrides: OE_=1->HiZ, ZERO_=0->Y=0, ORi=1->Yi=1 */
		&st->Seq.YS1 /* Outputs Yi of Y if not overridden by above */
	);

	am2911_init( &st->dev.Seq2, "Seq2",
		&st->Seq.cl.clk, /* Clock state from clockline */
		&st->Seq.S2S, &st->Seq.FE_, &st->Seq.PUP, /* Select operation */
		&st->Seq.DiS2, &st->Seq.RE_, /* Di/AR inputs (Load AR when RE_ is LO) */
		&st->Seq.C8, &st->Seq.Co, /* Incrementer carry in and carry out, Cn=1 increments uPC, Cn=0 repeats current op */
		&st->Seq.ZERO_, &st->Seq.OE_, /* Outputs overrides: OE_=1->HiZ, ZERO_=0->Y=0 */
		&st->Seq.YS2 /* Outputs Yi of Y if not overridden by above */
	);
}

void print_decoder_values(enum decoder_enum d, uint8_t v) {
	octal_t p;
	char *s;
	for(int i=0; i<8; i++) {
		p=(v>>i)&0x1;
		s=decoded_sig[d][i][p];
		printf("%s%s%s",(!p&&*s)?"_":"",s,*s? p?"\n":"_\n":"");
	}
}

void print_uIW_trace(uIW_trace_t *t) {
	print_comments(t->uADDR);
	printf("Current Address: 0x%03x  Previous Address: 0x%03x  Next Address: 0x%03x\n", t->uADDR, t->uADDR_Prev, t->uADDR_Next);
	printf("uCData: D/uADDR=0x%03x (DATA_=0x%02x)", t->uIW.uADDR, (~t->uIW.DATA_)&0xff);
	printf(" Shifter: %s / Carry Select: %s (SHCS=%0x)\n",shifter_ops[t->S_Shift], carry_ops[t->S_Carry], t->uIW.SHCS);
	printf("Conditoinal: %s\n",t->uIW.CASE_?"No":"Yes");
	printf("iD-Bus: 0x%02x\n", t->iD);
	printf("ALUs: A=0x%01x B=0x%01x RS=%s %s -> %s\n",
		t->uIW.A,
		t->uIW.B,
		am2901_source_operand_mnemonics[t->uIW.I210],
		am2901_function_symbol[t->uIW.I543],
		am2901_destination_mnemonics[t->uIW.I876]
	);
	printf("F-Bus: 0x%02x\n",t->F);
	printf("R-Bus: 0x%02x\n",t->R);
	printf("Seqs: S0: %s, S1: %s, S2: %s\n",
		am2909_ops[t->Seq0Op][3],
		am2909_ops[t->Seq1Op][3],
		am2909_ops[t->Seq2Op][3]
	);
	printf("Selected register: 0x%01x\n",t->RIR);
	printf("\nDecoders E7:0x%02x H11:0x%02x K11:0x%02x E6:0x%02x D3:0x%02x D2:0x%02x\n",
		t->E7_Out, t->H11_Out, t->K11_Out, t->E6_Out, t->D3_Out, t->D2_Out);

	print_decoder_values(D_D2,  t->D2_Out);
	print_decoder_values(D_D3,  t->D3_Out);
	print_decoder_values(D_E6,  t->E6_Out);
	print_decoder_values(D_K11, t->K11_Out);
	print_decoder_values(D_H11, t->H11_Out);
	print_decoder_values(D_E7,  t->E7_Out);
	printf("\n");
		

}

void init_cpu_state(cpu_state_t *st) {
	clock_init(&st->Seq.cl,"Seq Clock", CLK_LO);
	init_Seqs(st);
	clock_init(&st->ALU.cl,"ALU Clock", CLK_LO);
	init_ALUs(st);
	st->Bus.iD=0;
	st->Bus.F=0;
	st->Reg.LUF11=0;
}


int main(int argc, char **argv) {
	int r;
	uint16_t tmp;
	word_t uA, uAp;
	uint64_t salad;
	char binstr[100];
	cpu_state_t cpu_st;
	uIW_trace_t trace[ROMSIZE];
	//Byte to force into sys data bus register
	if(argc == 2) { inject=atoi(argv[1]); }
	else { inject=0x01; }

	if( (r=read_roms()) == 0 ) {
		merge_roms();

		init_cpu_state(&cpu_st);
		uA=0x0; uAp=0;
		for(int i=0; i<ROMSIZE; i++) {
			//printf("\n%#06x: %#06x",i,allrom[0][i]);
			//byte_bits_to_binary_string_grouped(binstr, allrom[0][i], NUMROMS*8, 1);
			//printf("   %s",binstr);
			//int64_bits_to_binary_string_grouped(binstr, iws[i], NUMROMS*8,4);
			int64_bits_to_binary_string_fields(binstr, iws[uA], NUMROMS*8,
				"\x1\1\1\x2\x4\x4\x3\x3\x3\x1\x2\x2\x2\x3\x4\x4\x1\x2\x3\x3\x3\x4");
			printf("Cycle:%04u 0x%#03x: 0x%016"PRIx64" %s\n",i,uA,iws[uA],binstr);
			//trace_uIW(&cpu_st, &trace[i],i,iws[i]);
			trace[i].uADDR_Prev=uAp;
			trace_uIW(&cpu_st, &trace[i],uA,iws[uA]);
			print_uIW_trace(&trace[i]);
			uAp=uA;
			uA=trace[i].uADDR_Next;
			//uA=i;
		}
	} else {
		printf("Could not read ROMS!");
	}
}
