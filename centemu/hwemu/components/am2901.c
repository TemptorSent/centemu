#include <stdio.h>
#include "../common/logic-common.h"
#include "am2901.h"

uint8_t am2901_read_Rmux(am2901_device_t *dev) {
	switch(dev->Rmux) {
		case 0: case 'Z': case '0': return(0);
		case 1: case 'D': return( *(dev->D) & 0xf );
		case 2: case 'A': return( dev->A & 0xf );
	}
	return(0);
}

uint8_t am2901_read_Smux(am2901_device_t *dev) {
	switch(dev->Smux) {
		case 0: case 'Z': case '0': return(0);
		case 1: case 'A': return(dev->A);
		case 2: case 'B': return(dev->B);
		case 3: case 'Q': return(dev->Q);
	}
	return(0);
}

uint8_t am2901_read_RAMmux(am2901_device_t *dev) {
	switch(dev->RAMmux) {
		case 0: case 'X': return(0);
		case 1: case 'D': return( ( (dev->F>>1) | ((*(dev->RAM3)&0x1)<<3) ) & 0xf ); 
		case 2: case 'N': return( (dev->F) & 0xf );
		case 3: case 'U': return( ( (dev->F<<1) | (*(dev->RAM0)&0x1) ) & 0xf );
	}
	return(0);
}

uint8_t am2901_read_Qmux(am2901_device_t *dev) {
	switch(dev->Qmux) {
		case 0: case 'X': return(0);
		case 1: case 'D': return( ( (dev->Q>>1) | ((*(dev->Q3)&0x1)<<3) ) & 0xf); 
		case 2: case 'N': return( (dev->F) & 0xf );
		case 3: case 'U': return( ( (dev->Q<<1) | (*(dev->Q0)&0x1) ) & 0xf);
	}
	return(0);
}

uint8_t am2901_readYmux(am2901_device_t *dev) {
	return( (dev->Ymux=='A'?dev->A:dev->F) & 0xf);
}


char *am2901_source_operand_decode(am2901_device_t *dev) {
	octal_t I210=*(dev->I210);
	/* Set MUX connections for R (0,A,D) and S (0,A,B,Q) operands */
	dev->Rmux=am2901_source_operands[I210][0];
	dev->Smux=am2901_source_operands[I210][1];
	return(am2901_source_operand_mnemonics[I210]);
}

char *am2901_destination_decode(am2901_device_t *dev) {
	octal_t I876=*(dev->I876);
	/* Set output source ('A' or 'F') */
	dev->Ymux=am2901_destinations[I876][0];

	/* Set RAM enable, mux, and RAM0/RAM3 directions */
	dev->RAM_EN=am2901_destinations[I876][1];
	dev->RAMmux=am2901_destinations[I876][2];
	dev->RAM0_DIR=am2901_destinations[I876][3];
	dev->RAM3_DIR=am2901_destinations[I876][4];

	/* Set Q enable, mux, and RAM0/RAM3 directions */
	dev->Q_EN=am2901_destinations[I876][5];
	dev->Qmux=am2901_destinations[I876][6];
	dev->Q0_DIR=am2901_destinations[I876][7];
	dev->Q3_DIR=am2901_destinations[I876][8];

	/* Return the mnemonic */
	return(am2901_destination_mnemonics[I876]);
}

