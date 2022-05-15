#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pty.h>

#define swap_bytes(n) (((((uint16_t)(n) & 0xFF)) << 8) | (((uint16_t)(n) & 0xFF00) >> 8))

#if BYTE_ORDER == BIG_ENDIAN
# define HTONS(n) (n)
# define NTOHS(n) (n)
#else
# define HTONS(n) swap_bytes(n)
# define NTOHS(n) swap_bytes(n)
#endif

#define htons(n) HTONS(n)
#define ntohs(n) NTOHS(n)

#define TRAP(address, ... ) { fprintf(stderr,"\nTRAP at address %#06x:",address); fprintf(stderr, __VA_ARGS__); halted=1; }

char *REG2NAME(uint8_t reg, uint8_t size);
uint8_t mmio_mux_writeb(uint16_t ioaddr, uint8_t val);
uint8_t mmio_mux_readb(uint16_t ioaddr);
uint8_t mmio_diag_writeb(uint16_t ioaddr, uint8_t val);
uint8_t mmio_diag_readb(uint16_t ioaddr);

typedef union status_word_t {
	uint16_t word;
	struct {
		uint16_t intlevel:4;
		uint16_t I:1; /* Interrupts Enabled */
		uint16_t F:1; /* Flag bit */
		uint16_t L:1; /* Link bit */
		uint16_t M:1; /* Minus bit */
		uint16_t V:1; /* Overflow bit */
		uint16_t C:1; /* Carry bit? */
		uint16_t Z:1; /* Zero bit? */

	};
} status_word_t;

typedef union regset_t {
	int16_t w[8];
	int8_t b[16];
	struct {
		int8_t R0,R1, R2,R3, R4,R5, R6,R7, R8,R9, RA,RB, RC,RD, RE,RF;	
	};
	struct {
		int16_t A,B,X,Y,Z,S,F,PC;
	};
} regset_t;

typedef union regmem_t {
	uint16_t w[128];
	uint8_t b[256];
	regset_t i[16];
} regmem_t;

typedef union memmap_t {
	uint8_t a[0x10000];
	struct {
		regmem_t r; /* 0x0000 - 0x00ff */
		int8_t user[0xefff-0x00ff]; /* 0x0100 - 0xeffff */
		union { /* 0xf000 - 0xffff */
			int8_t mmio[0x1000];
		};
	};
} memmap_t;

/* Instruction map */

typedef struct inst_node_t {
	char *node;
	uint8_t offset;
	uint8_t mask;
} inst_node_t;

typedef struct inst_base_t {
	char *base;
	uint8_t op_type;
	uint8_t data_type;
	uint8_t amode_type;
	inst_node_t n[];
} inst_base_t;

enum op_types { OT_SYS, OT_FLOW_COND, OT_FLOW_JUMP, OT_FLOW_CALL, OT_ALU1, OT_ALU2, OT_LOAD, OT_STORE };
enum op_dsize { DS_N, DS_B, DS_W, DS_WB, DS_BIT };
enum amode_types { ANONE, AREL, AALU, AMEM, ADMA, ATREG };
static inst_base_t ib_tregs={"t", OT_ALU2,DS_W,ATREG,{
	{"ay",0x5c,0x00},{"ab",0x5d,0x00},{"az",0x5e,0x00},{"as",0x5f,0x00},{0,0,0}
}};

static inst_base_t ib_dma={"dma", OT_SYS,DS_B,ADMA,{{"",0x2f,0x00},{0,0,0}}};

static inst_base_t ib_jump={"jump",OT_FLOW_JUMP,DS_N,AMEM,{{"",0x70,0x07},{0,0,0}}};

static inst_base_t ib_call={"call",OT_FLOW_CALL,DS_N,AMEM,{{"",0x78,0x07},{0,0,0}}};

static inst_base_t ib_reti={"reti",OT_SYS,DS_N,ANONE,{{"",0x0a,0x00},{0,0,0}}};
static inst_base_t ib_ret={"ret",OT_SYS,DS_N,ANONE,{{"",0x09,0x00},{0,0,0}}};

static inst_base_t ib_stz={"st",OT_STORE,DS_W,AMEM,{{"x",0x68,0x07},{0,0,0}}};

static inst_base_t ib_ldz={"ld",OT_LOAD,DS_W,AMEM,{{"x",0x60,0x07},{0,0,0}}};

static inst_base_t ib_st={"st",OT_STORE,DS_WB,AMEM,{{"a",0xa0,0x1f},{"b",0xe0,0x1f},{0,0,0}}};

static inst_base_t ib_ld={"ld",OT_LOAD,DS_WB,AMEM,{{"a",0x80,0x1f},{"b",0xc0,0x1f},{0,0,0}}};

static inst_base_t ib_aluls={"ls",OT_ALU1,DS_WB,AALU,{{"r",0x24,0x18},{"l",0x25,0x18},{0,0,0}}};

static inst_base_t ib_alur={"r",OT_ALU1,DS_WB,AALU,{{"rc",0x26,0x18},{"lc",0x27,0x18},{0,0,0}}};

static inst_base_t ib_f={"f",OT_SYS,DS_BIT,ANONE,{{"s",0x02,0x1},{"i",0x04,0x01},{"c",0x06,0x01},{"ac",0x08,0x00},{0,0,0}}};
static inst_base_t ib_fac={"fac",OT_SYS,DS_N,ANONE,{{"",0x08,0x00},{0,0,0}}};

static inst_base_t ib_bf={"b",OT_FLOW_COND,DS_BIT,AREL,{
	{"c",0x10,0x1},{"s",0x12,0x01},{"z",0x14,0x01},{0,0,0}
}};

static inst_base_t ib_b={"b",OT_FLOW_COND,DS_N,AREL,{
	{"gtz",0x16,0x00},{"gez",0x17,0x00},
	{"ltz",0x18,0x00},{"lez",0x19,0x00},
	{"xs0",0x1a,0x00}, {"xs1",0x1b,0x00},
	{"xs2",0x1c,0x00},{"xs3",0x1d,0x00},
	{"b0x1e",0x1e,0x00},{"b0x1f",0x1f,0x00},
	{0,0,0}
}};

static inst_base_t ib_alumisc={"",OT_ALU1,DS_WB,AALU,{{"inc",0x20,0x18},{"dec",0x21,0x18},{"clr",0x22,0x18},{"not",0x23,0x18},{0,0,0}}};

static inst_base_t ib_alubi={"",OT_ALU2,DS_WB,AALU,{
	{"add",0x40,0x18},{"sub",0x41,0x18},{"and",0x42,0x18},{"or",0x43,0x18},
	{"xor",0x44,0x18},{"mov",0x45,0x18},{"alu0x6",0x46,0x18},{"alu0x7",0x47,0x18},{0,0,0}
}};

static inst_base_t ib_misc={"",OT_SYS,DS_N,ANONE,{
	{"hlt",0x00,0x00},{"nop",0x01,0x00},{"dly",0x0e,0x00},
	{"0x0b",0x0b,0x00},{"0x0c",0x0c,0x00},{"0x0d",0x0d,0x00},{"0x0f",0x0f,0x00},{0,0,0}
}};

static inst_base_t *inst_bases[]= {
	&ib_tregs,
	&ib_dma,
	&ib_jump,
	&ib_call,
	&ib_reti,
	&ib_ret,
	&ib_ldz,
	&ib_stz,
	&ib_ld,
	&ib_st,
	&ib_aluls,
	&ib_alur,
	&ib_f,
	&ib_fac,
	&ib_bf,
	&ib_b,
	&ib_alumisc,
	&ib_alubi,
	&ib_misc,
	0
};



/* Active program counter */
volatile uint16_t PC;

uint8_t halted=1;
uint8_t sense[4]={0,0,0,0};

#define NUMBANKS 2
uint8_t ilevel=0;
memmap_t membank[NUMBANKS];
memmap_t *mem = &membank[0];
regmem_t *cpuregs;
regset_t *cpuregset;
status_word_t status;

char *regnames8[] = { "A0","A1","B0","B1","X0","X1","Y0","Y1","Z0","Z1","S0","S1","F0","F1","PC0","PC1" };
char *regnames16[] = { "A","B","X","Y","Z","S","F","PC" };

