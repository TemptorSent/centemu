#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "../common/logic-common.h"
#include "../common/clockline.h"
#include "../common/ginsumatic.h"
#include "../components/am2901.h"
#include "../components/am2909.h"
#include "../common/rom-common.h"


#include "./include/uCode_rom.h"
#include "./include/maprom_rom.h"
#include "./include/bootstrap_rom.h"

#include "./include/comments_generated.h"

// Inject bytes into system databus reads
char **inject;
byte_t inject_size;
byte_t inject_pos;

/* Microcode ROMs */
//#define NUMROMS 7
#define ROMSIZE 2048

//static uint8_t allrom[NUMROMS][ROMSIZE];
//static uint8_t mergedrom[ROMSIZE][NUMROMS];
//static uint64_t iws[ROMSIZE];
/*
static char *ROM_files[NUMROMS] = {
	"./ROMs/CPU_UM3_MWE3.11_M3.11.rom",	// MC<8:0>; uIW<55:48>
	"./ROMS/CPU_UJ3_MWJ3.11_F3.11.rom",	// MC<31:24>; uIW<47:40>
	"./ROMS/CPU_UE3_MWM3.11_E3.11.rom",	// MC<55:48>; uIW<39:32>
	"./ROMS/CPU_UF3_MWL3.11_D3.11.rom",	// MC<47:40>; uIW<31:24>
	"./ROMS/CPU_UK3_MWH3.11_C3.11.rom",	// MC<23:16>; uIW<23:16>
	"./ROMS/CPU_UL3_MWF3.11_B3.11.rom",	// MC<15:8>; uIW<15:8>
	"./ROMS/CPU_UH3_MWK3.11_A3.11.rom"	// MC<39:32>; uIW<7:0>
};
*/

/* ISA Entry point offset MAPROM */
//#define MAPROMSIZE 0x100
//uint8_t maprom[MAPROMSIZE];
//static char *MAPROM_file = "./ROMs/CPU_UB13_MAPROM_6309.rom";


