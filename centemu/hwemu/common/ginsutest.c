#include <stdint.h>
#include "logic-common.h"
#include "ginsumatic.h"
#include <stdio.h>
int main(int argc, char **argv) {
	char bs[16];
	char bso[16];
	uint8_t a,b,c;
	bitsalad_bag_t bsb;

	uint32_t in=  0x01234567;
	uint32_t out= 0x76543210;
	uint32_t mix= 0x01543267;

	char ch;

	bitsalad_prep_small( &bsb, 8, &a, &b, 0x01543267 );
	for(c=0;c<8;c++){
		a=1<<c;
		//b= bitsalad_n_byte(8, mix, 1<<a);
		bitsalad_shooter(&bsb);
		//bitsalad_tosser_n_byte(8,&a,&b, mix);
		byte_bits_to_binary_string_grouped(bs,a,8,4);
		byte_bits_to_binary_string_grouped(bso,b,8,4);
		printf("a=%s, b=%s\n",bs,bso);

	}
	b=0;
	for(int i=0; i<2; i++) {
		if((ch=*(argv[1]+i))) { b|=hexchar_to_nibble(ch)<<(4*(1-i)); }
		else { break; }
	}
	printf("Hex='%s' val='%x'\n",argv[1],b);
	return(0);
}
