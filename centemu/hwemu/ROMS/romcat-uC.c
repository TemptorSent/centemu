#include <stdio.h>
#include <inttypes.h>
#include "../logic-common.h"
#include "../ginsumatic.h"
#include "../am2901.h"
#include "../am2909.h"

#define NUMROMS 7
#define ROMSIZE 2048

/* Utility functions to extract ranges of bits, with _R reversing the bit order returned */
#define BITRANGE(d,s,n) ((d>>s) & ((1LL<<(n))-1LL) )
#define BITRANGE_R(d,s,n) (bitreverse_64(BITRANGE(d,s,n)) >>(64-n))

uint8_t allrom[NUMROMS][ROMSIZE];
uint8_t mergedrom[ROMSIZE][NUMROMS];
uint64_t iws[ROMSIZE];
static char *ROM_files[NUMROMS] = {
	"CPU_5.rom", /* MWK3.11 - A3.11 */
	"CPU_2.rom", /* MWF3.11 - B3.11 */
	"CPU_3.rom", /* MWH3.11 - C3.11 */
	"CPU_6.rom", /* MWL3.11 - D3.11 */
	"CPU_7.rom", /* MWM3.11 - E3.11 */
	"CPU_4.rom", /* MWJ3.11 - F3.11 */
	"CPU_1.rom"  /* MWE3.11 - ??3.11 */
};

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

/* Concatenate an array of bytes into an unsigned 64-bit int */
uint64_t concat_bytes(uint8_t bytes[]){
	uint64_t out=0,in;
	for(int i=0;i<NUMROMS;i++) {
		in=bytes[i];
		//printf(" in[%u]=%x ",i,in);
		out = out | (in<<((NUMROMS-i-1)*8));
	}
	return(out);
}

uint64_t bitreverse_64(uint64_t in) {
	uint64_t out=0;
	for(int i=63;i>=0;i--) {
		//printf("%u %#018"PRIx64" %#018"PRIx64"\n",i,in,out);
		out |= in&0x1;
		if(!i){return(out);}
		out <<= 1;
		in >>= 1;
		//printf("%u %#018"PRIx64" %#018"PRIx64"\n",i,in,out);
	}
}

uint16_t bitsalad_16(uint64_t order, uint16_t d) {
	uint8_t pos;
	uint16_t out;
	for(int i=0; i<16; i++) {
		pos=BITRANGE(order,i*4,4);
		if(d&1<<i){out |= 1<<pos;}
	}
	return(out);
}

uint8_t bitsalad_8(uint32_t order, uint8_t d) {
	uint8_t pos;
	uint8_t out;
	for(int i=0; i<16; i++) {
		pos=BITRANGE(order,i*4,4);
		if(d&1<<i){out |= 1<<pos;}
	}
	return(out);
}

int read_roms() {
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

}

