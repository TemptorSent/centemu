#include <stdio.h>
#include <stdint.h>

typedef union regset {
	int16_t w[8];
	int8_t b[16];
	struct {
		int8_t AH,AL,BH,BL,CH,CL,DH,DL,EH,EL,FH,FL,GH,GL,HH,HL;	
	};
	struct {
		int16_t AX,BX,CX,DX,EX,FX,GX,HX;
	};
} regset;

int main() {
	union {
		int8_t address_space[0x10000];
		struct {
			union {
				uint16_t w[8];
				uint8_t b[16];
				struct {

					regset il_0;
					regset il_1;
					regset il_2;
					regset il_3;
					regset il_4;
					regset il_5;
					regset il_6;
					regset il_7;
					regset il_8;
					regset il_9;
					regset il_A;
					regset il_B;
					regset il_C;
					regset il_D;
					regset il_E;
					regset il_F;
				};
			} regs;
			int8_t prog[0xf000-0xff];
			union {
				int8_t mmio[0x1000];
			};
		};
	} mem;

	mem.regs.il_0.AH='a';
	printf("%0x\n",mem.address_space[0]);


	return(0);
}