/* Structure for storing our machine state */
typedef struct cpu_state_t {
	struct dev {
		am2901_device_t ALU0;
		am2901_device_t ALU1;
		am2909_device_t Seq0;
		am2909_device_t Seq1;
		am2911_device_t Seq2;
	} dev;

	struct Clock {
		clockline_t A;	 // From UH12A
		clockline_t B;	 // From UH12B
		clockline_t B0_; // Clock.B -> UJ6A ('LS04: 3-15ns)
		clockline_t B1_; // Clock.B -> UK10B ('LS00: 3-15ns)
		clockline_t B2_; // Clock.B -> UE8B ('LS02: ~10-15ns)
	} Clock;

	struct Shifter {

		bit_t DownLine, UpLine;
	} Shifter;

	struct ALU {
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
		word_t ADDR; /* Internal Address bus */
		byte_t DP; /* Internal Data bus (DataPath) */
		byte_t F; /* Primarily output of ALU result F, also used for MAPROM output */
		byte_t R; /* Result bus, latched output of Bus.F */
		struct Sys { /* System bus, to edge connector to backplane */
			/* Master reset for all devices on bus */
			bit_t MRST; /* Master ReSeT */
			/* System Address Bus Signals */
			word_t AB_; /* Translated System Address Bus Address, (AB0_-AB15_) */
			uint32_t ABR_; /* Real System Address Bus Address, ?18?19-bits (AB0_-AB10_,ABR11-ABR17) */
			bit_t APRE; /* Address PREempted: Higher priority memory device claims address; Parity disabled */
			/* System Data Bus Signals */
			byte_t DB_; /* System Data Bus (DB0-DB7)*/
			/* Memory/MMIO Cycle Signals */
			bit_t RDIN; /* ReaD INitiate: CPU - Begin memory/IO read cycle; Device - Read cycle during DMA */
			bit_t WTIN; /* WriTe INitiate: CPU - Begin memory/IO write cycle; Device - Write cycle during DMA */
			bit_t BUSY; /* Memory/IO device BUSY: CPU will wait for device before changing address lines */
			bit_t DRDY; /* Data ReaDY: Device read access complete, data on bus ready to be sampled */
			bit_t DSYN; /* SYNchronus Device flag from mem/IO; Ready response synced with CPU clock */
			/* Interrupt Signals */
			nibble_t CL_; /* Current IL (CL0_ - CL3_) */
			bit_t INTR; /* INTerrupt Request from device */
			nibble_t INR_; /* Requested IL (INR0_ -  INR3_) */
			/* DMA Signals */
			bit_t INPO, INPI; /* INterrupt Priority In/Out daisy chain: Establish priority for devices using same IL */
			bit_t IORQ; /* IO ReQuest from device for DMA */
			bit_t IACK; /* IO ACKnowledge from CPU */
			bit_t IDON; /* IO DONe: Asserted by device to release bus after IO */
			bit_t IOBY; /* IO not BusY: CPU response to IODN to signal device to deassert IODN */
			bit_t IOPO, IOPI; /* IO Priority In/Out daisy chain: Establish priority for devices requesting DMA */
		} Sys;
	} Bus;

	struct Reg {
		byte_t RIR; // Register Index Register (UC13)
		nibble_t ILR; // Interrupt Level Register (UC15, read w/UH14)
		byte_t SDBOR; // Databus Output Register (UA11/UA12)
		byte_t SDBRL; // Databus Receive Latch (UA11/UA12)
		byte_t NSWAPR; // DP Nibble Swap Register (UC11/UC12)
		byte_t FLR; // Flag Register - ALU flag storage (UJ9)
		byte_t CCR; // Condition Code Register - ALU flag storage (M12)
		byte_t RR; // Result Register (UC9)
		word_t WAR; // Working Address Register (UB2/UC2 / UB5/UC5)
		word_t MAR; // Memory Address Register (UB1/UC1 / UB6/UC6)
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
	nibble_t LA,LB;
	octal_t I876, I543, I210;
	bit_t CASE_;
	twobit_t S0S, S1S, S2S;
	bit_t PUP, FE_;
	word_t uADDR;
	byte_t DATA_;
	octal_t D_E6,D_E7,D_H11,D_K11;
	octal_t M_UJ10_S210, M_UK9_S210;
	twobit_t M_UJ11_S10, M_UJ12_S10, M_UJ13_S10, M_UK13_S10;
	bit_t M_UJ10_E_, M_UJ11_E_, M_UK9_E_;
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
	byte_t DP;
	byte_t F;
	byte_t R;
	nibble_t RIR;
	
} uIW_trace_t;


enum decoder_enum { D_D2, D_D3, D_E6, D_K11, D_H11, D_E7 };
enum decoder_outputs {
	D_D2_0_READ_NSWAPR=0x00,/* D_D2_0_WRITE_NSWAPR=0x01,*/
	D_D2_1_READ_REG=0x02,
	D_D2_2_READ_BUS_SYS_ADDR_MSB=0x04, D_D2_3_READ_BUS_SYS_ADDR_LSB=0x06,

	D_D3_0_READ_BUS_SYS_ADDR_PAGE=0x10, D_D3_1_READ_SW_LSN=0x12, D_D3_2_READ_BUS_SYS_DATA=0x14, D_D3_3_READ_ILR=0x16,
	D_D3_4_READ_SW_MSN=0x18, D_D3_5_READ_uCDATA=0x1a,

	D_E6_0_UNUSED=0x20, D_E6_1_WRITE_RR=0x22, D_E6_2_WRITE_RIR=0x24, D_E6_3_WRITE_D9=0x26,
	D_E6_4_WRITE_PTBAR=0x28, D_E6_5_WRITE_WAR2MAR=0x2a, D_E6_5_WAR_SRC_DATA=0x2b, D_E6_6_WRITE_SEQ_AR=0x2c, D_E6_7=0x2e,

	D_K11_0_WRITE_RF=0x30, D_K11_1_WrCLK_L13A_SET=0x32, D_K11_2=0x34, D_K11_3_WrCLK_F11_ENABLE=0x36,
	D_K11_4=0x38, D_K11_5=0x3a, D_K11_6_WrCLK_WAR_LSB=0x3c, D_K11_7_WrCLK_BUS_SYS_DATA=0x3e,

	D_H11_0=0x40, D_H11_1=0x42, D_H11_2=0x44, D_H11_3_WRITE_WAR_MSB=0x46,
	D_H11_4=0x48, D_H11_5=0x4a, D_H11_6_READ_MAPROM=0x4c, D_H11_6_READ_ALU_RESULT=0x4d, D_H11_7_WRITE_NSWAPR=0x4e,

	D_E7_0=0x50, D_E7_1_UE14_CLK_EN=0x52, D_E7_2_WRITE_FLR=0x54, D_E7_3=0x56,
	D_E7_4_UNUSED=0x58, D_E7_5_UNUSED=0x5a, D_E7_6_UNUSED=0x5c, D_E7_7_UNUSED=0x5e
};


static char *decoded_sig[6][8][2] = {
	{ // Decoder D2 -> latch UD4
		{"D2.0 READ NIBBLE SWAP REGISTER -> (DP-Bus)",""}, // C12p1
		{"D2.1 READ REGISTER FILE -> (DP-Bus)",""}, // D13p1
		{"D2.2 READ ADDRESS BUS MSB (A-Bus) -> (DP-Bus)",""}, // A6p7
		{"D2.3 READ ADDRESS BUS LSB (A-Bus)-> (DP-Bus)",""}, // A4p7
		{"",""},{"",""},
		{"",""},{"",""}
	},
	{ // Decoder D3 -> latch UD5
		{"D3.0 READ SYSTEM ADDRES BUS PAGE -> (DP-Bus)", ""}, // A9p7
		{"D3.1 READ DIP SWITCHES LEAST SIGNIFICAN NIBBLE -> (DP-Bus)",""}, // M7p19
		{"D3.2 READ SYSTEM DATA BUS (Bus.Sys.DATA) -> (DP-Bus)",""}, // A12p11
		{"D3.3 READ IL REGISTER -> (DP-Bus)",""}, // A8p7
		{"D3.4 READ DIP SWITCHES MOST SIGNIFICANT NIBBLE -> (DP-Bus)",""}, // M7p1
		{"D3.5 READ uC DATA CONSTANT (uIW) -> (DP-Bus)",""}, // M6p18
		{"",""},{"",""}

	},
	{ // Decoder E6 (from latch E5)
		{"E6.0 IDLE STATE",""},
		{"E6.1 WRITE RESULT REGISTER (R-Bus) <- (F-Bus)",""}, // C9p1
		{"E6.2 WRITE REGISTER INDEX SELECTION REGISTER <- (F-Bus)",""}, // C13p1
		{"E6.3 (D9 Enable?)",""}, // D9p1
		{"E6.4 WRITE PAGETABLE BASE ADDRESS REGISTER <- (F-Bus)",""},  // D11p1
		{"E6.5 WRITE WORKING ADDRES REGISTER TO MEMORY ADDRESS REGISTER <- (A-Bus)",
			"E6.5 WORKING ADDRESS REGISTER SOURCE = RESULT <- (R-Bus)"}, // C4p1
		{"E6.6 WRITE DATA TO SEQUENCERS ADDRESS REGISTER <- (F-Bus)",""}, // AM2909p1 (RE_)
		{"E6.7 (Load Condition Code Reg M12?)",""} // M12p1
	},
	{ // Decoder K11 & (K12C(NOR)/H13B(NAND4)) - Write Register File(0) and Clock output select (1-7)
	  	// WRITE REGFILE NAND4[UH13B]( K11.A2[B] (NOR[UK12C](K11.A0,K11.A1)[C,D] ) -> UD14.WE_/UD15.WE_
		// WRITE_RF logic tied to input signals of K11
		{"K11.0 Write Register File <- (R-Bus)",""},
		// All output signals clocked through inverting input to inverting outputs (Idle HI)
		{"K11.1 (Unknown Clock Selected)",""},
		{"K11.2 (M13 Gate?)",""},
		{"K11.3 (F11 Enable)",""},
		{"K11.4 (Unknown Clock Selected)",""},
		{"K11.5 ENABLE CLOCK TO WRITE WORKING ADDRESS REGISER LSB (Clock.WrR_WAR)",""},
		{"K11.6 ENABLE CLOCK TO READ SYSTEM DATA BUS RECEIVE LATCH (Clock.RdL_SysDB)",""},
		{"K11.5 ENABLE CLOCK TO WRITE SYSTEM DATA BUS OUTPUT REGISTER <- (R-Bus) (Clock.WrR_SysDB)",""}
	},
	{ // Decoder H11
		{"H11.0",""},
		{"H11.1",""},
		{"H11.2",""},
		{"H11.3 WRITE WORKING ADDRESS REGISTER MSB <- (A-Bus/R-Bus)",""},
		{"H11.4",""},
		{"H11.5",""},
		{"H11.6 READ MAPPING PROM -> (F-Bus)","H11.6 READ ALU RESULT -> (F-Bus)"},
		{"H11.7 WRITE NIBBLE SWAP REGISTER <- (DP-Bus) (NSWAPR)",""}
	},
	{ // Decoder E7
		{"E7.0",""},
		{"E7.1 (E14 Clock enable?)",""},
		{"E7.2 WRITE FLAG REGISTER",""},
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



/* E_: uADDR<5>; S{2:0}=uADDR<8?:6> */
static char *UJ10_Sources[8] = {
	"UJ10.I0 UM12.Q3 (=UK13.I1a, UK10C.[AB])",
	"UJ10.I1 UK10C.Y ( =INV(UJ10.I0) )",
	"UJ10.I2 Link Flag? (Carry)",
	"UJ10.I3",
	"UJ10.I4 R-Bus<3> or R-Bus<7>",
	"UJ10.I5 ALU.RAM7 (Shifter DownLine)",
	"UJ10.I6 ALU.RAM0",
	"UJ10.I7 ALU.Q0 (Shifter UpLine)",
};

/* E_: uADDR<2>; S<1:0>=uADDR<4:3>,S2=Flag_M(UJ9.Q2) */
static char *UJ11_Sources[8] = {
	"UJ11.I0 (=I4?)",
	"UJ11.I1",
	"UJ11.I2 UM12p7?",
	"UJ11.I3",
	"UJ11.I4 (=I0?)",
	"UJ11.I5",
	"UJ11.I6", 
	"UJ11.I7",
};

/* Ea_= ???, Eb_= ???; S<1:0>=uADDR<1:0> */
static char *UJ12a_Sources[4] = {
	"UJ12.I0a",
	"UJ12.I1a",
	"UJ12.I2a R-Bus<1> or R-Bus<5>",
	"UJ13.I3a Result F7 =? 1 (Sign: FLAG_M)",
};

static char *UJ12b_Sources[4] = {
	"UJ12.I0b",
	"UJ12.I1a Result F =? 0 (Zero; FLAG_V)",
	"UJ12.I2b",
	"UJ12.I3b UK10p11",
};

/* Ea_=Eb_= CASE_; S<1:0>=uADDR<5:4> */
static char *UJ13a_Sources[4] = {
	"UJ13.I0a",
	"UJ13.I1a",
	"UJ13.I2a",
	"UJ13.I3a"
};
static char *UJ13b_Sources[4] = {
	"UJ13.I0a Result F =? 0 (Zero; FLAG_V)",
	"UJ13.I1b",
	"UJ13.I2b UB12p1",
	"UJ13.I3b",
};

/* E_: uIW.LA<0>; S{0,1,2}=uADDR<0:2> */
static char *UK9_Sources[8] = {
	"UK9.I0",
	"UK9.I1",
	"UK9.I2",
	"UK9.I3",
	"UK9.I4",
	"UK9.I5",
	"UK9.I6",
	"UK9.I7",
};


/* Ea_=Eb_= CASE_; S<1:0>=uADDR<7:6> */
static char *UK13a_Sources[4] = {
	"UK13.I0a UM12.Q3 (=UJ10.I0, UK10C.[AB])",
	"UK13.I1a",
	"UK13.I2a",
	"UK13.I3a"
};
static char *UK13b_Sources[4] = {
	"UK13.I0a",
	"UK13.I1b",
	"UK13.I2b UK10D.Y",
	"UK13.I3b",
};

/*

int read_roms() {
	int r;
	for(int i=0; i<NUMROMS; i++) {
		r=rom_read_file(ROM_files[i],ROMSIZE,allrom[i]);
		if(r) { printf("Error %x reading file %s\n", r, ROM_files[i]); return(r); }
	}
	r=rom_read_file(MAPROM_file, MAPROMSIZE, maprom);
	return(r);
}

int merge_roms() {
	uint64_t iw;
	for(int i=0; i<ROMSIZE; i++) {
		for(int j=0; j<NUMROMS; j++) {
			mergedrom[i][j]=allrom[j][i];
		}
		iw=concat_bytes_64(NUMROMS,mergedrom[i]);
		iws[i]=iw;
	}
	return(ROMSIZE);
}
*/
void print_comments(word_t addr) {
	comment_t *cmt=comments;
	while(cmt->comment) {
		if(cmt->addr == addr) { printf("// %s\n",cmt->comment);}
		cmt++;
	}
}

byte_t sysbus_read_data(word_t address) {
	char c,*p;
	byte_t b=0;
	if(inject_size) {

		for(int i=0; i<2; i++) {
			if((c=*(inject[inject_pos]+i))) { b|=hexchar_to_nibble(c)<<(4*(1-i)); }
			else { break; }
		}
		inject_pos=(inject_pos+1)%inject_size;
		return(b);
	}
	// Bootstrap ROM
	if(address >= 0xfc00 && address < 0xfe00) {
		return(bootstrap_rom[address-0xfc00]);
	}
	return(0);
}

void parse_uIW(uIW_t *uIW, uint64_t in) {

	uIW->S1S1_OVR_=BITRANGE(in,54,1); /* Override bit1=1 of S1S when LO, otherwise S1S=S2S */
	uIW->SHCS=BITRANGE(in,51,2); /* Shifter/Carry Control Select */
	uIW->LA=BITRANGE(in,47,4); /* ALU A Address/Logic Bus (Source) */
	uIW->LB=BITRANGE(in,43,4); /* ALU B Address/Logic Bus (Source/Dest) */
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
	
	/* Fill in S1S from logic on S2S and S1S1_OVR_ signals */
	/* S1S0 = S2S0, S1S1 = S2S1 -> INV -> NAND S1S1_OVR_ */
	/* S1S =    NAND( INV(S2S1),                 S1S1_OVR_<<1       ) OR     S1S0 */
	uIW->S1S= ( (~( ((~uIW->S2S)&0x2) & (uIW->S1S1_OVR_<<1) ))&0x2) | ((uIW->S2S)&0x1) ;


	/* Conditional MUX controls mostly overlap with uADDR range */
	uIW->M_UJ10_S210=BITRANGE(in,16+6,3); /* MUX UJ10 Source Select */
	uIW->M_UJ10_E_=BITRANGE(in,16+5,1); /* MUX UJ10 Enable */
	uIW->M_UJ11_S10=BITRANGE(in,16+3,2); /* MUX UJ11 Source Select S0/S1 - Dynamic: S2=Flag_M(UJ9.Q2) */
	uIW->M_UJ11_E_=BITRANGE(in,16+2,1); /* MUX UJ11 Enable */
	uIW->M_UJ12_S10=BITRANGE(in,16+0,2); /* MUX UJ12 Source Select */
	uIW->M_UJ13_S10=BITRANGE(in,16+4,2); /* MUX UJ13 Source Select */
	uIW->M_UK9_S210=BITRANGE(in,16+0,3); /* MUX K9 Source Select */
	uIW->M_UK9_E_=BITRANGE(in,47+0,1); /* MUX UK9 Enable: =LA<0> */
	uIW->M_UK13_S10=BITRANGE(in,16+6,2); /* MUX UK13 Source Select */
	
}

void do_conditionals(cpu_state_t *st, uIW_trace_t *t) {
	byte_t tb;
	nibble_t tn;

	/* MUX UJ10 */
	if(!t->uIW.M_UJ10_E_) {
		tb=(t->uIW.M_UJ10_S210);
		deroach("UJ10 Enabled, S=%0x: %s\n",tb,UJ10_Sources[tb]);
	}

	/* MUX UJ11 */
	if(!t->uIW.M_UJ11_E_) {
		tb=(t->uIW.M_UJ11_S10 | (st->Reg.FLR&0x4));
		deroach("UJ11 Enabled, S=%0x: %s\n",tb,UJ11_Sources[tb]);
	}

	/* MUX UJ12 Enables unknown */
	if(0) {
		tn=(!t->uIW.M_UJ12_S10);
		deroach("UJ12a Enabled, S=%0x: %s\n",tn,UJ12a_Sources[tn]);
	}
	if(0) {
		tn=(!t->uIW.M_UJ12_S10);
		deroach("UJ12b Enabled, S=%0x: %s\n",tn,UJ12b_Sources[tn]);
	}


	/* MUX UJ13 Enables common: CASE_ */
	if(!t->uIW.CASE_) {
		tn=(t->uIW.M_UJ13_S10);
		deroach("UJ13a Enabled, S=%0x: %s\n",tn,UJ13a_Sources[tn]);
		deroach("UJ13b Enabled, S=%0x: %s\n",tn,UJ13b_Sources[tn]);
	}


	/* MUX UK9 */
	if(!t->uIW.M_UK9_E_) {
		tb=(t->uIW.M_UK9_S210);
		deroach("UK9 Enabled, S=%0x: %s\n",tb,UK9_Sources[tb]);
	}


	/* MUX UK13 Enables common: CASE_ */
	if(!t->uIW.CASE_) {
		tn=(t->uIW.M_UK13_S10);
		deroach("UK13a Enabled, S=%0x: %s\n",tn,UK13a_Sources[tn]);
		deroach("UK13b Enabled, S=%0x: %s\n",tn,UK13b_Sources[tn]);
	}

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
				case D_D2_0_READ_NSWAPR:
					st->Bus.DP= st->Reg.NSWAPR;
					deroach("Read 0x%02x from DP Nibble Swap Register to DP-Bus\n", st->Bus.DP);
					break;
				case D_D2_1_READ_REG: {
					// This needs an enable to toggle between IL and RIR for high nibble
					nibble_t rIL= st->Reg.ILR;
					nibble_t rREG= st->Reg.RIR&0x0f;
					byte_t raddr= st->Reg.RIR; //(rILR<<4) | (rREG&0x0f) ;
					st->Bus.DP= st->Reg.RF[raddr];
					deroach("Read 0x%02x from Register File address 0x%02x to DP-Bus\n",
						st->Bus.DP, raddr);
					}; break;
				case D_D2_2_READ_BUS_SYS_ADDR_MSB:
					st->Bus.DP= (st->Bus.Sys.AB_ & 0xff00)>>8;
					deroach("Read 0x%02x from MSB of system Address Bus to DP-Bus\n", st->Bus.DP);
					break;
				case D_D2_3_READ_BUS_SYS_ADDR_LSB:
					st->Bus.DP= (st->Bus.Sys.AB_ & 0x00ff)>>0;
					deroach("Read 0x%02x from LSB of system Address Bus to DP-Bus\n", st->Bus.DP);
					break;
				case D_D3_0_READ_BUS_SYS_ADDR_PAGE:
					st->Bus.DP= (st->Bus.Sys.ABR_ & 0x0003f800)>>0;
					deroach("Read 0x%02x from PAGE of system Address Bus to DP-Bus\n", st->Bus.DP);
					break;
				case D_D3_1_READ_SW_LSN:
					st->Bus.DP= (~st->IO.DIPSW>>0)&0x0f;
					deroach("Read 0x%01x from low nibble of Dip Switches to DP-Bus\n", st->Bus.DP);
					break;
				case D_D3_2_READ_BUS_SYS_DATA:
					st->Bus.DP=sysbus_read_data(st->Bus.Sys.ABR_);
					deroach("Read 0x%02x from External Data Bus to DP-Bus\n", st->Bus.DP);
					break;
				case D_D3_3_READ_ILR:
					st->Bus.DP=st->Reg.ILR;
					deroach("Read 0x%02x from Interrupt Level Register to DP-Bus\n", st->Bus.DP);
					break;
				case D_D3_4_READ_SW_MSN:
					st->Bus.DP= (~st->IO.DIPSW>>4)&0x0f;
					deroach("Read 0x%01x from high nibble of Dip Switches to DP-Bus\n", st->Bus.DP);
					break;
				case D_D3_5_READ_uCDATA:
					st->Bus.DP= ~t->uIW.DATA_;
					deroach("Read 0x%02x of uCDATA to DP-Bus\n",st->Bus.DP);
					break;
				case D_H11_6_READ_MAPROM:
					st->Bus.F= maprom[st->Bus.DP];
					st->ALU.OE_=1; 
					deroach("Read 0x%02x from MAPROM address 0x%02x to F-Bus\n", st->Bus.F, st->Bus.DP);
					break;
				case D_H11_6_READ_ALU_RESULT:
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
					st->Bus.R=st->Bus.F;
					deroach("Wrote 0x%02x to Result Register (RR)\n", st->Reg.RR);
					break;
				case D_E6_2_WRITE_RIR:
					st->Reg.RIR=st->Bus.F;
					deroach("Register 0x%02x selected(RIR)\n", st->Reg.RIR);
					break;
				case D_E6_3_WRITE_D9: break;
				case D_E6_4_WRITE_PTBAR:
					st->Reg.PTBAR=st->Bus.F;
					deroach("Page Table Base Address set to 0x%02x\n", st->Reg.PTBAR);
					break;
				case D_E6_5_WRITE_WAR2MAR:
					deroach("Write Working Address Register to Memory Address Register (WAR2MAR)\n");
					break;
				case D_E6_5_WAR_SRC_DATA:
					deroach("WAR input source set to Data Path\n");
					break;
				case D_E6_6_WRITE_SEQ_AR:
					st->Seq.RiS0= (st->Bus.F&0x0f)>>0;
					st->Seq.RiS1= (st->Bus.F&0xf0)>>4;
					st->Seq.RE_=0; // Cheat for now, but just this would be the 'proper' way.
					deroach("Wrote Seq{1,0} AR=0x%01x%01x\n", st->Seq.RiS1, st->Seq.RiS0);
					//deroach("Sequencer AR latching enabled\n");
					break;
				case D_K11_0_WRITE_RF: // Write Register File indexed by RIR
					st->Reg.RF[st->Reg.RIR]= st->Bus.R;
					deroach("Wrote 0x%02x to Register File address 0x%02x\n",
							st->Reg.RF[st->Reg.RIR], st->Reg.RIR);
					break;
				case D_K11_1_WrCLK_L13A_SET:
					deroach("Clock to S_ input of flip-flop UL13A\n");
					break;
				case D_K11_3_WrCLK_F11_ENABLE: {
					bit_t D=t->uIW.LB&0x1;
					octal_t A=(t->uIW.LB&0xe)>>1;
					st->Reg.LUF11= ( st->Reg.LUF11 & (~(1<<A)&0xff) ) | (D<<A);
					deroach("UF11 latched bit %0x=%0x, now contains 0x%02x\n", A, D, st->Reg.LUF11);
					}; break;
				case D_K11_6_WrCLK_WAR_LSB:
					deroach("Clock for write to WAR\n");
					break;
				case D_K11_7_WrCLK_BUS_SYS_DATA:
					st->Reg.SDBOR=st->Bus.R;
					deroach("Clock: Wrote 0x%02x to System Data Bus Output Register (SDBOR)\n", st->Reg.SDBOR);
					break;
				case D_H11_3_WRITE_WAR_MSB:
					st->Reg.WAR= (st->Reg.WAR&0x00ff) | (st->Bus.DP<<8);
					deroach("Wrote 0x%02x to MSB of Working Address Register, now holds 0x%04x (WAR)\n",
						st->Bus.DP, st->Reg.WAR);
					break;
				case D_H11_7_WRITE_NSWAPR:
					st->Reg.NSWAPR= ( (st->Bus.DP&0xf0)>>4) | ( (st->Bus.DP&0x0f)<<4);
					deroach("Wrote 0x%02x to Nibble Swap Register from DP-Bus\n", st->Reg.NSWAPR);
					break;
				case D_E7_2_WRITE_FLR:
					st->Reg.FLR= (
						( (st->ALU.FZ & 0x1)	<<0) |
						( (st->ALU.F7 & 0x1)	<<1) |
						( (st->ALU.OVF & 0x1)	<<2) |
						( (st->ALU.Cout & 0x1)	<<3) |
						( (st->ALU.Chalf & 0x1)	<<4) |
						( (/* Unknown */ 0&0x1) <<5)
					);
					deroach("Wrote 0x%02x to Flag Register (FLR)\n", st->Reg.FLR);
					break;

				default: break;

			}
		}
		//deroach("\n");
	}
}