char *REG2NAME(uint8_t reg, uint8_t size) {
	switch(size) {
		case DS_B: return(regnames8[reg]);
		case DS_W: default: return(regnames16[reg>>1]);
	}	    
}
uint8_t NAME2REG(char * regname) {
	//fprintf(stderr,"Looking up number for register '%s' (%0x)",regname,*regname);
	uint8_t reg=0;
	char *c=regname;
	unsigned char l=tolower(*(c++));
	switch(l) {
		case 'a': reg=0x0; break;
		case 'b': reg=0x2; break;
		case 'x': reg=0x4; break;
		case 'y': reg=0x6; break;
		case 'z': reg=0x8; break;
		case 's': reg=0xa; break;
		case 'f': reg=0xc;
		case 'p': if (tolower(*(c++))=='c') { reg=0xe; } else { return(0xff);}; break;
		case 'r': if(*c < 0x10) { return(*c); }
			  l=tolower(*c);
			  if(l > 'f') { return(0xff); }
			  if(l >= 'a'){ return(l-'a'); }
			  if(l>'9' || l < '0') { return(0xff); }
			  return(l-'0');
	}
	switch(*c) {
		case '0': case 'H': case 'h':
			//fprintf(stderr," found %u(H)",reg);
			return(reg); break;
		case '1': case 'L': case 'l': default:
			//fprintf(stderr," found %u(L)",reg+1);
			return(reg+1); break;
	}
}


/* Read byte from register at address src */
uint8_t regmem_readb(uint8_t src) {
	return(mem->r.b[src]);
}

/* Write byte to register at address dst */
void regmem_writeb(uint8_t dst, uint8_t val) {
	for(uint8_t b=0;b<NUMBANKS;b++) {
		membank[b].r.b[dst]=val;	
	}
	return;	
}

/* Read word from register at address dst, converted from LE to host endian */
uint16_t regmem_readw(uint8_t src) {
	return(ntohs(mem->r.w[src>>1]));
}

/* Write word to register at address dst, converting from host endian to LE */
void regmem_writew(uint8_t dst, uint16_t val) {
	uint16_t bev=htons(val);
	for(uint8_t b=0;b<NUMBANKS;b++) {
		membank[b].r.w[dst>>1]=bev;	
	}
	return;	
}

/* Read by from currently mapped bank of user memory */
uint8_t usermem_readb(uint16_t src) {
	return(mem->a[src]);
}

/* Write byte to currently mapped bank of user memory */
void usermem_writeb(uint16_t dst, uint8_t val) {
	mem->a[dst]=val;
	return;
}

/* This will get special handling later */
uint8_t mmio_readb(uint16_t src) {
	if( src >= 0xf200 && src < 0xf300 ) { return(mmio_mux_readb(src)); }
	else if (src >= 0xf100 && src <= 0xf110 ) { return(mmio_diag_readb(src)); }
	return(mem->a[src]);
}

/* This will get special handling later */
void mmio_writeb(uint16_t dst, uint8_t val) {
	if( dst >= 0xf200 && dst < 0xf300 ) { mmio_mux_writeb(dst, val); }
	else if (dst >= 0xf100 && dst <= 0xf110 ) { mmio_diag_writeb(dst, val); }
	return;
}


/* Write byte to memory address dst, handle register and MMIO address ranges */
void mem_writeb(uint16_t dst, uint8_t val) {
	if(dst < 0x100) {
		regmem_writeb(dst,val); /* Registers */
	} else if(dst<0xf000) {
		usermem_writeb(dst,val); /* User memory */
	} else {
		mmio_writeb(dst,val); /* MMIO */
	}
}

/* Read a byte from memory address src */
uint8_t mem_readb(uint16_t src) {
	return(src<0x0100?regmem_readb(src):src<0xf000?usermem_readb(src):mmio_readb(src));
}
/* Read a word from memory as two bytes */
uint16_t mem_readw(uint16_t src) {
	return((mem_readb(src)<<8) | mem_readb(src+1));
}
/* Write a word to memory as two bytes */
void mem_writew(uint16_t dst, uint16_t val) {
	mem_writeb(dst,(val&0xff00)>>8);
	mem_writeb(dst+1, (val&0x00ff));
	return;
}

/* Read byte from src and write to dst */
void mem_movb(uint16_t src, uint16_t dst) {
	mem_writeb(dst,mem_readb(src));
	return;
}

/* Read byte from specified register number in current interrupt level */
uint8_t reg_readb(uint8_t reg) {
	return(regmem_readb(reg+ilevel*0x10));
}

/* Write byte to specified register number in current interrupt level */
void reg_writeb(uint8_t reg, uint8_t val) {
	regmem_writeb(reg+ilevel*0x10,val);
	return;
}

/* Read word from specified register number in current interrupt level */
uint16_t reg_readw(uint8_t reg) {
	return(regmem_readw(reg+ilevel*0x10));
}

/* Write word to specified register number in current interrupt level */
void reg_writew(uint8_t reg, uint16_t val) {
	regmem_writew(reg+ilevel*0x10,val);
	return;
}

/* Load byte to specified 8 bit register in current interrupt level from src */
void reg_ldb(uint8_t reg, uint16_t src) {
	reg_writeb(reg,mem_readb(src));
	return;
}

/* Store byte from specified 8 bit register in current interrupt level to dst */
void reg_stb(uint8_t reg, uint16_t dst) {
	mem_writeb(dst,reg_readb(reg));
	return;
}

/* Load word to specified 16 bit register in current interrupt level from src */
void reg_ldw(uint8_t reg, uint16_t src) {
	reg_ldb(reg,src);
	reg_ldb(reg+1,src+1);
	return;
}

/* Store byte from specified 16 bit register in current interrupt level to dst */
void reg_stw(uint8_t reg, uint16_t dst) {
	reg_stb(reg,dst);
	reg_stb(reg+1,dst+1);
	return;
}

/* Copy byte value from src register to dst register */
void reg_movb(uint8_t srcreg, uint8_t dstreg) {
	reg_writeb(dstreg,reg_readb(srcreg));
	return;
}

/* Copy word value from src register to dst register */
void reg_movw(uint8_t srcreg, uint8_t dstreg) {
	reg_writew(dstreg,reg_readw(srcreg));
	return;
}

/* Push contents of srcreg to stack in memory indexed by idxreg */
void reg_pushw(uint8_t srcreg, uint8_t idxreg) {
	uint16_t pos;
	pos=reg_readw(idxreg)-2;
	reg_writew(idxreg,pos);
	mem_writew(pos,reg_readw(srcreg));
}

/* Pop contents of stack indexed by idxreg to dstreg */
void reg_popw(uint8_t dstreg, uint8_t idxreg) {
	uint16_t pos;
	pos=reg_readw(idxreg);
	reg_writew(dstreg,mem_readw(pos));
	reg_writew(idxreg,pos+2);
}

/* Change interrupt level */
void interrupt(uint8_t level) {
	cpuregset=&(cpuregs->i[level]);
	return;
}


/* Ordered list of ALU operations */
/* Single register */
enum alu_ops1 {
	ALU_INC,ALU_DEC,
	ALU_CLR,ALU_NOT,
	ALU_LSR,ALU_LSL,
	ALU_RRC,ALU_RLC
};
/* Two register ALU ops, 0x5c and above are all implict moves */
enum alu_ops2 {
	ALU_ADD,ALU_SUB,
	ALU_AND,ALU_OR,
	ALU_XOR,ALU_MOV,
	ALU_U0x6,ALU_U0x7
};