char *am2901_function_decode(am2901_device_t *dev) {
	octal_t I543=*(dev->I543)&0x7;
	nibble_t R=am2901_read_Rmux(dev)&0xf;
	nibble_t S=am2901_read_Smux(dev)&0xf;

	nibble_t F;
	bit_t P,P_,P3,P2,P1,P0,Pall,G,G_,G3,G2,G1,G0,Gany,Cn,Co,C4,C3,OVR;
	Cn=*(dev->Cn)&0x1;

	/* Complement R or S before calculating P and G as required */
	switch(I543) {
		case SUBR: case NOTRS: case EXOR: R = (~R)&0xf; break;
		case SUBS: S = (~S)&0xf;
	}

	P=(R|S)&0xf;
	G=(R&S)&0xf;

	P0=(P&0x1)?1:0;
	P1=(P&0x2)?1:0;
	P2=(P&0x4)?1:0;
	P3=(P&0x8)?1:0;
	Pall=(P3&P2&P1&P0);

	G0=(G&0x1)?1:0;
	G1=(G&0x2)?1:0;
	G2=(G&0x4)?1:0;
	G3=(G&0x8)?1:0;
	Gany=(G3|G2|G1|G0);

	C4=G3|(P3&G2)|(P3&P2&G1)|(P3&P2&P1&G0)|(P3&P2&P1&P0&Cn);
	C3=G2|(P2&G1)|(P2&P1&G0)|(P2&P1&P0&Cn);

	switch(I543) {
		case ADD:
		case SUBR:
		case SUBS:
			F=( R + S + Cn ) & 0xf;
			P_=(~Pall) & 0x1;
			G_=(~((G3)|(P3&G2)|(P3&P2&G1)|(P3&P2&P1&G0))) & 0x1;
			Co=C4;
			OVR=C3^C4;
			break;
		case OR:
			F=( R | S ) & 0xf;
			P_=0;
			G_=Pall;
			Co=( (~Pall) | Cn) & 0x1;
			OVR=Co;
			break;
		case AND:
		case NOTRS:
			F=( R & S ) & 0xf;
			P_=0;
			G_= (~Gany) & 0x1;
			Co= Gany | Cn;
			OVR=Co;
			break;
		case EXOR:
		case EXNOR:
			F=( ( R & S ) | ( ((~R)&0xf) & ((~S)&0xf) ) ) & 0xf;
			P_=Gany;
			G_= G3 | (P3&G2) | (P3&P2&G1) | Pall;
			Co= (~( G_ & ( G0 | (~Cn&0x1) ) )) & 0x1;
			OVR=( ( ~P2 | (~G2&~P1) | (~G2&~G1&~P0) | (~G2&~G1&~G0&Cn) )
			    ^ ( ~P3 | (~G3&~P2) | (~G3&~G2&~P1) | (~G3&~G2&~G1&~P0) | (~G3&~G2&~G1&~G0&Cn) )
			    ) & 0x1;
			break;
	}
	dev->F=F;
	*(dev->P_)=P_;
	*(dev->G_)=G_;
	*(dev->Co)=Co;
	*(dev->OVR)=OVR;
	return(am2901_function_mnemonics[I543]);
}




char *am2901_clock_state_setup_H(am2901_device_t *dev){
	/* Decode destination portion of instruction */
	am2901_destination_decode(dev);
	return(0);
}

char *am2901_clock_state_edge_HL(am2901_device_t *dev) {
	/* Latch A & B from RAM address on A0-A3 */
	dev->A=dev->RAM[*(dev->ADDR_A)];
	dev->B=dev->RAM[*(dev->ADDR_B)];
	return(0);
}


char *am2901_clock_state_setup_L(am2901_device_t *dev){
	/* Decode source operands portion of instruction */
	am2901_source_operand_decode(dev);

	/* Decode ALU operation */
	am2901_function_decode(dev);

	/* Update RAM at address in ADDR_B if RAM_EN is high */
	if(dev->RAM_EN) { dev->RAM[*(dev->ADDR_B)]=am2901_read_RAMmux(dev); }
	return(0);
}

char *am2901_clock_state_edge_LH(am2901_device_t *dev) {
	/* Update Q regsister if Q_EN is high */
	if(dev->Q_EN) { dev->Q=am2901_read_Qmux(dev); }
	return(0);

}

char *am2901_update(am2901_device_t *dev) {
	//printf("clk=%s\n",CLOCK_STATE_NAME_FULL(*(dev->clk)));
	switch(*(dev->clk)) {
		case CLK_HL: am2901_clock_state_edge_HL(dev); break;
		case CLK_LO: am2901_clock_state_setup_L(dev); break;
		case CLK_LH: am2901_clock_state_edge_LH(dev); break;
		case CLK_HI: am2901_clock_state_setup_H(dev); break;
		default:
			printf("Unknown clock state: %0x", S_(clk)); break;
	}

	/* Update shifter outputs */
	if(dev->Q0_DIR=='O') { *(dev->Q0)=dev->Q&0x1; }
	if(dev->Q3_DIR=='O') { *(dev->Q3)=(dev->Q>>3)&0x1; }
	if(dev->RAM0_DIR=='O') { *(dev->RAM0)=dev->F&0x1; }
	if(dev->RAM3_DIR=='O') { *(dev->RAM3)=(dev->F>>3)&0x1; }

	/* Zero flag - must be "pulled high" externally, this acts as an open-collector output*/
	if(dev->F&0xf){*(dev->FZ)=0;}

	/* F3 flag - set if MSB of F is 1 (sign bit if MSB of word)*/
	*(dev->F3)=dev->F&0x8?1:0;

	/* Update Y outputs if enabled */
	if(!*(dev->OE_)){*(dev->Y)=am2901_readYmux(dev);}
	return(0);
}