int merge_roms() {
	uint64_t iw;
	for(int i=0; i<ROMSIZE; i++) {
		//printf("\n%04x",i);
		for(int j=0; j<NUMROMS; j++) {
			mergedrom[i][j]=allrom[j][i];
			//printf(" %02x",mergedrom[i][j]);
		}
		iw=concat_bytes(mergedrom[i]);
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
/* Represent specified number of bits of a 64-bit integer as an ascii string of '1's, '0's and ' ' */
/* Provide an output buffer long enough to hold all bits plus spaces between fields plus NULL */
/* Split sequences of bits into fields of specified widths given as byte string ("\x2\x10\x8..") */
char  *int64_bits_to_binary_string_fields(char *out, uint64_t in, uint8_t bits, char *fieldwidths) {
	char *p;
	char *f;
	uint8_t fc=0;
	f=fieldwidths;
	p=out;
	bits=bits<65?bits:64;
	for(int i=bits-1; i>=0; i--) {
		if(!fc && *f) { fc=*(f++); }
		*(p++)=(in&(1LL<<i))?'1':'0';
		if(!--fc){ *(p++)=' '; }
	}
	*p='\0';
	return(out);
}

/* Represent specified number of bits of a 64-bit integer as an ascii string of '1's and '0's */
/* Provide an output buffer long enough to hold all bits plus spaces between fields plus NULL */
/* Split sequences of bits into space separated groups of specified size */
char  *int64_bits_to_binary_string_grouped(char *out, uint64_t in, uint8_t bits, uint8_t grouping) {
	char *p;
	p=out;
	bits=bits<65?bits:64;
	for(int i=bits-1; i>=0; i--) {
		*(p++)=(in&(1LL<<i))?'1':'0';
		if(i&&!(i%grouping)&&grouping){ *(p++)=' '; }
	}
	*p='\0';
	return(out);
}

char  *byte_bits_to_binary_string_grouped(char *out, uint8_t in, uint8_t bits, uint8_t grouping) {
	char *p;
	p=out;
	bits=bits<9?bits:8;
	for(int i=bits-1; i>=0; i--) {
		*(p++)=(in&(1<<i))?'1':'0';
		if(i&&!(i%grouping)&&grouping){ *(p++)=' '; }
	}
	*p='\0';
	return(out);
}



typedef struct uIW_t {
	bit_t S1S1_OVR_;
	bit_t CASE_;
	nibble_t A,B;
	octal_t I876, I543, I210;
	twobit_t S0S, S1S, S2S;
	bit_t PUP, FE_;
	nibble_t D;
	octal_t D_E6,D_E7,D_H11,D_K11;
	nibble_t D_D2D3;
} uIW_t;

typedef struct uIW_trace_t {
	uint16_t addr;
	uIW_t uIW;
	twobit_t Seq0Op, Seq1Op, Seq2Op;
	nibble_t D2_Out;
	sixbit_t D3_Out, E6_Out, K11_Out, H11_Out, E7_Out;
} uIW_trace_t;


enum decoder_enum { D_D2, D_D3, D_E6, D_K11, D_H11, D_E7 };

char *decoded_sig[6][8][2] = {
	{ // Decoder D2 -> latch UD4
		{"D2.0 D4.4",""},
		{"READ REGISTER",""},
		{"READ ADDRESS MSB",""},
		{"D2.3 D4.3",""},
		{"",""},{"",""},
		{"",""},{"",""}
	},
	{ // Decoder D3 -> latch UD5
		{"D3.0 D5.0", ""},
		{"D3.1 D5.1",""},
		{"D3.2 D5.5",""},
		{"READ IL REGISTER",""},
		{"D3.4 D5.2",""},
		{"READ CONSTANT",""},
		{"",""},{"",""}

	},
	{ // Decoder E6 (from latch E5)
		{"E6.0",""},
		{"WRITE PAGETABLE DATA REGISTER",""},
		{"WRITE REGISTER INPUT LATCH? (RIR)",""},
		{"E6.3",""},
		{"WRITE PAGETABLE ADDRESS REGISTER",""},
		{"WRITE CURRENT PC TO STAGING ADDRESS LATCH","WRITE DATA TO STAGING ADDRESS LATCH"},
		{"WRITE DATA TO SEQUENCERS ADDRESS REGISTER",""},
		{"E6.7",""}
	},
	{ // Decoder K11
		{"K11.0",""},
		{"K11.1",""},
		{"K11.2",""},
		{"K11.3",""},
		{"K11.4",""},
		{"K11.5",""},
		{"K11.6",""},
		{"WRITE DATA REGISTER",""}
	},
	{ // Decoder H11
		{"H11.0",""},
		{"H11.1",""},
		{"H11.2",""},
		{"WRITE ADDRESS LATCH MSB",""},
		{"H11.4",""},
		{"H11.5",""},
		{"READ MAPPING PROM",""},
		{"H11.7",""}
	},
	{ // Decoder E7
		{"E7.0",""},
		{"E7.1",""},
		{"WRITE STATUS REGISTER",""},
		{"E7.3",""},
		{"E7.4",""},
		{"E7.5",""},
		{"E7.6",""},
		{"E7.7",""}
	}

};



void parse_uIW(uIW_t *uIW, uint64_t in) {

	uIW->S1S1_OVR_=BITRANGE(in,54,1);
	uIW->A=BITRANGE(in,47,4);
	uIW->B=BITRANGE(in,43,4);
	uIW->I876=BITRANGE(in,40,3);
	uIW->I543=BITRANGE(in,37,3);
	uIW->I210=BITRANGE(in,34,3);
	uIW->CASE_=BITRANGE(in,33,1);
	uIW->S2S=BITRANGE(in,31,2);
	uIW->S0S=BITRANGE(in,29,2);
	uIW->PUP=BITRANGE(in,28,1);
	uIW->FE_=BITRANGE(in,27,1);
	uIW->D=BITRANGE(in,16,11); 
	uIW->D_E7=BITRANGE(in,13,3);
	uIW->D_H11=BITRANGE(in,10,3);
	uIW->D_K11=BITRANGE(in,7,3);
	uIW->D_E6=BITRANGE(in,4,3);
	uIW->D_D2D3=BITRANGE(in,0,4);
	
	
}

void trace_uIW(uIW_trace_t *t, uint16_t addr, uint64_t in) {
	t->addr=addr;
	parse_uIW(&(t->uIW), in);
	/* S0&S2 S{01} -> (NAND INT_), FE_ -> (NAND INT_) -> INV (cancels), PUP has no NAND */
	t->Seq0Op=  ( ((~t->uIW.S0S)&0x3)<<2) | (t->uIW.FE_<<1) | (t->uIW.PUP<<0) ;
	t->Seq2Op=  ( ((~t->uIW.S2S)&0x3)<<2) | (t->uIW.FE_<<1) | (t->uIW.PUP<<0) ;

	/* S1S1 = S2S1 -> INV -> NAND S1S1_OVR_ -> (NAND INT_), S1S0 -> (NAND INT_) */
	t->Seq1Op=  (
		~( // (NAND INT_)
			~( // NAND S1S1_OVR_
				( // (AND S1S1_OVR_)
					~( // S1S1 INV
						(t->uIW.S2S&0x2)>>1
					) & t->uIW.S1S1_OVR_
				)
			)
		)&0x3)<<3 | ((~t->uIW.S2S)&0x1)<<2 | (t->uIW.FE_<<1) | (t->uIW.PUP<<0) ;


	//t->Seq0Op= ~( (t->uIW.S0S<<2) | ((~(t->uIW.FE_)&0x1)<<1) | (t->uIW.PUP) )&0xf;
	//t->Seq1Op= ~( ((t->uIW.S2S | (t->uIW.S1S_OR_2?2:0))<<2) | ((~(t->uIW.FE_)&0x1)<<1) | (t->uIW.PUP) )&0xf;
	//t->Seq2Op= ~( (t->uIW.S2S<<2) | ((~(t->uIW.FE_)&0x1)<<1) | (t->uIW.PUP) )&0xf;
	t->D2_Out= ~(  ( t->uIW.D_D2D3&0x8)? 0 : 1<<((t->uIW.D_D2D3)&0x3) )&0xf;
	t->D3_Out= ~( (t->uIW.D_D2D3&0x8)? 1<<((t->uIW.D_D2D3)&0x7) : 0 )&0x3f;
	t->E6_Out= ~( 1<<(t->uIW.D_E6) )&0xff;
	t->K11_Out= ~( 1<<(t->uIW.D_K11) )&0xff;
	t->H11_Out= ~( 1<<(t->uIW.D_H11) )&0xff;
	t->E7_Out= ~( 1<<(t->uIW.D_E7) )&0xff;
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
	printf("Data: D=%#05x\n", t->uIW.D);
	printf("ALUs: A=%01x B=%01x RS=%s %s -> %s\n",
		t->uIW.A,
		t->uIW.B,
		am2901_source_operand_mnemonics[t->uIW.I210],
		am2901_function_symbol[t->uIW.I543],
		am2901_destination_mnemonics[t->uIW.I876]
	);
	printf("Seqs: S0: %s, S1: %s, S2: %s\n",
		am2909_ops[t->Seq0Op][3],
		am2909_ops[t->Seq1Op][3],
		am2909_ops[t->Seq2Op][3]
	);
	printf("\nDecoders E7:%02x H11: %02x K11: %02x E6: %02x D3: %02x D2 %02x\n",
		t->E7_Out, t->H11_Out, t->K11_Out, t->E6_Out, t->D3_Out, t->D2_Out);

	print_decoder_values(D_D2,  t->D2_Out);
	print_decoder_values(D_D3,  t->D3_Out);
	print_decoder_values(D_E6,  t->E6_Out);
	print_decoder_values(D_K11, t->K11_Out);
	print_decoder_values(D_H11, t->H11_Out);
	print_decoder_values(D_E7,  t->E7_Out);
	printf("\n");
		

}

int main(int argc, char **argv) {
	int r;
	uint16_t tmp;
	uint64_t salad;
	char binstr[100];
	uIW_trace_t trace[ROMSIZE];
	if( (r=read_roms()) > 0 ) {
		printf("read_roms returned: %i\n",r);
		merge_roms();

		for(int i=0; i<ROMSIZE; i++) {
			//printf("\n%#06x: %#06x",i,allrom[0][i]);
			//byte_bits_to_binary_string_grouped(binstr, allrom[0][i], NUMROMS*8, 1);
			//printf("   %s",binstr);
			//int64_bits_to_binary_string_grouped(binstr, iws[i], NUMROMS*8,4);
			int64_bits_to_binary_string_fields(binstr, iws[i], NUMROMS*8,
				"\x1\1\1\x2\x4\x4\x3\x3\x3\x1\x2\x2\x2\x3\x4\x4\x3\x3\x3\x3\x4");
			printf(" %#06x: %#018"PRIx64" %s\n",i,iws[i],binstr);
			trace_uIW(&trace[i],i,iws[i]);
			print_uIW_trace(&trace[i]);
		/*	
			printf(" 2901:");
			printf(" I=%03o",BITRANGE(iws[i],34,9));
			printf(" A=%01x",BITRANGE(iws[i],47,4));
			printf(" B=%01x",BITRANGE(iws[i],43,4));
			printf(" 2909:");
			printf(" PUP/FE=%01o",BITRANGE(iws[i],27,2));
			printf(" S0S=%01o",BITRANGE(iws[i],29,2));
			printf(" (S1S=%01o)",BITRANGE(iws[i],31,2)|(BITRANGE(iws[i],54,1)?2:0));
			printf(" S2S=%01o",BITRANGE(iws[i],31,2));
			printf(" D=%03x",BITRANGE(iws[i],16,11));
			printf("\n");
		*/
		}
	} else {
		printf("Could not read ROMS!");
	}
}