/* Handle byte size single register ALU instructions */
void alu_op1regb(uint8_t op, uint8_t reg, uint8_t opt) {
	uint8_t arg,res;
	uint16_t tmp;
	/* Read argument byte from specified register */
	arg=reg_readb(reg);
	switch(op&0x07) {
		case ALU_INC: res=arg+(1+opt); status.V=(res<arg)?1:0; break;
		case ALU_DEC: res=arg-(1+opt); status.V=(res>arg)?1:0; break;
		case ALU_CLR: res=0; break;
		case ALU_NOT: res=~arg; break;
		case ALU_LSR: res=arg>>(1+opt); status.C=((arg>>opt)&0x01)?1:0; 
			      fprintf(stderr,"LSR[%x]: %x -> %x C=%u",reg,arg,res,status.C);break;
		case ALU_LSL: res=arg<<(1+opt); status.C=((arg<<opt)&0x80)?1:0; break;
		case ALU_RLC: /* Dup arg bits, shift up, take the high word and shift back down */
			      /* 01234567C -> 1234567C01234567*/
			      tmp=(arg << 8) | ((status.C&0x1) << 7) | (arg>>1);
			      tmp<<=(1+opt);
			      tmp=(tmp&0xff00)>>8;
			      //tmp= ( ( (arg<<8|arg)<<(1+opt) )  & 0xff00 ) >> 8;
			      res=(tmp&0xff);
			      status.C= ( (arg<<opt) & 0x80 )?1:0; break;
			      fprintf(stderr,"RLC[%s]: %x -> %x C=%u",REG2NAME(reg,DS_B),arg,res,status.C);
			      break;
		case ALU_RRC: /* Dup arg bits, shift down, take the low word */
			      tmp=(arg << 9) | ((status.C&0x1) << 8) | arg;
			      tmp>>=(1+opt);
			      tmp=tmp&0x00ff;
			      //tmp= ( (arg<<8|arg) & 0x00ff ) >> (1+opt);
			      res=(tmp&0xff);
			      status.C= ( (arg>>opt) & 0x01 )?1:0;
			      fprintf(stderr,"RLC[%s]: %x -> %x C=%u",REG2NAME(reg,DS_B),arg,res,status.C);
			      break;
	}
	/* Set zero and minus flags appropriately*/
	status.Z=res?0:1;
	status.M=(res&0x80)?1:0;
	fprintf(stderr,"\nALU1b: reg[%s]=%#04x opt=%#04x op=%02x, res=%#04x\n",
			REG2NAME(reg,DS_B),arg, opt, op,res);
	reg_writeb(reg,res);
	return;
}

/* Handle word size single register ALU instructions */
void alu_op1regw(uint8_t op, uint8_t reg, uint8_t opt) {
	uint16_t arg,res;
	uint32_t tmp;
	/* Read argument byte from specified register */
	arg=reg_readw(reg);
	switch(op&0x07) {
		case ALU_INC: res=arg+(1+opt); status.V=(res<arg)?1:0; break;
		case ALU_DEC: res=arg-(1+opt); status.V=(res>arg)?1:0; break;
		case ALU_CLR: res=0; break;
		case ALU_NOT: res=~arg; break;
		case ALU_LSR: res=arg>>(1+opt); status.C=((arg>>opt)&0x0001)?1:0; break;
		case ALU_LSL: res=arg<<(1+opt); status.C=((arg<<opt)&0x8000)?1:0; break;
		case ALU_RLC: /* Dup arg bits, shift up, take the high word and shift back down */
			      tmp=(arg << 16) | ((status.C&0x1) << 15) | (arg >> 1);
			      tmp<<=(1+opt);
			      tmp=(tmp&0xffff0000)>>16;
			      //tmp= ( ( (arg<<16|arg)<<(1+opt) ) & 0xffff0000 ) >> 16;
			      res=(tmp&0xffff);
			      status.C= ( (arg<<opt) & 0x8000 )?1:0;
			      fprintf(stderr,"RLC[%s]: %x -> %x C=%u",REG2NAME(reg,DS_W),arg,res,status.C);
			      break;
		case ALU_RRC: /* Dup arg bits, shift down, take the low word */
			      tmp=(arg << 17) | ((status.C&0x1) << 16) | arg;
			      tmp>>=(1+opt);
			      tmp=tmp&0x0000ffff;
			      //tmp= ( (arg<<16|arg)>>(1+opt) ) & 0x0000ffff;
			      res=(tmp&0xffff);
			      status.C= ( (arg>>opt) & 0x0001 )?1:0;
			      fprintf(stderr,"RRC[%s]: %x -> %x C=%u",REG2NAME(reg,DS_W),arg,res,status.C);
			      break;
	}
	/* Set zero and minus flags appropriately*/
	status.Z=res?0:1;
	status.M=(res&0x8000)?1:0;
	fprintf(stderr,"\nALU1w: reg[%s]=%#06x opt=%#04x, op=%02x, res=%#06x\n",
			REG2NAME(reg,DS_W),arg, opt, op,res);
	
	reg_writew(reg,res);
	return;
}

/* Handle byte size two register ALU instructions */
void alu_op2regb(uint8_t op, uint8_t srcreg, uint8_t dstreg) {
	uint8_t srcarg,dstarg,res;
	uint16_t tmp;
	/* Read argument bytes from specified registers */
	srcarg=reg_readb(srcreg);
	dstarg=reg_readb(dstreg);

	switch(op&0x07) {
		case ALU_ADD: case ALU_ADD|0x8:
			tmp=dstarg+srcarg;
			res=dstarg+srcarg;
			status.C=(tmp&0x0100)?1:0;
			/* 2901 ALU sets overflow when C_n^C_(n-1) */
			/* Check if src and dest have the same high bit */
			/* Then see if there was a carry in to the high bit */
			status.V=( (srcarg&0x80)==(dstarg&0x80) && (res&0x80)!=(srcarg&0x80))?1:0;
			//status.V=(~((srcarg&0x80)^(dstarg&0x80)) & ((res&0x80)^(srcarg&0x80)))?1:0;
			break;
		case ALU_SUB: case ALU_SUB|0x8:
			tmp=dstarg+~srcarg+1;
			res=dstarg+~srcarg+1;
			status.C=(tmp&0x0100)?1:0;
			status.V=( (srcarg&0x80)==(dstarg&0x80) && (res&0x80)!=(srcarg&0x80))?1:0;
			//status.V=(~((res&0x80)^(dstarg&0x80)) & ((dstarg&0x80)^(srcarg&0x80)))?1:0;
			break;
		case ALU_AND: case ALU_AND|0x8: res=dstarg&srcarg; break;
		case ALU_OR: case ALU_OR|0x8: res=dstarg|srcarg; break;
		case ALU_XOR: res=dstarg^srcarg; break;
		case ALU_MOV: res=srcarg; break;
		default:
			fprintf(stderr,"\nALU2b - Unknown operation number %#04x\n",op);
			res=srcarg; break;
	}

	/* Set zero and minus flags appropriately*/
	status.Z=res?0:1;
	status.M=(res&0x80)?1:0;
	fprintf(stderr,"\nALU2b: srcrreg[%s]=%#04x dstreg[%s]=%#04x op=%02x, res=%#04x\n",
			REG2NAME(srcreg,DS_B),srcarg, REG2NAME(dstreg,DS_B),dstarg, op,res);
	 
	reg_writeb(dstreg,res);
	return;
}

/* Handle word size two register ALU instructions */
void alu_op2regw(uint8_t op, uint8_t srcreg, uint8_t dstreg) {
	uint16_t srcarg,dstarg,res;
	uint32_t tmp;
	/* Read argument bytes from specified registers */
	srcarg=reg_readw(srcreg);
	dstarg=reg_readw(dstreg);
	switch(op&0x07) {
		case ALU_ADD:
			tmp=dstarg+srcarg;
			res=dstarg+srcarg;
			status.C=(tmp&0x00010000)?1:0;
			status.V=( (srcarg&0x8000)==(dstarg&0x8000) && (res&0x8000)!=(srcarg&0x8000))?1:0;
			//status.V=(~((srcarg&0x8000)^(dstarg&0x8000)) & ((res&0x8000)^(srcarg&0x8000)))?1:0;
			break;
		case ALU_SUB:
			tmp=dstarg+~srcarg+1;
			res=dstarg+~srcarg+1;
			status.C=(tmp&0x00010000)?1:0;
			status.V=( (srcarg&0x8000)==(dstarg&0x8000) && (res&0x8000)!=(srcarg&0x8000))?1:0;
			//status.V=(~((res&0x8000)^(dstarg&0x8000)) & ((dstarg&0x8000)^(srcarg&0x8000)))?1:0;
			break;
		case ALU_AND: res=dstarg&srcarg; break;
		case ALU_OR: res=dstarg|srcarg; break;
		case ALU_XOR: res=dstarg^srcarg; break;
		case ALU_MOV: res=srcarg; break;
		default:
			fprintf(stderr,"\nALU2w - Unknown operation number %#04x\n",op);
			res=srcarg; break;
	}
	/* Set zero and minus flags appropriately*/
	status.Z=res?0:1;
	status.M=(res&0x8000)?1:0;
	fprintf(stderr,"\nALU2w: srcrreg[%s]=%#06x dstreg[%s]=%#06x op=%02x, res=%#06x\n",
			REG2NAME(srcreg,DS_W),srcarg, REG2NAME(dstreg,DS_W),dstarg, op,res);
	 
	reg_writew(dstreg,res);
	return;
}