void uIW_trace_run_Seqs(cpu_state_t *st, uIW_trace_t *t ) {

	/* Force for now until and unless we find logic connectoins for these */
	st->Seq.OE_= 0; // Always Enable Output
	st->Seq.ZERO_= 1; // Never Zero
	st->Seq.Cn=1; // Always Increment


	/* Transfer inputs from microinstruction word to Seq inputs - this could be done with pointer mods instead? */
	st->Seq.S0S= (~t->uIW.S0S)&0x3; // Seq0 Source for next uA
	st->Seq.S1S= (~t->uIW.S1S)&0x3; // Seq1 Source for next uA
	st->Seq.S2S= (~t->uIW.S2S)&0x3; // Seq2 Source for next uA

	st->Seq.FE_= t->uIW.FE_&0x1; // (Active LO) File Enable, PUsh/Pop enabled when active
	st->Seq.PUP= t->uIW.PUP&0x1; // PUP=LO: PUsh next uPC to stack; PUP=HI: Pop stack

	st->Seq.DiS0= (t->uIW.uADDR&0x00f)>>0; // Seq0 Direct Data Input Di<3:0>
	st->Seq.DiS1= (t->uIW.uADDR&0x0f0)>>4; // Seq1 Direct Data Input Di<7:4>
	st->Seq.DiS2= (t->uIW.uADDR&0x700)>>8; // Seq2 Direct Data Input Di<11:8>
	
	printf("S0S=%0x  S1S=%0x  S2S=%0x\n",st->Seq.S0S, st->Seq.S1S, st->Seq.S2S);

	st->Seq.RiS0= (st->Bus.F&0x0f)>>0; // Seq0 uA Register Input Ri<3:0>
	st->Seq.RiS1= (st->Bus.F&0xf0)>>4; // Seq1 uA Register Input Ri<7:4>


	// Try some logic on for size
	if(!t->uIW.CASE_) {
		printf("Let's try a conditoinal!\n");
		printf("OR1S0=ALU.FZ=%0x  ",st->ALU.FZ);
		st->Seq.ORiS0= t->uIW.CASE_?0x0:( (st->ALU.FZ <<1) ) ;
		printf("Seq.ORiS0=%0x\n",st->Seq.ORiS0);
	} else { st->Seq.ORiS0=0; }

	/* Run our sequencers */
	am2909_update(&st->dev.Seq0);
	am2909_update(&st->dev.Seq1);
	am2911_update(&st->dev.Seq2);

}

