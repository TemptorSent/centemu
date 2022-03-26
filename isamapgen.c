#include <stdio.h>


struct centinst {
	uint8_t opcode;
	uint8_t arg1;
	uint8_t arg2;
	uint8_t argc;
	uint8_t amode;
} centinst;

struct mnem_operation {
	char mnemonic[8];
	char description[64];
};

struct centamode {
	char mode[16];
	char mnem_format[32];


};

static char* mn_misc[]={"hlt","nop","f2?","f3?","f4?","f5?","f6?","fcc","f8?","ret","u?a","u?b","u?c","u?d","dly","u?f",0};
static char* mn_b[]={"bcs","bcc","bss","bsc","bzs","bzc","blt","bge","bgt","ble","bxs0","bxs1","bxs2","bxs3","b??0","b??1",0};
static char* mn_alub[]={"addb","subb","andb","orb","alu4b","alu5b","alu6b","alu7b",0};
static char* mn_aluw[]={"addw","subw","andw","orw","alu4w","alu5w","alu6w","alu7w",0};
static char* mn_jc[]={"jump","call",0};
static char* mn_ldst[]={"stxb","stxw","ldxb","ldxw","styb","styw","ldyb","ldyw",0};
static char* unknown="????";

char *parse_opcode(uint8_t op) {
	char *mn;
	unsigned char argb=0;
	switch( op >> 4 ) {
		case 0xf:
		case 0xe:
		case 0xd:
		case 0xc:
		case 0xb:
		case 0xa:
		case 0x9:
		case 0x8:
			mn=mn_ldst[(op&0x70)>>4];
			argb=(op&0x08)?0:(op&0x07)>2?1:(op&0x03)?2:(op&0x10)?2:1;
			break;
		case 0x7:
			mn=mn_jc[(op&0x08)>>3];
			argb=(op&0x07)>3?1:2;
			break;
		case 0x6:
			mn=unknown;
			break;
		case 0x5:
			mn=mn_aluw[(op&0x07)];
			argb=(op&0x08)?1:0;
			break;
		case 0x4:
			mn=mn_alub[(op&0x07)];
			argb=(op&0x08)?1:0;
			break;
		case 0x3:
		case 0x2:
			mn=unknown;
			break;
		case 0x1:
			mn=mn_b[(op&0x0f)];
			argb=1;
			break;
		case 0x0:
			mn=mn_misc[(op&0x0f)];
			argb=0;
			break;
	}
	return(mn);
}

int main() {
	unsigned int i,h,l;
	for(i=0; i<=0xff; i++) {
		printf("0x%0x\t%s\n", i, parse_opcode(i));
	}
	return(0);
}