/* Halt the CPU */
void sys_halt() {
	halted=1;
	return;
}
/* Return from interrupt */
void flow_reti() {
	return;
}

/* Unconditional branch instructions */
void flow_jump(uint16_t dst) {
	PC=dst;
	fprintf(stderr,"\n Jumping to %#06x\n",PC);
	return;
}

/* Function call */
void flow_call(uint16_t dst) {
	uint16_t tmpX,tmpS;
	//if( !reg_readw(4) & !status.L ) { 
	/* If the X register is empty and we're not nested, make a cheap call */
	//	fprintf(stderr,"Link flag clear and X empty, making cheap call to %#06x\n",dst);
	//} else {
	/* If the X register is not empty or link flag is set, push it to the stack and set the link flag */
	//	tmpX=reg_readw(0x4);
	//	tmpS=reg_readw(0xa);
	//	mem_writeb(tmpS-1,(tmpX&0xff00)>>8);
	//	mem_writeb(tmpS-2,tmpX&0x00ff);
	//	mem_writeb(tmpS-3,status.L?mem_readb(S)+1:0);
	//	reg_writew(0xa,tmpS-3);
	//	status.L=1;
	//	fprintf(stderr,"Link flag set, pushing %#06x to SP(%#06x) and calling %#06x\n",
	//			tmpX,(tmpX&0xff00)>>8,tmpX&0xff,tmpS-2,dst);
	//}
	reg_pushw(0x4,0xa);
	reg_writew(0x4,PC);
	PC=dst;
	return;
}


/* Return from function call */
void flow_ret() {
	uint8_t depth;
	uint16_t tmpS, tmpX;
	PC=reg_readw(0x4);
	reg_popw(0x4,0xa);
	/* If the link flag is not set, return to X and clear it */
	//if(!status.L) {
	//	reg_writew(4,0x0);
	//} else {
	//	tmpS=reg_readw(0xa);
	//	status.L=mem_readb(S+2)?1:0;
	//	tmpX=(mem_readb(tmpS+1) << 8 ) | mem_readb(tmpS);
	//	reg_writew(0xa,tmpS+3);
	//	fprintf(stderr,"Link flag not set, popping %#06x from SP(%#06x)\n",tmpX,tmpS);
	//	reg_writew(0x4,tmpX);
	//}
	return;
}



/* This needs to delay 4.5ms */
void sys_delay() {
	return;
}

void sys_dma() {
	fprintf(stderr,"DMA instruction\n");
}
enum sys_functions {
	SYS_HALT, SYS_NOP,
	FLAG_MINUS_SET, FLAG_MINUS_CLEAR,
	FLAG_INT_SET, FLAG_INT_CLEAR,
	FLAG_CARRY_SET, FLAG_CARRY_CLEAR,
	FLAG_ALL_CLEAR, FLOW_RET,
	FLOW_RETI, SYS_0x0b,
	SYS_0x0c, SYS_0x0d,
	SYS_DELAY, SYS_0x0f, SYS_DMA=0x2f
};

/* System functions */
void sys_func(uint8_t op) {
	switch(op) {
		case SYS_HALT: sys_halt(); break;
		case SYS_NOP: break;
		case FLAG_MINUS_SET: status.M=1; break;
		case FLAG_MINUS_CLEAR: status.M=0; break;
		case FLAG_INT_SET: status.I=1; break;
		case FLAG_INT_CLEAR: status.I=0; break;
		case FLAG_CARRY_SET: status.C=1; break;
		case FLAG_CARRY_CLEAR: status.C=0; break;
		case FLOW_RET: flow_ret(); break;
		case FLOW_RETI: flow_reti(); break;
		case SYS_DELAY: sys_delay(); break;
		case SYS_DMA: sys_dma(); break;
		default: break;
	}
	return;
}

enum branch_rel_cond {
	BCOND_CS,BCOND_CC,
	BCOND_MS,BCOND_MC,
	BCOND_ZS,BCOND_ZC,
	BCOND_GTZ,BCOND_GEZ,
	BCOND_LTZ,BCOND_LEZ,
	BCOND_XS0, BCOND_XS1,
	BCOND_XS2, BCOND_XS3,
	BCOND_0x1e, BCOND_0x1f
};

/* Conditional branch instructions */
void flow_cond_rel(uint8_t op, int8_t offset) {
	uint8_t res=0;
	/* Handle logical pairs */
	if((op&0xf)<BCOND_GTZ) {
		switch(op&0xe) {
			case BCOND_CS: res=(status.C)?1:0; break;
			case BCOND_MS: res=(status.M)?1:0; break;
			case BCOND_ZS: res=(status.Z)?1:0; break;
			// These would require the overflow flag, which doesn't appear to be used in practice
			//case BCOND_LT: res=(status.Z)?0:((status.M^status.V)?1:0); break;
			//case BCOND_GT: res=(status.Z)?0:((status.M^status.V)?0:1); break;
		}
		/* Inverted versions of the above */
		res=(op&0x01)?(res?0:1):(res?1:0);
	} else {
	/* Handle the remaining ops */
		switch(op&0xf) {
			case BCOND_GTZ: res=(status.Z)?0:(status.M)?0:1; break;
			case BCOND_GEZ: res=(status.Z)?1:(status.M)?0:1; break;
			case BCOND_LTZ: res=(status.Z)?0:(status.M)?1:0; break;
			case BCOND_LEZ: res=(status.Z)?1:(status.M)?1:0; break;
			case BCOND_XS0:
			case BCOND_XS1:
			case BCOND_XS2:
			case BCOND_XS3:
				printf("\nChecking if sense[%x] is set: %x .\n", (op&0xf)-BCOND_XS0, sense[(op&0xf)-BCOND_XS0]);
			       	res=(sense[(op&0xf)-BCOND_XS0])?1:0; break;
			default: break;
		}
	}
	/* If the result is true, add the specified offset to the PC */
	if(res){
		PC=(uint16_t)(PC+(int8_t)offset);
		fprintf(stderr,"\n Branching to %#06x\n",PC);
	}
	return;
}
/* Address mode types */
typedef struct amode_type_t {
	uint8_t mask;
	uint8_t shift;
	char *arg_desc[];
} amode_type_t;

static amode_type_t amode_type_none={0x00,0,{""}};

static amode_type_t amode_type_rel={0x00,0,{"PC+O"}};

static amode_type_t amode_type_alu={0x08,3,{"R,R","!"}};

static amode_type_t amode_type_mem={0x0f,0,{"#D","M","[M]","PC+O","[PC+O]","_[R]_","?R?","?R?","R0","R1","R2","R3","R4","R5","R6","R7"}};

amode_type_t *amode_types[]={&amode_type_none,&amode_type_rel,&amode_type_alu,&amode_type_mem};

char *mn_data_size_mods[]={"",".b",".w"};
char *mn_flag_mods[]={"s","c"};