void uIW_trace_run_ALUs(cpu_state_t *st, uIW_trace_t *t ) {
	
	/* Transfer inputs from microinstruction word to ALU inputs - this could be done with pointer mods instead? */
	st->ALU.ADDR_A= t->uIW.LA&0xf;
	st->ALU.ADDR_B= t->uIW.LB&0xf;
	st->ALU.I876= ((octal_t)t->uIW.I876)&0x7;
	st->ALU.I543= ((octal_t)t->uIW.I543)&0x7;
	st->ALU.I210= ((octal_t)t->uIW.I210)&0x7;

	/* Update the Di inputs for both ALUs from the DP-Bus */
	st->ALU.Dlow=  (st->Bus.DP&0x0f)>>0;
	st->ALU.Dhigh= (st->Bus.DP&0xf0)>>4;

	/* This needs additional logic */
	st->ALU.Cin= (t->S_Carry==3?0:t->S_Carry==2?st->ALU.Cout:t->S_Carry?1:0);

	/* Cycle starts on falling edge */
	if(CLOCK_IS_HL(st->Clock.B1_.clk)) {
		st->ALU.FZ=1; // Pull FZ high at beginning of cycle, OC outputs will pull low if not zero
	}

	/* Update the shifter inputs/outputs */
	if(t->S_Shift&0x4) {
		st->Shifter.UpLine= (t->S_Shift==7?st->ALU.Cout:t->S_Shift==6?st->ALU.F7:t->S_Shift==5?0:st->ALU.RAM7);
		st->ALU.Q0=st->Shifter.UpLine;
	} else {
		st->Shifter.DownLine= (t->S_Shift==3?st->ALU.Cout:t->S_Shift==2?st->ALU.Q0:t->S_Shift?0:st->ALU.F7);
		st->ALU.RAM7=st->Shifter.DownLine;
	}

	/* Update the ALU states */
	am2901_update(&st->dev.ALU0);
	am2901_update(&st->dev.ALU1);

	/* Update the output on the F-bus if OE_=LO is asserted */
	if(!st->ALU.OE_) {
		st->Bus.F= (((st->ALU.Fhigh&0x0f)<<4)&0xf0) | ((st->ALU.Flow<<0)&0x0f);
	}

	/* Clock.B1_=HI is the end of the cyle, print data here */
	if(CLOCK_IS_LO(st->Clock.B1_.clk)) {
		am2901_print_state(&st->dev.ALU0);
		am2901_print_state(&st->dev.ALU1);
		if(!st->ALU.OE_) { deroach("ALU is outputting Y=0x%02x to F-Bus\n", st->Bus.F); }
	}
}


