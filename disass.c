#include <stdio.h>


#define nibble2bcdb(n) ( (((n&0x8)>>3) * 1000) + (((n&0x4)>>2) * 100) + (((n&0x2)>>1) * 10) + (n&0x1))

enum opwidths {N,B,W,WB,BIT};
char *opwmods[]={"",".b",".w"};
char *opfmods[]={"s","c",""};
enum amodefams {ANONE, AREL, AALU, AMEM};

struct centinst {
	uint8_t opcode;
	uint8_t arg1;
	uint8_t arg2;
	uint8_t argc;
	uint8_t amode;
} centinst;

typedef struct mnem_operation {
	char *mnemonic;
	uint8_t argb;
	char description[64];
} op;

struct centamode {
	char mode[16];
	char mnem_format[32];
};

typedef struct stem_node {
	char *node;
	uint8_t offset;
	uint8_t mask;
} stem_node;

typedef struct stem {
	char *st;
	unsigned char opwidth;
	unsigned char amodetype;
	stem_node sn[];
} stem;




static stem mns_jump={"jump",N,AMEM,{{"",0x70,0x07},{0,0,0}}};
static stem mns_call={"call",N,AMEM,{{"",0x78,0x07},{0,0,0}}};
static stem mns_ret={"ret",N,ANONE,{{"i",0x0a,0x00},{"",0x09,0x00},{0,0,0}}};
static stem mns_ldz={"ldz",W,AMEM,{{"",0x68,0x07},{0,0,0}}};
static stem mns_stz={"stz",W,AMEM,{{"",0x60,0x07},{0,0,0}}};
static stem mns_ld={"ld",WB,AMEM,{{"x",0xa0,0x1f},{"y",0xe0,0x1f},{0,0,0}}};
static stem mns_st={"st",WB,AMEM,{{"x",0x80,0x1f},{"y",0xc0,0x1f},{0,0,0}}};
static stem mns_aluls={"ls",WB,AALU,{{"l",0x24,0x18},{"r",0x25,0x18},{0,0,0}}};
static stem mns_alur={"r",WB,AALU,{{"rc",0x26,0x18},{"lc",0x27,0x18},{0,0,0}}};
static stem mns_f={"f",BIT,ANONE,{{"s",0x02,0x1},{"i",0x04,0x01},{"c",0x06,0x01},{"ac",0x08,0x00},{0,0,0}}};
static stem mns_b={"b",BIT,AREL,{
	{"c",0x10,0x1},{"s",0x12,0x01},{"z",0x14,0x01},
	{"lt",0x16,0x00},{"ge",0x17,0x00},{"gt",0x18,0x00},{"le",0x19,0x00},
	{"xs",0x1a,0x03},
	{"b0x1b",0x1b,0x00},{"b0x1c",0x1c,0x00},{"b0x1d",0x1d,0x00},
	{"b0x1e",0x1e,0x00},{"b0x1f",0x1f,0x00},{0,0,0}
}};

static stem mns_alumisc={"",WB,AALU,{{"inc",0x20,0x18},{"dec",0x21,0x18},{"clr",0x22,0x18},{"not",0x23,0x18},{0,0,0}}};
static stem mns_alubi={"",WB,AALU,{
	{"add",0x40,0x18},{"sub",0x41,0x18},{"and",0x42,0x18},{"or",0x43,0x18},
	{"xor",0x44,0x18},{"mov",0x45,0x18},{"alu0x6",0x46,0x18},{"alu0x7",0x47,0x18},{0,0,0}
}};
static stem mns_misc={"",N,ANONE,{
	{"hlt",0x00,0x00},{"nop",0x01,0x00},{"dly",0x0e,0x00},
	{"0x0b",0x0b,0x00},{"0x0c",0x0c,0x00},{"0x0d",0x0d,0x00},{"0x0f",0x0f,0x00},{0,0,0}
}};
static stem *mn_stems[]= {
	&mns_jump,
	&mns_call,
	&mns_ret,
	&mns_ldz,
	&mns_stz,
	&mns_ld,
	&mns_st,
	&mns_aluls,
	&mns_alur,
	&mns_f,
	&mns_b,
	&mns_alumisc,
	&mns_alubi,
	&mns_misc,
	0
};

static char* mn_misc[]={"hlt","nop","fss","fsc","fis","fic","fcs","fcc","fac","ret","u?a","u?b","u?c","u?d","dly","u?f",0};
static char* mn_b[]={"bcs","bcc","bss","bsc","bzs","bzc","blt","bge","bgt","ble","bxs0","bxs1","bxs2","bxs3","b??0","b??1",0};
static char* mn_alub[]={"addb","subb","andb","orb","alu4b","alu5b","alu6b","alu7b",0};
static char* mn_aluw[]={"addw","subw","andw","orw","alu4w","alu5w","alu6w","alu7w",0};
static char* mn_jc[]={"jump","call",0};
static char* mn_ldst[]={"stxb","stxw","ldxb","ldxw","styb","styw","ldyb","ldyw",0};
static char* none="";
static char* unknown="????";