/* Current instruction type */
typedef struct cinst_t {
	uint16_t address;
	uint8_t opcode; /* op code */
	union {
		uint8_t args[2]; /* argument bytes, if given */
		uint16_t argw;
	};
	uint8_t argc; /* number of argument bytes */
	uint8_t op_type; /* Operation handler type */
	uint8_t data_type; /* none = 0, byte = 1, word = 2 */
	uint8_t amode_type; /* Addressing mode type */
	uint8_t amode; /* Addressing mode */
	uint8_t autoidx_mode; /* Autoindex mode (pre/post inc/dec) */
	uint8_t autoidx_reg; /* Autoindex register */
	int8_t autoidx_delta; /* Autoindex delta */
	uint8_t indirects; /* Number of levels of indirection */
	uint8_t dsttype; /* 0 - register/NA, 1 - memory address */
	uint8_t dstreg; /* Destination register */
	uint16_t dstaddr; /* Destination address */
	uint8_t srctype; /* 0 - register/NA, 1 - memory address */
	uint8_t srcreg; /* Source register */
	uint16_t srcaddr; /* Source address */
	uint16_t value; /* Argument value */
	int8_t offset; /* Relative offset */
	char mnemonic[8]; /* assembly mnemonic */
	char argtext[16]; /* assembly text for arguments */
} cinst_t;
cinst_t cinst;

typedef struct amode_t {
	char *desc;
	char *fmt;
	char indirects;
	char type;
} amode_t;

enum amode_mem_modes { IMMEDIATE, MEM_DIRECT, MEM_INDIRECT, PCREL_DIRECT, PCREL_INDIRECT, REG_INDIRECT_AUTOIDX,
	IMP_REG_A_INDIRECT, IMP_REG_B_INDIRECT, IMP_REG_X_INDIRECT, IMP_REG_Y_INDIRECT, IMP_REG_Z_INDIRECT,
	IMP_REG_S_INDIRECT, IMP_REG_F_INDIRECT, IMP_REG_PC_INDIRECT};
static amode_t amodes_mem[]={
	{"immediate", "#D",0,0},
	{"memory direct", "M",1,1},
	{"memory indirect", "(M)",2,1},
	{"PC relative direct", "PC+O",1,1},
	{"PC relative indirect", "(PC+O)",2,1},
	{"register indirect autoinc", "(R)+",2,0},
	{"unknown mode 0x6", "U6",0,0},
	{"unknown mode 0x7", "U7",0,0},
	{"implicit register A indirect", "(A)",2,0},
	{"implicit register B indirect", "(B)",2,0},
	{"implicit register X indirect", "(X)",2,0},
	{"implicit register Y indirect", "(Y)",2,0},
	{"implicit register Z indirect", "(Z)",2,0},
	{"implicit register S indirect", "(S)",2,0},
	{"implicit register F indirect", "(F)",2,0},
	{"implicit register PC indirect", "(PC)",2,0}
};

/* Indirection level: 0=immediate,1=direct,2+=indirect
 * Source type: 0=register, 1=address */

uint8_t get_valueb(uint8_t indirects, uint8_t srctype, uint16_t src) {
	fprintf(stderr,"Getting byte value: indirects=%u, srctype=%u, src=%#06x\n",indirects,srctype,src);
	/* Special case register direct where we only want to read and return a byte from the reg */
	if(indirects==0){ return(src&0xff); }
	if(indirects==1&&!srctype){ return(reg_readb(src&0xf)); }
	while(indirects-->1){
		if(srctype++) { src=( (mem_readb(src)<<8) + mem_readb(src+1) ); }
		else { src=reg_readw((src&0xf)); }
		fprintf(stderr,"             value: indirects=%u, srctype=%u, src=%#06x\n",indirects,srctype,src);
	}
	src=mem_readb(src);
	fprintf(stderr,"             value: indirects=%u, srctype=%u, src=%#04x\n",indirects,srctype,src);
	return(src);
}

uint16_t get_valuew(uint8_t indirects, uint8_t srctype, uint16_t src) {
	fprintf(stderr,"Getting word value: indirects=%u, srctype=%u, src=%#06x\n",indirects,srctype,src);
	while(indirects--){
		if(srctype++) { src=( (mem_readb(src)<<8) + mem_readb(src+1) ); }
		else { src=reg_readw((src&0xf)); }
		fprintf(stderr,"        word value: indirects=%u, srctype=%u, src=%#06x\n",indirects,srctype,src);
	}
	return(src);
}

void put_valueb(uint8_t indirects, uint8_t dsttype, uint16_t dst, uint8_t val) {
	fprintf(stderr,"Putting byte value %#04x: indirects=%u, dsttype=%u, dst=%#06x\n",val, indirects,dsttype,dst);
	/* Special case register direct where we only want to store a byte to the reg */
	if(indirects==1&&!dsttype){ return(reg_writeb(dst&0xf, val)); }
	while(indirects-->1){
		if(dsttype++) { dst=( (mem_readb(dst)<<8) + mem_readb(dst+1) ); }
		else { dst=reg_readw((dst&0xf)); }
		fprintf(stderr,"                        indirects=%u, dsttype=%u, dst=%#06x\n",indirects,dsttype,dst);
	}
	mem_writeb(dst,val);
	return;
}

void put_valuew(uint8_t indirects, uint8_t dsttype, uint16_t dst, uint16_t val) {
	fprintf(stderr,"Putting word value %#06x: indirects=%u, dsttype=%u, dst=%#06x\n", val, indirects,dsttype,dst);
	/* Special case register direct where we only want to store a word to the reg */
	if(indirects==1&&!dsttype){ return(reg_writew(dst&0xf, val)); }
	while(indirects-->1){
		if(dsttype++) { dst=( (mem_readb(dst)<<8) + mem_readb(dst+1) ); }
		else { dst=reg_readw((dst&0xf)); }
		fprintf(stderr,"                          indirects=%u, dsttype=%u, dst=%#06x\n",indirects,dsttype,dst);
	}
	mem_writeb(dst,(val&0xff00)>>8);
	mem_writeb(dst+1,(val&0x00ff));
	return;
}




void parse_cinst_amode() {
	uint8_t tmp1, tmp2;
	/* Determine arg count, source and destination types and locations */
	switch(cinst.amode_type) {
		/* Memory operation addressing */
		case AMEM:
			if(!cinst.amode) { cinst.argc=cinst.data_type; }
			else if(cinst.amode<0x3) { cinst.argc=DS_W; }
			else if(cinst.amode>0x7) { cinst.argc=0; }
			else { cinst.argc=DS_B; }
			break;

		/* ALU operation addressing */
		case AALU:
			cinst.argc=cinst.amode?0:1; break;
		/* Conditional branching operation relative addhttps://www.grymoire.com/Unix/Awk.html#toc-uh-47ressing */
		case AREL:
		case ADMA:
			cinst.argc=1; break;
		/* Everything else */
		default:
			cinst.argc=0; break;
	}
}


void parse_cinst_opcode() {
	inst_base_t *ib;
	inst_node_t *n=0;
	char *mod;
	char tmpname[2]=" ";
	char implicit;
	amode_type_t *amtp;
	uint8_t i,j;
	for(i=0;(ib=inst_bases[i]);i++){
		for(j=0;ib->n[j].node;j++){
			n=&ib->n[j];
			if((cinst.opcode & ~n->mask) == n->offset) {
				/* We found a match */
				cinst.argw=0; /* Clear the arguments */
				cinst.op_type=ib->op_type; /* Store the op type */
				/* Set implicit registers used by default by various op types */
				switch(cinst.op_type){
					case OT_LOAD: cinst.dstreg=NAME2REG((n->node)); break;
					case OT_STORE: cinst.srcreg=NAME2REG((n->node)); break;
					case OT_ALU2:
						if(ib->amode_type == ATREG ) {
							tmpname[0]=*(n->node);
							cinst.srcreg=NAME2REG(tmpname);
							tmpname[0]=*(n->node+1);
							cinst.dstreg=NAME2REG(tmpname);
						} else {
						       cinst.srcreg=NAME2REG("a");
						       cinst.dstreg=NAME2REG("b");
						}
						break;
					case OT_ALU1:
						      cinst.dstreg=NAME2REG("a");
						      break;
					default: break;    
				}
				/* Determine the data type */
				cinst.data_type=(ib->data_type==DS_WB?((cinst.opcode&0x10)?DS_W:DS_B):ib->data_type);
				cinst.amode_type=ib->amode_type; /* Store the address mode type */
				amtp=amode_types[cinst.amode_type]; /* Get pointer to details for this address mode type */
				cinst.amode=((cinst.opcode)&(amtp->mask)&(n->mask))>>amtp->shift; /* Get address mode */
				/* Parse the addressing mode */
				parse_cinst_amode();
				cinst.argtext[0]='\0';
				cinst.autoidx_mode=0;
				cinst.autoidx_delta=0;
				cinst.autoidx_reg=0;
				mod=(cinst.data_type==DS_BIT)?
						(n->mask?mn_flag_mods[cinst.opcode&0x1]:mn_data_size_mods[DS_N])
						 :mn_data_size_mods[cinst.data_type];
				implicit=(cinst.amode_type!=ANONE && !cinst.argc)?'!':'\0';
				snprintf(cinst.mnemonic,sizeof(cinst.mnemonic),
					"%s%s%s%c",ib->base,n->node,mod,implicit);
				goto found;
			}
				
		}
	}

found:
	return;

}


