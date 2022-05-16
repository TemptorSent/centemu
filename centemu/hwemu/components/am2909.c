#include <stdio.h>
#include "am2909.h"
#include "../common/ginsumatic.h"


char *am2909_clock_edge_LH(am2909_device_t *dev) {
	uint8_t ouPC, tuPC,SP;

	ouPC=I_(uPC);

	/* Latch Ri into AR if RE_ is LOW */
	if( !S_(RE_) ){ I_(AR)= oS_(Ri, S_(Di)); /* deroach("\tLatching AR=0x%01x\n", I_(AR));*/ }


	/* Output first */
	/* If OE_=HI, don't output anything (nor change uPC from MUX?) */
	if(S_(OE_)) {
		deroach("%s: Y=HiZ - Outputs Disabled (OE_=HI)", I_(id));
	/* If OE_=LO, calculate our output uPC */
	} else {
		/* If ZERO_=LO, force Y=0 */
		if(!S_(ZERO_)) {
			tuPC=0;
			deroach("%s: Y=0x0 (ZERO_=LO)", I_(id));
		/* Otherwise, decode S to determine our source */
		} else {
			deroach("\t%s: uPC Source ", I_(id));
			/* Select source to temp output O */
			switch((enum am2909_source_code) S_(S)) {
				case uPC: tuPC=I_(uPC); deroach("S0: uPC=0x%01x\n",tuPC); break;
				case AR: tuPC=I_(AR); deroach("S1: AR=0x%01x\n",tuPC); break;
				case STK0: tuPC=dev->STK[I_(SP)]; deroach("S2: STK[%0x]=0x%01x  ",I_(SP),tuPC); break;
				case Di: tuPC=S_(Di); deroach("S3: Di=0x%01x\n",tuPC); break;
			}

			/* Apply ORi to O */
			tuPC|= oS_(ORi,0);
			//deroach("\tY=uPC=0x%01x", tuPC);
			//if(oS_(ORi,0)){ deroach(" ORi=0x%01x", oS_(ORi,0)); }
		}
		S_(Y)= I_(uPC)= tuPC; // Store our uPC and output it to Y
		
	}
	//deroach("\n\t");


	/* Push/Pop as indicated by FE_ and PUP */
	if(!S_(FE_)) {
		deroach("\n\t");
		SP=I_(SP);
		if(S_(PUP)) {
			I_(SP)=(SP+1)&0x3;
			dev->STK[I_(SP)]=ouPC;
			deroach("SP++ (PUSH 0x%01x);",ouPC);
		} else {
			I_(SP)=SP?SP-1:0x3;
			deroach("SP-- (POP);");
		}
	} else {
		;//deroach("SP HOLD;"); 
	}
	
	//deroach(" SP=%01x STK",I_(SP));
	//for(int i=0; i<4; i++) { deroach(" [%0x]=0x%01x ",i,dev->STK[i]); }
	//printf("\n");


	/* Increment our uPC based on Cn and set Co if needed */
	S_(Co)= ( S_(Cn) && (I_(uPC)==0xf) )? 1 : 0;
	I_(uPC)=(I_(uPC)+S_(Cn))&0xf;

	//deroach("\tNext uPC=0x%01x  AR=0x%01x  (Cin=%0x,Cout=%0x)\n",I_(uPC),I_(AR),S_(Cn),S_(Co));

	return(0);
}


void am2909_update(am2909_device_t *dev) {
	if(S_(clk)==CLK_LH) { am2909_clock_edge_LH(dev); }
}

//	am2909_init(seq0,id,&clk,&S,&FE_,&PUP,&Di,&Ri,&RE_,&Cn,&Co,&ORi,&ZERO_,&OE_,&Y);
int am2909_init(am2909_device_t *dev, char* id,
	clock_state_t *clk, /* Clock state from clockline */
	twobit_t *S, bit_t *FE_, bit_t *PUP, /* Select operation */
	nibble_t *Di, nibble_t *Ri, bit_t *RE_, /* Direct and AR (when RE_ is LO) inputs */
	bit_t *Cn, bit_t *Co, /* Incrementer carry in and carry out, Cn=1 increments uPC, Cn=0 repeats current op */
	nibble_t *ORi, bit_t *ZERO_, bit_t *OE_, /* Outputs overrides: OE_=1->HiZ, ZERO_=0->Y=0, ORi=1->Yi=1 */
	nibble_t *Y /* Outputs Yi of Y if not overridden by above */) {
	dev->clk=clk;
	dev->id=id;
	dev->S=S;
	dev->FE_=FE_;
	dev->PUP=PUP;
	dev->Cn=Cn;
	dev->Di=Di;
	dev->RE_=RE_;
	dev->Ri=Ri;

	dev->Co=Co;
	dev->ORi=ORi;
	dev->ZERO_=ZERO_;
	dev->OE_=OE_;
	dev->Y=Y;

	// Initialize internal state to 0
	dev->uPC=0;
	dev->AR=0;
	dev->SP=0;
	for(int i=0; i<4; i++) { dev->STK[i]=0;}
	return(0);
}
typedef am2909_device_t am2911_device_t;
int am2911_init(am2911_device_t *dev, char* id,
	clock_state_t *clk, /* Clock state from clockline */
	twobit_t *S, bit_t *FE_, bit_t *PUP, /* Select operation */
	nibble_t *Di, bit_t *RE_, /* Direct and AR (when RE_ is LO) inputs */
	bit_t *Cn, bit_t *Co, /* Incrementer carry in and carry out, Cn=1 increments uPC, Cn=0 repeats current op */
	bit_t *ZERO_, bit_t *OE_, /* Outputs overrides: OE_=1->HiZ, ZERO_=0->Y=0, ORi=1->Yi=1 */
	nibble_t *Y /* Outputs Yi of Y if not overridden by above */) {

	return(am2909_init(dev,id,clk,S,FE_,PUP,Di,0,RE_,Cn,Co,0,ZERO_,OE_,Y));
}

void am2911_update(am2911_device_t *dev){ am2909_update(dev); }

int main_am2909() {
	am2909_device_t d0a={}, *seq0;
	clockline_t clockline, *cl;
	clock_state_t *clk;
	twobit_t S;
	nibble_t Di, Ri, ORi, Y;
	bit_t FE_, PUP, Cn, RE_;
	bit_t Co, ZERO_, OE_;
	char *seq0id="seq0";
	seq0=&d0a;
	cl=&clockline;
	clk=&cl->clk;
	clock_init(cl,"Sequencer Clockline",CLK_LO);
	am2909_init(seq0,seq0id,clk,&S,&FE_,&PUP,&Di,&Ri,&RE_,&Cn,&Co,&ORi,&ZERO_,&OE_,&Y);
/*
	seq0->S=&S;
	seq0->FE_=&FE_;
	seq0->PUP=&PUP;
	seq0->Cn=&Cn;
	seq0->Di=&Di;
	seq0->RE_=&RE_;
	seq0->Ri=&Ri;

	seq0->Co=&Co;
	seq0->ORi=&ORi;
	seq0->ZERO_=&ZERO_;
	seq0->OE_=&OE_;
	seq0->Y=&Y;
*/
	am2909_clock_edge_LH(seq0);
	return(0);
}