typedef struct alu_input_t {
	enum am2901_source_operand_code I210;
	enum am2901_function_code I543;
	enum am2901_destination_code I876;
	nibble_t ADDR_A, ADDR_B;
	nibble_t Din;
	bit_t Cin;
} alu_input_t;
	
void am2901_print_state_ram(am2901_device_t *dev) {
	printf("RAM:"); 
	for(int r=0; r<16; r++) {
		printf(" [%0x]:%0x",r,dev->RAM[r]);
	}
	printf("\n");
}

void am2901_print_state_inputs(am2901_device_t *dev) {
	printf("I=%0o%0o%0o Cin=%0x, OE_=%0x",S_(I876),S_(I543),S_(I210), S_(Cn), S_(OE_));
	printf(" ADDR_A=%0x, ADDR_B=%0x, D=%0x",S_(ADDR_A), S_(ADDR_B),S_(D));;
}

void am2901_print_state_sources(am2901_device_t *dev) {
	printf(" A=%0x, B=%0x, D=%0x, Q=%0x", I_(A), I_(B), S_(D), I_(Q));
}

void am2901_print_state_mux_RS(am2901_device_t *dev) {
	printf(" R='%c'=%0x, S='%c'=%0x ", I_(Rmux), am2901_read_Rmux(dev), I_(Smux), am2901_read_Smux(dev));
}


void am2901_print_function_replace_RSC(char *fstr, char sym, char R, char S, char C) {
	char *fsym;
	fsym=fstr;
	while(*fsym) {
		switch(*fsym) {
			case 'R': printf(sym?"%c":"%0x",R); break;
			case 'S': printf(sym?"%c":"%0x",S); break;
			case 'C': printf(sym?"%c":"%0x",C); break;
			default: printf("%c",*fsym); break;
		}
		fsym++;
	}
}

void am2901_print_function_symbolic_reduced(am2901_device_t *dev) {
	char *fstr;
	fstr=am2901_function_symbol_reduced[S_(Cn)?0:1][I_(Smux)=='Z'?2:I_(Rmux)=='Z'?1:0][S_(I543)];
	am2901_print_function_replace_RSC(fstr,1,I_(Rmux), I_(Smux), 'C');
}

void am2901_print_function_numeric_reduced(am2901_device_t *dev) {
	char *fstr;
	fstr=am2901_function_symbol_reduced[S_(Cn)?0:1][I_(Smux)=='Z'?2:I_(Rmux)=='Z'?1:0][S_(I543)];
	am2901_print_function_replace_RSC(fstr,0,am2901_read_Rmux(dev), am2901_read_Smux(dev), S_(Cn));
}

void am2901_print_state_result(am2901_device_t *dev) {
	printf(" F=%0x [%c%c%c%c]",I_(F), S_(Co)?'C':'-',S_(OVR)?'V':'-',S_(F3)?'N':'-',S_(FZ)?'Z':'-');
}
void am2901_print_state_mux_RAM(am2901_device_t *dev, bit_t sym) {
	uint8_t r, rm=I_(RAMmux);
	if(I_(RAM_EN)) {
		if(sym) { printf("F%s -> RAM[%0x], ", rm=='D'?"/2":rm=='U'?"*2":"", S_(ADDR_B)); }
		else {
			r=(rm=='N'?I_(F):rm=='D'?I_(F)>>1|S_(RAM3)<<3:I_(F)<<1|S_(RAM0))&0xf;
			printf("%0x%s=%0x -> RAM[%0x]",I_(F), rm=='D'?"/2":rm=='U'?"*2":"",r, S_(ADDR_B));
		}
	} else { printf("RAM: N/C"); }

	if(I_(RAM0_DIR) == 'Z') { printf(" RAM0=HiZ"); }
	else { printf(" RAM0=%0x(%c)", S_(RAM0), I_(RAM0_DIR) ); }

	if(I_(RAM3_DIR) == 'Z') { printf(" RAM3=HiZ"); }
	else { printf(" RAM3=%0x(%c)", S_(RAM3), I_(RAM3_DIR) ); }
}