/* Fetch an instruction from memory */
void fetch_instruction() {
	uint8_t n=0;
	/* Store the address for this instruction and increment the program counter */
	cinst.address=PC++;

	/* Read the opcode from memory */
	cinst.opcode=mem_readb(cinst.address);
	//fprintf(stderr,"\n%#06x %02x", cinst.address,cinst.opcode);
	/* Parse the opcode */
	parse_cinst_opcode();

	/* Capture additional argument bytes */
	while(n<cinst.argc) {
		cinst.args[n]=mem_readb(PC++);
		//fprintf(stderr," %02x", cinst.args[n]);
		n++;
	}
	//fprintf(stderr,"\n");
	return;
}

void parse_argtext() {
	uint8_t tmpreg=0;
	uint16_t tmpaddr=0;
	char *pre, *post;
	tmpreg=(cinst.op_type==OT_LOAD)? cinst.srcreg : cinst.dstreg;
	tmpaddr=(cinst.op_type==OT_LOAD)? cinst.srcaddr : cinst.dstaddr;

	switch(cinst.amode_type) {
		case AREL:
			snprintf(cinst.argtext,sizeof(cinst.argtext),
				"PC%c%#04x",cinst.offset<0?'-':'+',cinst.offset<0?-cinst.offset:cinst.offset);
			break;
		case AALU:
			if(cinst.argc) {
				if(cinst.op_type==OT_ALU2) {
					snprintf(cinst.argtext,sizeof(cinst.argtext), "%s, %s",
						REG2NAME(cinst.dstreg,cinst.data_type),
						REG2NAME(cinst.srcreg,cinst.data_type) );
				} else if(cinst.value) {
					snprintf(cinst.argtext,sizeof(cinst.argtext), "%s, %#04x",
						REG2NAME(cinst.dstreg,cinst.data_type),
						cinst.value );
				} else {
					snprintf(cinst.argtext,sizeof(cinst.argtext), "%s",
						REG2NAME(cinst.dstreg,cinst.data_type) );
				}
			}
			break;
		case AMEM:
			switch(cinst.amode) {
				case IMMEDIATE:
					snprintf(cinst.argtext,sizeof(cinst.argtext),
						cinst.data_type==DS_W?"#%#06x":"#%#04x",
						cinst.value);
					break;
				case MEM_DIRECT:
					snprintf(cinst.argtext,sizeof(cinst.argtext),
						"%#06x", tmpaddr);
					break;
				case MEM_INDIRECT:
					snprintf(cinst.argtext,sizeof(cinst.argtext),
						"(%#06x)", tmpaddr);
					break;
				case PCREL_DIRECT:
					snprintf(cinst.argtext,sizeof(cinst.argtext),
						"PC%c%#04x",cinst.offset<0?'-':'+',cinst.offset<0?-cinst.offset:cinst.offset);
					break;
				case PCREL_INDIRECT:
					snprintf(cinst.argtext,sizeof(cinst.argtext),
						"PC%c%#04x",cinst.offset<0?'-':'+',cinst.offset<0?-cinst.offset:cinst.offset);
					break;
				case REG_INDIRECT_AUTOIDX:
					switch(cinst.autoidx_mode) {
						case 0:pre="";post=""; break;
						case 1:pre="";post="+"; break;
						case 2:pre="-";post=""; break;
						default: pre="??"; post="??"; break;
					}
					snprintf(cinst.argtext,sizeof(cinst.argtext), "%s(%s)%s",
						pre,REG2NAME(cinst.autoidx_reg,DS_W),post);
					break;
				case IMP_REG_A_INDIRECT:
				case IMP_REG_B_INDIRECT:
				case IMP_REG_X_INDIRECT:
				case IMP_REG_Y_INDIRECT:
				case IMP_REG_Z_INDIRECT:
				case IMP_REG_S_INDIRECT:
				case IMP_REG_F_INDIRECT:
				case IMP_REG_PC_INDIRECT:
					snprintf(cinst.argtext,sizeof(cinst.argtext), "(%s)",
						REG2NAME(tmpreg,DS_W));
					break;
				default:
					snprintf(cinst.argtext,sizeof(cinst.argtext), "??%s??",
						REG2NAME(tmpreg,DS_W));
					break;
				break;
			}

	}
}
void prepare_instruction() {
	uint8_t tmpreg=0;
	int8_t autoidx_delta=0;
	uint8_t autoidx_reg=0;
	uint16_t tmpaddr=0;
	char srcname,dstname;
	// fprintf(stderr,"amode_type: %x\n",cinst.amode_type);
	switch(cinst.amode_type){
		case AREL:
			cinst.offset=(int8_t)(cinst.args[0]);
			cinst.dstaddr=(uint16_t)(PC+(int8_t)cinst.offset);
			break;
		case AALU:
			fprintf(stderr,"AALU argc=%u op_type=%u\n", cinst.argc, cinst.op_type);
			if(cinst.argc) {
				if(cinst.op_type==OT_ALU2) {
					cinst.dstreg=cinst.args[0]&0x0f;
					cinst.srcreg=(cinst.args[0]&0xf0)>>4;
				} else {
					cinst.dstreg=(cinst.args[0]&0xf0)>>4;
					cinst.value=cinst.args[0]&0x0f;
				}
			} else { cinst.value=0; }
			break;
		case AMEM:
			switch(cinst.amode) {
				case IMMEDIATE: cinst.value=(cinst.data_type==DS_W)?ntohs(cinst.argw):cinst.args[0];
						break;
				case MEM_DIRECT:
				case MEM_INDIRECT:
						   tmpaddr=ntohs(cinst.argw); break;
				case PCREL_DIRECT:
				case PCREL_INDIRECT:
					cinst.offset=(int8_t)(cinst.args[0]);
					tmpaddr=(uint16_t)(PC+cinst.offset);
					fprintf(stderr,"PC+%i  %#06x-> %#06x\n", cinst.offset,PC, tmpaddr);
					break;
				case REG_INDIRECT_AUTOIDX:
					tmpreg=(cinst.args[0]&0xf0)>>4;
					cinst.autoidx_mode=(cinst.args[0]&0x0f);
					cinst.autoidx_reg=tmpreg;
					if(cinst.autoidx_mode>2) {
					TRAP(cinst.address,
						"Unknown auto-index mode specified for register %s: %x",
						regnames16[tmpreg>>1],cinst.autoidx_mode
					);}
					cinst.autoidx_delta=!cinst.autoidx_mode?0:cinst.autoidx_mode&1?1:cinst.autoidx_mode&2?-1:42;
					cinst.autoidx_delta*=cinst.data_type==DS_W?2:1;
					fprintf(stderr,"Autoindex register %s delta %i\n",
							REG2NAME(cinst.autoidx_reg,cinst.data_type),cinst.autoidx_delta);
					break;
				case IMP_REG_A_INDIRECT: tmpreg=0x0; break;
				case IMP_REG_B_INDIRECT: tmpreg=0x2; break;
				case IMP_REG_X_INDIRECT: tmpreg=0x4; break;
				case IMP_REG_Y_INDIRECT: tmpreg=0x6; break;
				case IMP_REG_Z_INDIRECT: tmpreg=0x8; break;
				case IMP_REG_S_INDIRECT: tmpreg=0xa; break;
				case IMP_REG_F_INDIRECT: tmpreg=0xc; break;
				case IMP_REG_PC_INDIRECT: tmpreg=0xe; break;
				default:
					TRAP(cinst.address,
						"Unknown addressing mode: op=%#04x",
						cinst.opcode);
				break;
			}

			cinst.indirects=amodes_mem[cinst.amode].indirects;
			if(cinst.op_type==OT_LOAD) {
				cinst.srctype=amodes_mem[cinst.amode].type;
				cinst.srcreg=tmpreg;
				cinst.srcaddr=tmpaddr;
			} else {
				cinst.dsttype=amodes_mem[cinst.amode].type;
				cinst.dstreg=tmpreg;
				cinst.dstaddr=tmpaddr;
			}
			break;

	}

}
void execute_instruction() {
	uint16_t tmpsrc,tmpdst,tmpvalue;
	if(cinst.autoidx_delta<0) { 
		fprintf(stderr,"Autoindexing register %s by %i %#06x -> ",
		REG2NAME(cinst.autoidx_reg,cinst.data_type),cinst.autoidx_delta,reg_readw(cinst.autoidx_reg));
		reg_writew(cinst.autoidx_reg,reg_readw(cinst.autoidx_reg)+cinst.autoidx_delta);
		fprintf(stderr,"%#06x\n", reg_readw(cinst.autoidx_reg));
	}
	switch(cinst.op_type) {
		case OT_SYS:
			sys_func(cinst.opcode); break;
		case OT_FLOW_COND:
			flow_cond_rel(cinst.opcode, cinst.offset); break;
		case OT_FLOW_JUMP:
			flow_jump(get_valuew(cinst.indirects-1,cinst.dsttype,cinst.dstaddr));
			break;
		case OT_FLOW_CALL:
			flow_call(get_valuew(cinst.indirects-1,cinst.dsttype,cinst.dstaddr));
			break;
		case OT_ALU1:
			if (cinst.data_type==DS_W) { alu_op1regw(cinst.opcode, cinst.dstreg, cinst.value); }
			else { alu_op1regb(cinst.opcode, cinst.dstreg, cinst.value); }
			break;
		case OT_ALU2:
			if (cinst.data_type==DS_W) {
				if(cinst.amode_type==ATREG) { reg_movw(cinst.srcreg, cinst.dstreg); }
				else { alu_op2regw(cinst.opcode, cinst.srcreg, cinst.dstreg); }
			} else { 
				if(cinst.amode_type==ATREG) { reg_movb(cinst.srcreg, cinst.dstreg); }
				else { alu_op2regb(cinst.opcode, cinst.srcreg, cinst.dstreg); }
			}
			break;
		case OT_LOAD:
			/* Indirection level: 0=immediate,1=direct,2+=indirect
			 * Source type: 0=register, 1=address */
			tmpsrc=cinst.indirects?cinst.srctype?cinst.srcaddr:cinst.srcreg:cinst.value;
			fprintf(stderr,"Loading reg %0x from %#0x\n",cinst.dstreg,tmpsrc);

			if (cinst.data_type==DS_W) {
				tmpvalue=get_valuew(cinst.indirects, cinst.srctype, tmpsrc);
				reg_writew(cinst.dstreg, tmpvalue);
				status.M=(tmpvalue&0x8000)?1:0;
			} else {
				tmpvalue=get_valueb(cinst.indirects, cinst.srctype, tmpsrc);
				reg_writeb(cinst.dstreg, tmpvalue&0xff);
				status.M=(tmpvalue&0x80)?1:0;
			}
			status.Z=tmpvalue?0:1;
			break;

		case OT_STORE:
			tmpdst=cinst.indirects?cinst.dsttype?cinst.dstaddr:cinst.dstreg:cinst.value;
			fprintf(stderr,"Storing %#0x from reg %0x\n",tmpdst, cinst.srcreg);
			if (cinst.data_type==DS_W) { put_valuew(cinst.indirects, cinst.dsttype, tmpdst,reg_readw(cinst.srcreg)); }
			else { put_valueb(cinst.indirects, cinst.dsttype, tmpdst,reg_readb(cinst.srcreg)); }
			break;
	}
	if(cinst.autoidx_delta>0) { 
		fprintf(stderr,"Autoindexing register %s by %i %#06x -> ",
		REG2NAME(cinst.autoidx_reg,cinst.data_type),cinst.autoidx_delta,reg_readw(cinst.autoidx_reg));
		reg_writew(cinst.autoidx_reg,reg_readw(cinst.autoidx_reg)+cinst.autoidx_delta);
		fprintf(stderr,"%#06x\n", reg_readw(cinst.autoidx_reg));
	}
}