char * mn_list[0x10][0x10]={
	{"hlt","nop","fss","fsc","fis","fic","fcs","fcc","fac","ret","u?a","u?b","u?c","u?d","dly","u?f"},
	{"bcs","bcc","bss","bsc","bzs","bzc","blt","bge","bgt","ble","bxs0","bxs1","bxs2","bxs3","b??0","b??1"},
	{"incb","decb","clrb","notb","lsrb","lslb","rrcb","rlcb", 0,0,0,0, 0,0,0,0},
	{"incw","decw","clrw","notw","lsrw","lslw","rrcw","rlcw", 0,0,0,0, 0,0,0,0},
	{"addb!","subb!","andb!","orb!","alu4b!","alu5b!","alu6b!","alu7b!","addb","subb","andb","orb","alu4b","alu5b","alu6b","alu7b"},
	{"addw!","subw!","andw!","orw!","alu4w!","alu5w!","alu6w!","alu7w!","addw","subw","andw","orw","alu4w","alu5w","alu6w","alu7w"},
	{"stzw","ldzw",0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
	{"jump","call",0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
	{"stxb",0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
	{"stxw",0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
	{"ldxb",0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
	{"ldxw",0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
	{"styb",0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
	{"styw",0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
	{"ldyb",0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
	{"ldyw",0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0}

};

static char *am_mem_list[0x10] = {"#D","A","[A]","PC+N","[PC+N]","_[R]_","?R?","?R?","R0","R1","R2","R3","R4","R5","R6","R7"};
static char *am_alu_list[0x02] = {"R,R", "!"};
static char *am_rel[0x01] = {"PC+N"};
static char *am_none[0x01] = {""};

typedef struct amfam {
	uint8_t mask;
	uint8_t shift;
	char *ams[];
} amfam;

static amfam amfam_none={0x00,0,{""}};
static amfam amfam_rel={0x00,0,{"PC+N"}};
static amfam amfam_alu={0x08,3,{"R,R","!"}};
static amfam amfam_mem={0x0f,0,{"#D","A","[A]","PC+N","[PC+N]","_[R]_","?R?","?R?","R0","R1","R2","R3","R4","R5","R6","R7"}};
amfam *amfams[]={&amfam_none,&amfam_rel,&amfam_alu,&amfam_mem};


uint8_t find_am_argc(uint8_t am, uint8_t af, uint8_t ds) {
	switch(af) {
		case AMEM:
			if(!am) { return(ds); }
			else if(am<3) { return(W); }
			else { return(B); }
		case AALU:
			return(am?0:1);
		case AREL:
			return(1);
		default:
			return(0);
	}
}


char *find_stem(uint8_t op) {
	stem *st;
	stem_node *sn=0;
	unsigned char i,j,ow=0,f=0,argc=0,aft,am;
	char *opmod, *amode;
	amfam *af;
	for(i=0;mn_stems[i]!=0;i++){
		st=mn_stems[i];
		// printf("%u: %s\n",i,st->st);
		for(j=0;st->sn[j].node;j++){
			sn=&st->sn[j];
			// printf("%u: %s, %02x m%02x - %02x\n",j,sn->node, sn->offset, sn->mask, (op & ~sn->mask));
			if((op & ~sn->mask)  == sn->offset) {
				f=1;
				af=amfams[st->amodetype];
				am=(op&af->mask&sn->mask)>>af->shift;
				amode=af->ams[am];
				ow=st->opwidth;
				switch(ow) {
					case BIT:
						opmod=sn->mask?opfmods[op&0x01]:opfmods[2];
						break;
					case WB:
						ow=(op&0x10)?W:B;
						opmod=opwmods[ow];
						break;
					case W:
						opmod=opwmods[W];
						break;
					default:
						opmod=opwmods[N];
						break;
				}

				argc=find_am_argc(am,st->amodetype,ow);
				printf("%02x  %04u  %04u    ARGC:%u\t%s%s%s\t%s\n",op,nibble2bcdb((op&0xf0)>>4),nibble2bcdb(op&0x0f),argc,st->st,sn->node,opmod,amode);
				goto done;
			}
		}
	}
done:
	return(st->st);
}

int parse_opcode(uint8_t op, char **mn, char **am) {
	uint8_t oph=(op >> 4);
	unsigned char argb=0;
	*mn=unknown;
	*am=am_none[0];
	switch( oph ) {
		case 0xf:
		case 0xe:
		case 0xd:
		case 0xc:
		case 0xb:
		case 0xa:
		case 0x9:
		case 0x8:
			*mn=mn_list[oph][0];
			*am=am_mem_list[(op&0xf)];
			argb=(op&0x08)?0:(op&0x07)>2?1:(op&0x03)?2:(op&0x10)?2:1;
			break;
		case 0x7:
			*mn=mn_list[oph][(op&0x08)>>3];
			*am=am_mem_list[(op&0x7)];
			argb=(op&0x07)>3?1:2;
			break;
		case 0x6:
			*mn=mn_list[oph][(op&0x08)>>3];
			*am=am_mem_list[(op&0x7)];
			argb=(op&0x07)>2?1:2;
			break;
		case 0x5:
		case 0x4:
			*mn=mn_list[oph][(op&0x0f)];
			*am=am_alu_list[(op&0x8)>>3];
			argb=(op&0x08)?1:0;
			break;
		case 0x3:
		case 0x2:
			*mn=mn_list[oph][(op&0x07)];
			*am=am_alu_list[(op&0x8)>>3];
			argb=(op&0x08)?1:0;
			break;
		case 0x1:
			argb=1;
			*am=am_rel[0];
		case 0x0:
			*mn=mn_list[oph][(op&0x0f)];
			break;
	}
	return(argb);
}


int main() {
	char **mn, **am;
	unsigned int i,args;
	unsigned short h,l;
	for(i=0; i<=0xff; i++) {
		h=nibble2bcdb(i>>4);
		l=nibble2bcdb(i&0xf);
		args=parse_opcode(i,mn,am);
	//	printf("0x%02x\t %04u %04u\t%u\t%s\t%s\n", i, h,l,args, *mn, *am);
		find_stem(i);
	}

	//printf("Test: %s%s %02x %02x", mn_stems[1]->st, mn_stems[1]->sn[0].node, mn_stems[1]->sn[0].offset, mn_stems[1]->sn[0].mask);
	  return(0);
}