void am2901_print_state_mux_Q(am2901_device_t *dev, uint8_t sym) {
	uint8_t q, qm=I_(Qmux);

	if(I_(Q_EN)) {
		if(sym) { printf("%0x%s -> Q,",qm=='N'?'F':'Q',qm=='D'?"/2":qm=='U'?"*2":""); }
		else {
			q=(qm=='N'?I_(F):qm=='D'?I_(Q)>>1|S_(Q3)<<3:I_(Q)<<1|S_(Q0))&0xf;
			printf("%0x%s=%0x -> Q,", qm=='N'?I_(F):I_(Q), qm=='D'?"/2":qm=='U'?"*2":"", q);
		}
	} else { printf("Q: N/C"); }

	if(I_(Q0_DIR) == 'Z') { printf(" Q0=HiZ"); }
	else { printf(" Q0=%0x(%c)", S_(Q0), I_(Q0_DIR) ); }

	if(I_(Q3_DIR) == 'Z') { printf(" Q3=HiZ"); }
	else { printf(" Q3=%0x(%c)", S_(Q3), I_(Q3_DIR) ); }	
}

void am2901_print_state_outputs(am2901_device_t *dev) {
	if (S_(OE_)) { printf(" Y=HiZ (OE_=HI)"); }
	else { printf(" Y='%c'=%0x", I_(Ymux),S_(Y)); }
	
}

void am2901_print_state(am2901_device_t *dev) {
	char *fsym;
	printf("\nRAM:"); 
	for(int r=0; r<16; r++) {
		printf(" [%0x]:%0x",r,dev->RAM[r]);
	}
	printf("\nA=%0x, B=%0x, D=%0x, Q=%0x", dev->A, dev->B, *(dev->D), dev->Q);
	printf(" R:[%c]=%0x, S:[%c]=%0x, C=%0x", dev->Rmux, am2901_read_Rmux(dev), dev->Smux, am2901_read_Smux(dev), *(dev->Cn));
	printf(" F=%0x ",dev->F );
	fsym=am2901_function_symbol_reduced[*(dev->Cn)?0:1][dev->Smux=='Z'?2:dev->Rmux=='Z'?1:0][(uint8_t)*(dev->I543)&0x7];
	while(*fsym) {
		if(*fsym=='R') { printf("%c(%0x)",dev->Rmux,am2901_read_Rmux(dev)); }
		else if(*fsym=='S') { printf("%c(%0x)",dev->Smux,am2901_read_Smux(dev)); }
		else {printf("%c",*fsym);}
		fsym++;
	}

	printf(" Y=[%c]%0x", dev->Ymux,*(dev->Y));
	printf(" Flags: [%c%c%c%c]", *(dev->Co)?'C':' ',*(dev->OVR)?'V':' ',*(dev->F3)?'N':' ',*(dev->FZ)?'Z':' ');
	printf("\nADDR_A=%0x, ADDR_B=%0x ",*(dev->ADDR_A), *(dev->ADDR_B));
	if(dev->RAM_EN) {
		printf("RAM[%0x]=%0x%s",*(dev->ADDR_B), dev->F, dev->RAMmux=='D'?"/2":dev->RAMmux=='U'?"*2":"");
		if(dev->RAM0_DIR == 'Z') { printf(" RAM0=HiZ"); }
		else { printf(" RAM0=%0x(%c)", *(dev->RAM0), dev->RAM0_DIR ); }
		if(dev->RAM3_DIR == 'Z') { printf(" RAM3=HiZ"); }
		else { printf(" RAM3=%0x(%c)", *(dev->RAM3), dev->RAM3_DIR ); }
	} else { printf("RAM: N/C"); }
	if(dev->Q_EN) {
		printf(" Q=%0x%s",dev->Qmux=='N'?dev->F:dev->Q,dev->Qmux=='D'?"/2":dev->Qmux=='U'?"*2":"");
		if(dev->Q0_DIR == 'Z') { printf(" Q0=HiZ"); }
		else { printf(" Q0=%0x(%c)", *(dev->Q0), dev->Q0_DIR ); }
		if(dev->Q3_DIR == 'Z') { printf(" Q3=HiZ"); }
		else { printf(" Q3=%0x(%c)", *(dev->Q3), dev->Q3_DIR ); }
	} else { printf(" Q: N/C"); }

	printf("\n");
}
//am2901_init( clk, id, I210, I543, I876, RAM0, RAM3, ADDR_A, ADDR_B, D, Cn, P_, G_, Co, OVR, Q0, Q3, FZ, F3, Y, OE_);