void trace_uIW(cpu_state_t *st, uIW_trace_t *t, uint16_t addr, uint64_t in) {
	t->uADDR=addr;
	parse_uIW(&(t->uIW), in);


	/* Assemble complete sequencer operations for each sequencer */
	/* S0&S2 S{01} -> (NAND INT_), FE_ -> (NAND INT_) -> INV (cancels), PUP has no NAND */
	t->Seq0Op=  ( ((~t->uIW.S0S)&0x3)<<2) | (t->uIW.FE_<<1) | (t->uIW.PUP<<0) ;
	t->Seq1Op=  ( ((~t->uIW.S1S)&0x3)<<2) | (t->uIW.FE_<<1) | (t->uIW.PUP<<0) ;
	t->Seq2Op=  ( ((~t->uIW.S2S)&0x3)<<2) | (t->uIW.FE_<<1) | (t->uIW.PUP<<0) ;

	/* Start our cycle */	
	clock_set(&st->Clock.B,CLK_HL);
	clock_set(&st->Clock.B0_,st->Clock.B.clk_);
	clock_set(&st->Clock.B1_,st->Clock.B.clk_);
	clock_set(&st->Clock.B2_,st->Clock.B.clk_);
	do {
		/* Clock.B0_ */
		if(CLOCK_IS_LH(st->Clock.B0_.clk)) {
			/* Decode decoder codes (Active LO) */
			/* From Latch UJ5 */
			t->K11_Out= ~( 1<<(t->uIW.D_K11) )&0xff;
			t->H11_Out= ~( 1<<(t->uIW.D_H11) )&0xff;
			/* From Latches UJ5 & UH5 */
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
			 *  Za -> ALU0.Cn (UF7)
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
			 * Za -> RAM7[ALU1.RAM3] (UF9), UJ10.I5
			 * Zb -> Q0[ALU0.Q0] (UF7), UJ10.I7
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


		}


		/* Clock.B2_ */
		if(CLOCK_IS_LH(st->Clock.B2_.clk)) {
			/* Decode decoder codes (Active LO) */
			/* To Latch D4 */
			t->D2_Out= ~(  ( t->uIW.D_D2D3&0x8)? 0 : 1<<((t->uIW.D_D2D3)&0x3) )&0xf;
			/* To Latch D5 */
			t->D3_Out= ~( (t->uIW.D_D2D3&0x8)? 1<<((t->uIW.D_D2D3)&0x7) : 0 )&0x3f;
			/* From Latch E5 */
			t->E6_Out= ~( 1<<(t->uIW.D_E6) )&0xff;
		}




		t->R= st->Bus.R;
		t->F= st->Bus.F;
		t->DP=st->Bus.DP;

		deroach("\nRunning READS:\n");
		do_read_sources(st, t);
		t->R= st->Bus.R;
		t->F= st->Bus.F;
		t->DP=st->Bus.DP;
		deroach("\nDP-Bus: 0x%02x\n",st->Bus.DP);

		deroach("\nRunning ALUs:");
		uIW_trace_run_ALUs(st, t);
		t->R= st->Bus.R;
		t->F= st->Bus.F;
		t->DP=st->Bus.DP;

		deroach("\nUpdating conditional:\n");
		do_conditionals(st,t);

		st->Bus.R= st->Reg.RR;
		deroach("\nUpdated R-Bus from Result Register: 0x%02x\n", st->Bus.R);

		deroach("\nRunning SEQs:\n");
		uIW_trace_run_Seqs(st, t);

		deroach("\nRunning WRITES:\n");
		do_write_dests(st, t);
		t->R= st->Bus.R;
		t->F= st->Bus.F;
		t->DP=st->Bus.DP;
		

		t->RIR=st->Reg.RIR;

		clock_advance(&st->Clock.B);
		clock_advance(&st->Clock.B0_);
		clock_advance(&st->Clock.B1_);
		clock_advance(&st->Clock.B2_);

	} while(!CLOCK_IS_HL(st->Clock.B.clk));
	deroach("\nCycle complete, here's your trace:\n");

	/* Update our next uA from sequencer outputs */
	t->uADDR_Next= ((st->Seq.YS2&0x7)<<8) | ((st->Seq.YS1&0xf)<<4) | ((st->Seq.YS0&0xf)<<0);
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

	am2901_init(&st->dev.ALU0, "ALU0", &st->Clock.B1_.clk,
		&st->ALU.I210, &st->ALU.I543, &st->ALU.I876,
		&st->ALU.RAM0Q7, &st->ALU.RAM3,
		&st->ALU.ADDR_A, &st->ALU.ADDR_B,
		&st->ALU.Dlow, &st->ALU.Cin,
		&st->ALU.P_0, &st->ALU.G_0,
		&st->ALU.Chalf, &st->ALU.nibbleOVF,
		&st->ALU.Q0, &st->ALU.Q3,
		&st->ALU.FZ, &st->ALU.F3,
		&st->ALU.Flow, &st->ALU.OE_);

	am2901_init(&st->dev.ALU1, "ALU1", &st->Clock.B1_.clk,
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
		&st->Clock.B0_.clk, /* Clock state from clockline */
		&st->Seq.S0S, &st->Seq.FE_, &st->Seq.PUP, /* Select operation */
		&st->Seq.DiS0, /* Direct inputs */
		&st->Seq.RiS0, &st->Seq.RE_, /* AR inputs (tied to Di on am2911) (when RE_ is LO) inputs */
		&st->Seq.Cn, &st->Seq.C4, /* Incrementer carry in and carry out, Cn=1 increments uPC, Cn=0 repeats current op */
		&st->Seq.ORiS0, &st->Seq.ZERO_, &st->Seq.OE_, /* Outputs overrides: OE_=1->HiZ, ZERO_=0->Y=0, ORi=1->Yi=1 */
		&st->Seq.YS0 /* Outputs Yi of Y if not overridden by above */
	);

	am2909_init(&st->dev.Seq1, "Seq1",
		&st->Clock.B0_.clk, /* Clock state from clockline */
		&st->Seq.S1S, &st->Seq.FE_, &st->Seq.PUP, /* Select operation */
		&st->Seq.DiS1, /* Direct inputs */
		&st->Seq.RiS1, &st->Seq.RE_, /* AR inputs (tied to Di on am2911) (when RE_ is LO) inputs */
		&st->Seq.C4, &st->Seq.C8, /* Incrementer carry in and carry out, Cn=1 increments uPC, Cn=0 repeats current op */
		&st->Seq.ORiS1, &st->Seq.ZERO_, &st->Seq.OE_, /* Outputs overrides: OE_=1->HiZ, ZERO_=0->Y=0, ORi=1->Yi=1 */
		&st->Seq.YS1 /* Outputs Yi of Y if not overridden by above */
	);

	am2911_init( &st->dev.Seq2, "Seq2",
		&st->Clock.B0_.clk, /* Clock state from clockline */
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
	printf("DP-Bus: 0x%02x\n", t->DP);
	printf("ALUs: A=0x%01x B=0x%01x RS=%s %s -> %s\n",
		t->uIW.LA,
		t->uIW.LB,
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
	clock_init(&st->Clock.B,"Clock.B", CLK_LO);
	clock_init(&st->Clock.B,"Clock.B0_", CLK_HI);
	clock_init(&st->Clock.B,"Clock.B1_", CLK_HI);
	clock_init(&st->Clock.B,"Clock.B2_", CLK_HI);
	init_Seqs(st);
	init_ALUs(st);
	st->Bus.DP=0;
	st->Bus.F=0;
	st->Reg.LUF11=0;
}

char *inj_NOP[1] = {"01"};

int main(int argc, char **argv) {
	int r;
	uint16_t tmp;
	word_t uA, uAp;
	uint64_t salad;
	char binstr[100];
	cpu_state_t cpu_st;
	uIW_trace_t trace[ROMSIZE];
	//Byte to force into sys data bus register
	if(argc > 1 ) { inject=argv+1; inject_size=argc-1; }
	else { inject=inj_NOP; inject_size=1; }

	//if( (r=read_roms()) == 0 ) {
	//merge_roms();

	init_cpu_state(&cpu_st);
	uA=0x0; uAp=0;
	for(int i=0; i<ROMSIZE_uCode_rom; i++) {
		//printf("\n%#06x: %#06x",i,allrom[0][i]);
		//byte_bits_to_binary_string_grouped(binstr, allrom[0][i], NUMROMS*8, 1);
		//printf("   %s",binstr);
		//int64_bits_to_binary_string_grouped(binstr, iws[i], NUMROMS*8,4);
		//int64_bits_to_binary_string_fields(binstr, iws[uA], NUMROMS*8,
		//	"\x1\1\1\x2\x4\x4\x3\x3\x3\x1\x2\x2\x2\x3\x4\x4\x1\x2\x3\x3\x3\x4");
		//printf("Cycle:%04u 0x%#03x: 0x%016"PRIx64" %s\n",i,uA,iws[uA],binstr);
		int64_bits_to_binary_string_fields(binstr, uCode_rom[uA], ROMWIDTH_uCode_rom*8,
			"\x1\1\1\x2\x4\x4\x3\x3\x3\x1\x2\x2\x2\x3\x4\x4\x1\x2\x3\x3\x3\x4");
		printf("Cycle:%04u 0x%03x: 0x%016"PRIx64" %s\n",i,uA,uCode_rom[uA],binstr);
		//trace_uIW(&cpu_st, &trace[i],i,iws[i]);
		trace[i].uADDR_Prev=uAp;
		trace_uIW(&cpu_st, &trace[i],uA,uCode_rom[uA]);
		//trace_uIW(&cpu_st, &trace[i],uA,iws[uA]);
		print_uIW_trace(&trace[i]);
		uAp=uA;
		uA=trace[i].uADDR_Next;
		//uA=i;
	}
	//} else {
	//	printf("Could not read ROMS!");
	//}
}