#define NUMROMS 5
typedef struct ROMfile_t {
	char *filename;
	uint8_t bank;
	uint32_t base;
	uint16_t size;
	uint8_t reverse;
	uint64_t addr_bitsalad;
} ROMfile_t;

static ROMfile_t ROMS[] = {
	{"Bootstrap.rom",0,0xfc00,0x0200,1,0xfedcba9765843210LL},
	{"Diag/Diag_F1_Rev_1.0.BIN",0,0x8000,0x0800,0,0},
	{"Diag/Diag_F2_Rev_1.0.BIN",0,0x8800,0x0800,0,0},
	{"Diag/Diag_F3_Rev_1.0.BIN",0,0x9000,0x0800,0,0},
	{"Diag/Diag_F4_1133CMD.BIN",0,0x9800,0x0800,0,0},
	{0,0,0,0,0,0}
};

/* Utility functions to extract ranges of bits */
#define BITRANGE(d,s,n) ((d>>s) & ((2LL<<(n-1))-1) )
uint16_t bitsalad_16(uint64_t order, uint16_t d) {
	uint8_t pos;
	uint16_t out;
	for(int i=0; i<16; i++) {
		pos=BITRANGE(order,i*4,4);
		if(d&1<<i){out |= 1<<pos;}
	}
	return(out);
}

int read_roms() {
	int i=0;
        FILE *fp;
	char *filename;
        size_t ret_code;
	uint16_t ROMSIZE, pos;
	uint8_t buf[0x10000];

        //for(int  i=0; i<NUMROMS; i++) {
        for(i=0; ROMS[i].filename; i++) {
		ROMSIZE=ROMS[i].size;
		filename=ROMS[i].filename;
                fp=fopen(filename,"rb");
                ret_code=fread(buf,1,ROMSIZE,fp);
                if(ret_code != ROMSIZE) {
                        if(feof(fp)) {
                                fprintf(stderr,"Unexpected EOF while reading %s: Only got %u byte of an expected %u.\n",
                                        filename, (uint16_t)ret_code, ROMSIZE);
                        } else if(ferror(fp)){
                                fprintf(stderr,"Failed while reading %s!\n",filename);
                        }
                        fclose(fp);
                        return(-1);
                }
                fclose(fp);
		for(uint32_t p=0; p<ROMSIZE; p++) {
			if(ROMS[i].reverse) {
				pos=ROMS[i].base+(ROMSIZE-1-p);
			} else {
				pos=ROMS[i].base+p;
			}
			if(ROMS[i].addr_bitsalad) { pos=bitsalad_16(ROMS[i].addr_bitsalad,pos); }

			membank[ROMS[i].bank].a[pos]=buf[p];
			fprintf(stderr,"buf[%04x]=%02x\n",pos, buf[p]);
			fprintf(stderr,"pos: %x val: %x  memval: %x \n", pos, buf[p],membank[ROMS[i].bank].a[pos]);
		}

        }
        return(i);

}



#define PTSNAME_MAXLEN 1024
typedef struct pty_pair_t {
	int master;
	char slave_name[PTSNAME_MAXLEN];
} pty_pair_t;

int pty_setup(pty_pair_t *pair) {
	if(!pair) {
		return(EINVAL);
	}
	if( (pair->master = posix_openpt(O_RDWR|O_NOCTTY)) < 0 ) {
		perror("posix_openpt() failed to open pty");
		goto pty_setup_end;
	}
	if( grantpt(pair->master) ) {
		perror("grantpt() failed");
		goto pty_setup_end;
	}
	if( unlockpt(pair->master) ) {
		perror("unlockpt() failed");
		goto pty_setup_end;
	}
	if( ptsname_r(pair->master,pair->slave_name,PTSNAME_MAXLEN) ) {
		perror("ptsname_r() failed");
		goto pty_setup_end;
	}

pty_setup_end:
	if(errno && pair && pair->master>-1) { close(pair->master); }
	else { printf("pty opened: %s\n",pair->slave_name); }
	return(errno);


}