int am2901_init(am2901_device_t *dev, char* id,
	clock_state_t *clk, /* Clock state from clockline */
	/* Instruction word of 9 bits in 3 groups of 3 bit (octal) values */
	octal_t *I210, /* Source operand octal value */
	octal_t *I543, /* ALU function octal value */ 
	octal_t *I876, /* Destination octal value */

	/* RAM input shifter */
	bit_t *RAM0, /* LSB in RAM data input shift line */
	bit_t *RAM3, /* MSB in RAM data input shift line */
	nibble_t *ADDR_A, /* Address in RAM to use for "A" (read) */
	nibble_t *ADDR_B, /* Address in RAM to use for "B" (read/write) */
	/* External data input */
	nibble_t *D, /* Direct data input */

	/* ALU */
	bit_t *Cn, /* Carry-in */
	bit_t *P_, /* Propagate signal */
	bit_t *G_, /* Generate signal */
	bit_t *Co, /* Carry-out */
	bit_t *OVR, /* Overflow flag */

	/* Q input shifter */
	bit_t *Q0, /* LSB in Q shift line */
	bit_t *Q3, /* MSB in Q shift line */
	/* Outputs */
	bit_t *FZ, /* Zero flag output */
	bit_t *F3, /* High bit set (Negitive) flag output */
	nibble_t *Y, /* Output value */
	bit_t *OE_ /* Output Enable (Active LOW) HiZ=1 */

	) {
	dev->clk=clk;
	dev->id=id;

	/* Instruction word */
	dev->I210=(enum am2901_source_operand_code *)I210;
	dev->I543=(enum am2901_function_code *)I543;
	dev->I876=(enum am2901_destination_code *)I876;

	/* RAM input shifter */
	defS_(RAM0);
	defS_(RAM3);
	defS_(ADDR_A);
	defS_(ADDR_B);

	/* External data input */
	defS_(D);

	/* ALU */
	defS_(Cn);
	defS_(P_);
	defS_(G_);
	defS_(Co);
	defS_(OVR);

	/* Q input shifter */
	defS_(Q0);
	defS_(Q3);

	/* Outputs */
	defS_(FZ);
	defS_(F3);
	defS_(Y);
	defS_(OE_);

	/* Clear internal state */
	I_(F)=0;
	/* Zero A & B registers */
	I_(A)=0;
	I_(B)=0;
	/* Zero Q register and clear it's input enable */
	I_(Q)=0;
	I_(Q_EN)=0;
	/* Initilize Rmux & Smux */
	I_(Rmux)='A';
	I_(Smux)='A';
	/* Initilize RAM shifter */
	I_(RAM0_DIR)='Z';
	I_(RAM3_DIR)='Z';
	I_(RAMmux)='N';
	/* Initilize Q Shifter */
	I_(Q0_DIR)='Z';
	I_(Q3_DIR)='Z';
	I_(Qmux)='N';
	/* Initilize Ymux */
	I_(Ymux)='F';

	for(int r=0; r<16; r++) {
		dev->RAM[r]=0;
	}
	return(0);
}






