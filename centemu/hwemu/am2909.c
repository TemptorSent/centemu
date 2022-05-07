#include <stdio.h>
#include "am2909.h"
#include "ginsumatic.h"


char *am2909_clock_edge_LH(am2909_device_t *dev) {
	uint8_t O,SP;

	/* Latch Ri into AR if RE_ is LOW */
	if( !S_(RE_) ){ I_(AR)=oS_(Ri,S_(Di)); deroach("Seq Latching AR=%0x\n",I_(AR)); }

	/* Select source to temp output O */
	switch((enum am2909_source_code) S_(S)) {
		case uPC: O=I_(uPC); deroach("Seq S:uPC=%0x\n",O); break;
		case AR: O=I_(AR); deroach("Seq S:AR=%0x\n",O); break;
		case STK0: O=dev->STK[I_(SP)]; deroach("Seq S:STK[%0x]=%0x\n",I_(SP),O); break;
		case Di: O=S_(Di); deroach("Seq S:Di=%0x\n",O); break;
	}

	/* Apply ZERO_ and ORi to O */
	O=(S_(ZERO_)?O|oS_(ORi,0):0);
	deroach("Seq O=%x%s",O, S_(ZERO_)?"":" (ZERO_)");
	if(oS_(ORi,0)){ deroach(" ORi=%0x",oS_(ORi,0)); }

	/* Increment our uPC based on Cn and set Co if needed */
	I_(uPC)=(O+S_(Cn))&0xf;
	S_(Co)=(S_(Cn)&&O==0xf)?1:0;
	deroach("  uPC=%0x (Cin=%0x,Cout=%0x)",I_(uPC),S_(Cn),S_(Co));


	/* Push/Pop as indicated by FE_ and PUP */
	if(!S_(FE_)) {
		SP=I_(SP);
		if(S_(PUP)) {
			I_(SP)=(SP+1)&0x3;
			dev->STK[I_(SP)]=uPC;
			deroach("SP++");
		} else {
			I_(SP)=SP?SP-1:0x3;
			deroach("SP--");
		}
	} else { deroach("HOLD SP"); }

	/* Set output values Y if OE_ is LOW (HiZ=1) */
	TRI_OUTPUT(S_(Y),!S_(OE_),O);
	if(S_(OE_)) { deroach(" Y=HiZ\n"); } 
	else { deroach(" Y=%2x\n",S_(Y)); }
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
