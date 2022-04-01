#include <stdio.h>
#include <stdint.h>

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
		int16_t X,Y,Z,U,W,SP,ST,PC;
	};
} regset_t;

typedef union regmem_t {
	uint16_t w[128];
	uint8_t b[256];
	regset_t i[16];
} regmem_t;

typedef union memmap_t {
	int8_t a[0x10000];
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
enum amode_types { ANONE, AREL, AALU, AMEM };

static inst_base_t ib_jump={"jump",OT_FLOW_JUMP,DS_N,AMEM,{{"",0x70,0x07},{0,0,0}}};

static inst_base_t ib_call={"call",OT_FLOW_CALL,DS_N,AMEM,{{"",0x78,0x07},{0,0,0}}};

static inst_base_t ib_reti={"reti",OT_SYS,DS_N,ANONE,{{"",0x0a,0x00},{0,0,0}}};
static inst_base_t ib_ret={"ret",OT_SYS,DS_N,ANONE,{{"",0x09,0x00},{0,0,0}}};

static inst_base_t ib_ldz={"ldz",OT_LOAD,DS_W,AMEM,{{"",0x68,0x07},{0,0,0}}};

static inst_base_t ib_stz={"stz",OT_STORE,DS_W,AMEM,{{"",0x60,0x07},{0,0,0}}};

static inst_base_t ib_ld={"ld",OT_LOAD,DS_WB,AMEM,{{"x",0xa0,0x1f},{"y",0xe0,0x1f},{0,0,0}}};

static inst_base_t ib_st={"st",OT_STORE,DS_WB,AMEM,{{"x",0x80,0x1f},{"y",0xc0,0x1f},{0,0,0}}};

static inst_base_t ib_aluls={"ls",OT_ALU1,DS_WB,AALU,{{"l",0x24,0x18},{"r",0x25,0x18},{0,0,0}}};

static inst_base_t ib_alur={"rc",OT_ALU1,DS_WB,AALU,{{"r",0x26,0x18},{"l",0x27,0x18},{0,0,0}}};

static inst_base_t ib_f={"f",OT_SYS,DS_BIT,ANONE,{{"s",0x02,0x1},{"i",0x04,0x01},{"c",0x06,0x01},{"ac",0x08,0x00},{0,0,0}}};

static inst_base_t ib_bf={"b",OT_FLOW_COND,DS_BIT,AREL,{
	{"c",0x10,0x1},{"s",0x12,0x01},{"z",0x14,0x01},{0,0,0}
}};

static inst_base_t ib_b={"b",OT_FLOW_COND,DS_N,AREL,{
	{"lt",0x16,0x00},{"ge",0x17,0x00},{"gt",0x18,0x00},{"le",0x19,0x00},
	{"xs0",0x1a,0x00}, {"xs1",0x1b,0x00},{"xs2",0x1c,0x00},{"xs3",0x1d,0x00},
	{"b0x1e",0x1e,0x00},{"b0x1f",0x1f,0x00},{0,0,0}
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
	&ib_bf,
	&ib_b,
	&ib_alumisc,
	&ib_alubi,
	&ib_misc,
	0
};



/* Active program counter */
uint16_t PC;

uint8_t halted=1;
uint8_t sense[4];

#define NUMBANKS 2
uint8_t ilevel=0;
memmap_t membank[NUMBANKS];
memmap_t *mem = &membank[0];
regmem_t *cpuregs;
regset_t *cpuregset;
status_word_t status;

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
	return(0xff);
}

/* This will get special handling later */
void mmio_writeb(uint16_t dst, uint8_t val) {
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
	ALU_LSL,ALU_LSR,
	ALU_RCL,ALU_RCR
};
/* Two register */
enum alu_ops2 {
	ALU_ADD,ALU_SUB,
	ALU_AND,ALU_OR,
	ALU_XOR,ALU_MOV,
	ALU_U0x6,ALU_U0x7
};

/* Handle byte size single register ALU instructions */
void alu_op1argb(uint8_t op, uint8_t reg, uint8_t opt) {
	uint8_t arg,res;
	uint16_t tmp;
	/* Read argument byte from specified register */
	arg=reg_readb(reg);
	switch(op&0x0f) {
		case ALU_INC: res=arg+(1+opt); status.V=(res<arg)?1:0; break;
		case ALU_DEC: res=arg-(1+opt); status.V=(res>arg)?1:0; break;
		case ALU_CLR: res=0; break;
		case ALU_NOT: res=~res; break;
		case ALU_LSL: res=arg<<(1+opt); status.C=((arg<<opt)&0x80)?1:0; break;
		case ALU_LSR: res=arg>>(1+opt); status.C=((arg>>opt))?1:0; break;
		case ALU_RCL: /* Dup arg bits, shift up, take the high word and shift back down */
			      tmp=(0xff00&((arg<<8|arg)<<(1+opt)))>>(1+opt);
			      res=(tmp&0xff);
			      status.C=((arg<<opt)&0x8000)?1:0; break;
		case ALU_RCR: /* Dup arg bits, shift down, take the low word */
			      tmp=(0x00ff&(arg<<8|arg))>>(1+opt);
			      status.C=((arg<<opt)&0x80)?1:0; break;
	}
	/* Set zero and minus flags appropriately*/
	status.Z=res?0:1;
	status.M=(res&0x80)?1:0;
	
	reg_writeb(reg,res);
	return;
}

/* Handle word size single register ALU instructions */
void alu_op1argw(uint8_t op, uint8_t reg, uint8_t opt) {
	uint16_t arg,res;
	uint32_t tmp;
	/* Read argument byte from specified register */
	arg=reg_readw(reg);
	switch(op&0x0f) {
		case ALU_INC: res=arg+(1+opt); status.V=(res<arg)?1:0; break;
		case ALU_DEC: res=arg-(1+opt); status.V=(res>arg)?1:0; break;
		case ALU_CLR: res=0; break;
		case ALU_NOT: res=~arg; break;
		case ALU_LSL: res=arg<<(1+opt); status.C=((arg<<opt)&0x8000)?1:0; break;
		case ALU_LSR: res=arg>>(1+opt); status.C=((arg>>opt))?1:0; break;
		case ALU_RCL: /* Dup arg bits, shift up, take the high word and shift back down */
			      tmp=(0xffff0000&((arg<<16|arg)<<(1+opt)))>>(1+opt);
			      res=(tmp&0xffff);
			      status.C=((arg<<opt)&0x8000)?1:0; break;
		case ALU_RCR: /* Dup arg bits, shift down, take the low word */
			      tmp=(0x0000ffff&(arg<<16|arg))>>(1+opt);
			      status.C=((arg<<opt)&0x8000)?1:0; break;
	}
	/* Set zero and minus flags appropriately*/
	status.Z=res?0:1;
	status.M=(res&0x8000)?1:0;
	
	reg_writew(reg,res);
	return;
}

/* Handle byte size two register ALU instructions */
void alu_op2regb(uint8_t op, uint8_t srcreg, uint8_t dstreg) {
	uint16_t srcarg,dstarg,res;
	uint32_t tmp;
	/* Read argument bytes from specified registers */
	srcarg=reg_readb(srcreg);
	dstarg=reg_readb(dstreg);

	switch(op&0x0f) {
		case ALU_ADD:
			tmp=dstarg+srcarg;
			res=dstarg+srcarg;
			status.C=(tmp&0x00010000)?1:0;
			status.V=(~((srcarg&0x8000)^(dstarg&0x8000)) & ((res&0x8000)^(srcarg&0x8000)))?1:0;
			break;
		case ALU_SUB:
			tmp=dstarg-srcarg;
			res=dstarg-srcarg;
			status.C=(tmp&0x00010000)?1:0;
			status.V=(~((res&0x8000)^(dstarg&0x8000)) & ((dstarg&0x8000)^(srcarg&0x8000)))?1:0;
			break;
		case ALU_AND: res=dstarg&srcarg; break;
		case ALU_OR: res=dstarg|srcarg; break;
		case ALU_XOR: res=dstarg^srcarg; break;
		case ALU_MOV: res=srcarg; break;
		default: break;
	}
	/* Set zero and minus flags appropriately*/
	status.Z=res?0:1;
	status.M=(res&0x8000)?1:0;
	 
	reg_writeb(dstreg,res);
	return;
}

/* Handle word size two register ALU instructions */
void alu_op2regw(uint8_t op, uint8_t srcreg, uint8_t dstreg) {
	uint8_t srcarg,dstarg,res;
	uint16_t tmp;
	/* Read argument words from specified registers */
	srcarg=reg_readw(srcreg);
	dstarg=reg_readw(dstreg);

	switch(op&0x0f) {
		case ALU_ADD:
			tmp=dstarg+srcarg;
			res=dstarg+srcarg;
			status.C=(tmp&0x0100)?1:0;
			status.V=(~((srcarg&0x80)^(dstarg&0x80)) & ((res&0x80)^(srcarg&0x80)))?1:0;
			break;
		case ALU_SUB:
			tmp=dstarg-srcarg;
			res=dstarg-srcarg;
			status.C=(tmp&0x0100)?1:0;
			status.V=(~((res&0x80)^(dstarg&0x80)) & ((dstarg&0x80)^(srcarg&0x80)))?1:0;
			break;
		case ALU_AND: res=dstarg&srcarg; break;
		case ALU_OR: res=dstarg|srcarg; break;
		case ALU_XOR: res=dstarg^srcarg; break;
		case ALU_MOV: res=srcarg; break;
		default: break;
	}
	/* Set zero and minus flags appropriately*/
	status.Z=res?0:1;
	status.M=(res&0x80)?1:0;
	 
	reg_writew(dstreg,res);
	return;
}



/* Halt the CPU */
void sys_halt() {
	halted=1;
	return;
}

/* Return from function call */
void flow_ret() {
	PC=reg_readw(4);
	return;
}

/* Return from interrupt */
void flow_reti() {
	return;
}


/* This needs to delay 4.5ms */
void sys_delay() {
	return;
}

enum sys_functions {
	SYS_HALT, SYS_NOP,
	FLAG_MINUS_SET, FLAG_MINUS_CLEAR,
	FLAG_INT_SET, FLAG_INT_CLEAR,
	FLAG_CARRY_SET, FLAG_CARRY_CLEAR,
	FLAG_ALL_CLEAR, FLOW_RET,
	FLOW_RETI, SYS_0x0b,
	SYS_0x0c, SYS_0x0d,
	SYS_DELAY, SYS_0x0f
};

/* System functions */
void sys_func(uint8_t op) {
	switch(op&0xf) {
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
		default: break;
	}
	return;
}

enum branch_rel_cond {
	BCOND_CS,BCOND_CC,
	BCOND_MS,BCOND_MC,
	BCOND_ZS,BCOND_ZC,
	BCOND_LT,BCOND_GE,
	BCOND_GT,BCOND_LE,
	BCOND_XS0, BCOND_XS1,
	BCOND_XS2, BCOND_XS3,
	BCOND_0x1e, BCOND_0x1f
};

/* Conditional branch instructions */
void flow_cond_rel(uint8_t op, int8_t offset) {
	uint8_t res=0;
	/* Handle logical pairs */
	if((op&0xf)<BCOND_XS0) {
		switch(op&0xe) {
			case BCOND_CS: res=(status.C)?1:0; break;
			case BCOND_MS: res=(status.M)?1:0; break;
			case BCOND_ZS: res=(status.Z)?1:0; break;
			case BCOND_LT: res=(status.Z)?0:((status.M^status.V)?1:0); break;
			case BCOND_GT: res=(status.Z)?0:((status.M^status.V)?0:1); break;
		}
		/* Inverted versions of the above */
		res=(op&0x01)?(res?1:0):(res?0:1);
	} else {
	/* Handle the remaining ops */
		switch(op&0xf) {
			case BCOND_XS0:
			case BCOND_XS1:
			case BCOND_XS2:
			case BCOND_XS3:
			       	res=(sense[op&0x3])?0:1; break;
			default: break;
		}
	}
	/* If the result is true, add the specified offset to the PC */
	if(res){PC=(uint16_t)(PC+offset);}
	return;
}

/* Unconditional branch instructions */
void flow_jump(uint16_t dst) {
	PC=dst;
	return;
}

/* Function call */
void flow_call(uint16_t dst) {
	reg_writew(4,PC);
	status.L=1;
	PC=dst;
	return;
}

/* Address mode types */
typedef struct amode_type_t {
	uint8_t mask;
	uint8_t shift;
	char *arg_desc[];
} amode_type_t;

static amode_type_t amode_type_none={0x00,0,{""}};

static amode_type_t amode_type_rel={0x00,0,{"PC+N"}};

static amode_type_t amode_type_alu={0x08,3,{"R,R","!"}};

static amode_type_t amode_type_mem={0x0f,0,{"#D","A","[A]","PC+N","[PC+N]","_[R]_","?R?","?R?","R0","R1","R2","R3","R4","R5","R6","R7"}};

amode_type_t *amode_types[]={&amode_type_none,&amode_type_rel,&amode_type_alu,&amode_type_mem};

char *mn_data_size_mods[]={"",".b",".w"};
char *mn_flag_mods[]={"s","c"};

/* Current instruction type */
typedef struct cinst_t {
	uint16_t address;
	uint8_t opcode; /* op code */
	uint8_t args[2]; /* argument bytes, if given */
	uint8_t argc; /* number of arguments */
	uint8_t op_type; /* Operation handler type */
	uint8_t data_type; /* none = 0, byte = 1, word = 2 */
	uint8_t amode_type; /* Addressing mode type */
	uint8_t amode; /* Addressing mode */
	char mnemonic[8]; /* assembly mnemonic */
} cinst_t;
cinst_t cinst;

uint8_t get_amode_argc(uint8_t amode, uint8_t amode_type, uint8_t data_size) {
	switch(amode_type) {
		case AMEM:
			if(!amode) { return(data_size); }
			else if(amode<3) { return(DS_W); }
			else { return(DS_B); }
		case AALU:
			return(amode?0:1);
		case AREL:
			return(1);
		default:
			return(0);
	}
}


void parse_cinst_opcode() {
	inst_base_t *ib;
	inst_node_t *n=0;
	amode_type_t *amtp;
	char *mod;
	char implicit;
	uint8_t i,j;
	for(i=0;(ib=inst_bases[i]);i++){
		for(j=0;ib->n[j].node;j++){
			n=&ib->n[j];
			if((cinst.opcode & ~n->mask) == n->offset) {
				cinst.op_type=ib->op_type;
				cinst.data_type=(ib->data_type==DS_WB?((cinst.opcode&0x10)?DS_W:DS_B):ib->data_type);
				cinst.amode_type=ib->amode_type;
				amtp=amode_types[cinst.amode_type];
				cinst.amode=((cinst.opcode)&(amtp->mask)&(n->mask))>>amtp->shift;
				cinst.argc=get_amode_argc(cinst.amode,cinst.amode_type,cinst.data_type);
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
	
	/* Parse the opcode */
	parse_cinst_opcode();

	/* Capture additional argument bytes */
	while(n<cinst.argc) {
		cinst.args[n]=mem_readb(PC++);
		n++;
	}

	return;
}

void parse_instruction() {
	//parse_cinst_args();
	return;
}

void execute_instruction() {
	
}

int main() {
	cpuregs=&(mem->r);
	cpuregset=&(cpuregs->i[0]);
	mem->a[0x100]=0xa0;
	mem->a[0x101]=0x55;
	PC=0x0100;
	fetch_instruction();
	printf("%s %02x\n",cinst.mnemonic, cinst.args[0]);
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
	printf("%c %x\n",mem->a[4], reg_readw(4));


	return(0);
}