int am2901_main() {

	clockline_t clockline, *cl;
	cl=&clockline;

	am2901_device_t d0a={}, d1a={};
	am2901_device_t *alu0, *alu1;
	uint8_t din0=0xc, dout0, P_0, G_0, OVR0, FS0;
	uint8_t din1=0x5, dout1, P_1, G_1, OVR1, FS1;
	uint8_t RAM0,RAM3,RAM7, Q0,Q3,Q7, C0,C4,C8;
	uint8_t	aluA=0x0, aluB=0x0, OE_=0;
	uint8_t FZ, FN, FC, FV;
	enum am2901_source_operand_code I210;
	enum am2901_function_code I543;
	enum am2901_destination_code I876;

	clock_init(cl, "ALU Clock", CLK_LO);
	alu_input_t prog[4]={
		{DZ,OR,RAMF,0,0,0xf0,0},
		{DZ,OR,RAMF,0,1,0x55,0},
		{AB,EXNOR,RAMQU,0,1,0,0},
		{DA,ADD,RAMQU,1,2,0x03,0}
	};

	alu0=&d0a;
	alu1=&d1a;
	/* Clocks */
	alu0->clk=&clockline.clk;
	alu1->clk=&clockline.clk;

	/* Inputs */
	alu0->I210=&I210;
	alu0->I543=&I543;
	alu0->I876=&I876;
	alu0->ADDR_A=&aluA;
	alu0->ADDR_B=&aluB;
	alu0->OE_=&OE_;

	alu1->I210=&I210;
	alu1->I543=&I543;
	alu1->I876=&I876;
	alu1->ADDR_A=&aluA;
	alu1->ADDR_B=&aluB;
	alu1->OE_=&OE_;

	alu0->D=&din0;
	alu0->Cn=&C0;
	alu0->Co=&C4;
	alu0->Y=&dout0;
	alu0->P_=&P_0;
	alu0->G_=&G_0;
	alu0->OVR=&OVR0;
	alu0->FZ=&FZ;
	alu0->F3=&FS0;
	alu0->RAM0=&RAM0;
	alu0->RAM3=&RAM3;
	alu0->Q0=&Q0;
	alu0->Q3=&Q3;

	alu1->D=&din1;
	alu1->Y=&dout1;
	alu1->Cn=&C4;
	alu1->Co=&FC;
	alu1->P_=&P_1;
	alu1->G_=&G_1;
	alu1->OVR=&FV;
	alu1->FZ=&FZ;
	alu1->F3=&FN;
	alu1->RAM0=&RAM3;
	alu1->RAM3=&RAM7;
	alu1->Q0=&Q3;
	alu1->Q3=&Q7;
	


	for(int i=0; i<4; i++) {
		/* Pull the Zero flag high (open collector output) */
		FZ=1;

		I210=prog[i].I210;
		I543=prog[i].I543;
		I876=prog[i].I876;

		aluA=prog[i].ADDR_A;
		aluB=prog[i].ADDR_B;
		C0=prog[i].Cin;

		din0=prog[i].Din&0x0f;
		din1=(prog[i].Din&0xf0)>>4;

	//	am2901_clock_state_setup_H(alu0);
	//	am2901_clock_state_setup_H(alu1);

	//	am2901_clock_state_edge_HL(alu0);
	//	am2901_clock_state_edge_HL(alu1);

	//	am2901_clock_state_hold_L(alu0);
	//	am2901_clock_state_hold_L(alu1);
	//	am2901_print_state(alu0);
	//	am2901_print_state(alu1);

	//	am2901_clock_state_setup_L(alu0);
	//	am2901_clock_state_setup_L(alu1);

	//	am2901_clock_state_edge_LH(alu0);
	//	am2901_clock_state_edge_LH(alu1);

	//	am2901_clock_state_hold_H(alu0);
	//	am2901_clock_state_hold_H(alu1);
		am2901_update(alu0);
		am2901_update(alu1);
		printf("\n");
		am2901_print_state_ram(alu1);
		am2901_print_state_ram(alu0);
		am2901_print_state_inputs(alu0);
		am2901_print_state_sources(alu0);
		printf("\n");
		am2901_print_state_inputs(alu1);
		am2901_print_state_sources(alu1);
		printf("\n");
		am2901_print_function_symbolic_reduced(alu1);
		am2901_print_function_symbolic_reduced(alu0);
		am2901_print_state_outputs(alu1);
		am2901_print_state_outputs(alu0);
		printf("\n");
		
		clock_advance(cl);
		am2901_update(alu0);
		am2901_update(alu1);
		
		clock_advance(cl);
		am2901_update(alu0);
		am2901_update(alu1);
		
		clock_advance(cl);
		am2901_update(alu0);
		am2901_update(alu1);
		
		clock_advance(cl);
		am2901_update(alu0);
		am2901_update(alu1);
		//Dout=dout1<<4|dout0;
	//	am2901_print_state(alu0);
	//	am2901_print_state(alu1);
	//	printf("\n%x  Flags: %c%c%c%c \n", Dout,FC?'Z':' ',FV?'V':' ',FN?'N':' ',FZ?'Z':' ');
		/*printf(
			"A[%x]:%x %x  B[%x]:%x %x\n\n",
			aluA, alu1->RAM[aluA], alu0->RAM[aluA],
			aluB,alu1->RAM[aluB],alu0->RAM[aluB]
		);*/
	}
	return(0);
}