/* MUX Ports (N) with input and output lines */
#define MUXPORTS 1
typedef struct muxport_t {
	uint16_t base_addr;
	/* NDB2=1, NDB1=1, NSB=0, NPB=1, POE=X, BAUD=9600 (CLK/1?) */
	uint8_t control_word; /* 0xc5 (0b11000101) ->8N1 9600 */
	/* Rx Parity Error RPE,  Rx Framing Error RFE, Rx Overrun ROR, Tx Buffer Empty TBMT, Rx Data Available RDA */
	uint8_t status_word; /* &0x1c (0b00011100)-> Error State (RPE|RFE|ROR), &0x02->TBMT, &0x01->RDA  */
	pty_pair_t pty_pair;
} muxport_t; 

int mux_running=0;
muxport_t muxports[MUXPORTS];

//int MUX[MUXPORTS][2]; /* File descriptors for open MUX fifo endpoints */
//static const char *MUX_fifo[MUXPORTS][2] = { {"mux0out","mux0in"} };

int start_mux(uint8_t port){
	if(port>MUXPORTS-1) { return(-EINVAL); }
//	for (uint8_t i=0;i<2;i++) {
//		if ( mkfifo(MUX_fifo[port][i],0666) ) {
//			fprintf(stderr,"Failed to create %s FIFO: %s\n", i?"input":"output", MUX_fifo[port][0]);
//			return(-1);
//		}
//	}
	pty_setup(&muxports[port].pty_pair);
	mux_running |= 1<<port;
	return(0);
}

int connect_mux(uint8_t port, uint8_t dir) {
	if(port>MUXPORTS-1) { return(-EINVAL); }
//	if( MUX[port][dir] ) { return(0); }
//	if ( ( MUX[port][dir]= open(MUX_fifo[port][dir], (dir?O_RDONLY:O_WRONLY)|O_NONBLOCK) ) < 0 ) {
//		if(errno != ENXIO) {
//			perror("Error opening FIFO:");
//			return(-1);
//		} 
//	}
	return(0);

}

int mux_out(uint8_t port,char c) {
	if(!(mux_running&(1<<port)) || port>MUXPORTS-1) { return(-EINVAL); }
//	if( connect_mux(port,0) < 0 ) { return(-1); }
//	write(MUX[port][0],&c,1);
	write(muxports[port].pty_pair.master, &c, 1);
	return(0);
}

int mux_in(uint8_t port) {
	char c;
	if(!(mux_running&(1<<port)) || port>MUXPORTS-1) { return(-EINVAL); }
//	if( connect_mux(port,1) < 0 ) { return(-1); }
//	if( (read(MUX[port][1], &c,1)) != -1) { return((int)c); }
	if( (read(muxports[port].pty_pair.master, &c,1)) != -1) { return((int)c); }
	return(-1);
}

int stop_mux(uint8_t port) {
	if(port>MUXPORTS-1) { return(-EINVAL); }
//	close(MUX[port][0]);
//	close(MUX[port][1]);
//	remove(MUX_fifo[port][0]);
//	remove(MUX_fifo[port][1]);

	mux_running &= ~(1<<port);
	return(0);
}

uint8_t mmio_mux_readb(uint16_t ioaddr) {
	uint8_t in;
	uint8_t status=0x3;
	switch(ioaddr&0xf) {
		case 0x00:
			fprintf(stderr,"\nMUX status: %0x\n",status);
			return(status);
		case 0x01:
			in=mux_in(0);
			fprintf(stderr,"\nMUX input: %c\n",in);
			return(in);
		default: break;
	}
}

uint8_t mmio_mux_writeb(uint16_t ioaddr, uint8_t val) {
	switch(ioaddr&0xf) {
		case 0x00:
			fprintf(stderr,"Setting MUX control word to %#04x\n",val); return;
		case 0x01:
			fprintf(stderr,"MUX out: '%c' %#04x & ~0x80\n",val&~0x80,val);//return;
			return(mux_out(0,val&~0x80));
		default: break;
	}
}

uint8_t diag_dp[4];
uint8_t diag_digits;
uint8_t diag_digits_en;
uint8_t diag_dips;

uint8_t mmio_diag_readb(uint16_t ioaddr) {
	switch(ioaddr&0x001f) {
		case 0x10:
			printf("MMIO Diag reading DIP switches: %0x\n",diag_dips);
			return(diag_dips); /* Read DIP switches */
	}
}
uint8_t mmio_diag_writeb(uint16_t ioaddr, uint8_t val) {
	switch(ioaddr&0x001f) {
		case 0x6: diag_digits_en=1; break; /* Unblank hex displays */
		case 0x7: diag_digits_en=0; break; /* Blank hex displays */
		case 0x8: case 0xa: case 0xc: case 0xe:
			diag_dp[((ioaddr&0xf)>>1)-4]=1; break;
		case 0x9: case 0xb: case 0xd: case 0xf:
			diag_dp[((ioaddr&0xf)>>1)-4]=0; break;
		case 0x10: diag_digits=val; break;
	}
}

int main(int argc, char *argv[] ) {
	int opt;
	while((opt = getopt(argc,argv,"T:D")) != -1) {
		switch(opt) {
			case 'T': diag_dips=strtol(optarg,NULL,0); break;
			case 'D':  stderr=stdout; break;
			case '?':
				if(optopt=='T') {
					printf("\n-T option requires test number.\n");
				}
				break;
		}
	}
	// if(argc>1) { diag_dips=strtol(argv[1],NULL,0); }
	cpuregs=&(mem->r);
	cpuregset=&(cpuregs->i[0]);
	read_roms();
	start_mux(0);
	while (!mux_in(0)) {;}
	mux_out(0,'H');
	mux_out(0,'e');
	mux_out(0,'l');
	mux_out(0,'l');
	mux_out(0,'o');
	mux_out(0,'!');
	fprintf(stderr,"0xfc90=%02x\n",mem->a[0xfc90]);
	mem->a[0x100]=0x60;
	mem->a[0x101]=0x55;
	mem->a[0x102]=0x22;
	PC=0xfc00;
	halted=0;
	sense[0]=1;
	while(!halted) {
		fetch_instruction();
		prepare_instruction();
		parse_argtext();
		fprintf(stderr,"\n%#06x: %02x",cinst.address, cinst.opcode);
		switch(cinst.argc){
			case 0: fprintf(stderr,"      "); break;
			case 1: fprintf(stderr," %02x   ", cinst.args[0] ); break;
			case 2: fprintf(stderr," %02x %02x", cinst.args[0], cinst.args[1] ); break;
		}
		fprintf(stderr,"\t%s\t%s\n", cinst.mnemonic, cinst.argtext);
		//fprintf(stderr,"%02x\n", cinst.argc?cinst.argc>1?ntohs(cinst.argw):cinst.args[0]:0);
		execute_instruction();

		fprintf(stderr,"Status word:[%c%c%c%c%c]\n",
				status.C?'C':' ',
				status.Z?'Z':' ',
				status.V?'V':' ',
				status.M?'M':' ',
				status.L?'L':' '
				);
	}
	mem->a[0x0200]='H';
	mem->a[0x0201]='e';
	mem->a[0x0202]='l';
	mem->a[0x0203]='l';
	mem->a[0x0204]='o';
	mem->a[0x0205]='w';
	mem->a[0x0206]=',';
	mem->a[0x0207]=' ';
	mem->a[0x0208]='w';
	mem->a[0x0209]='o';
	mem->a[0x020a]='r';
	mem->a[0x020b]='l';
	mem->a[0x020c]='d';
	mem->a[0x020d]='!';
	mem->a[0x020e]='\0';


	//regs_writew(5,('1'<< 8 )|'0');
	reg_ldb(0x04,0x0200);
	fprintf(stderr,"%c %x %x\n",mem->a[4], reg_readb(4), reg_readb(5));

	stop_mux(0);

	return(0);
}

