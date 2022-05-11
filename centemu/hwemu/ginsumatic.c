/* It slices, it dices... But wait! There's more! */
#include <stdio.h>
#include <inttypes.h>
#include "logic-common.h"
#include "ginsumatic.h"

/* Extract a bit from a larger type */
void bit_of_a_twobit( bit_t *dest, twobit_t *src, bit_t bit) { *dest=(*src&0x03&(1<<bit))?1:0; }
void bit_of_an_octal( bit_t *dest, octal_t *src, twobit_t bit) { *dest=(*src&0x07&(1<<bit))?1:0; }
void bit_of_a_nibble( bit_t *dest, nibble_t *src, twobit_t bit) { *dest=(*src&0x0f&(1<<bit))?1:0; }
void bit_of_a_fivebit( bit_t *dest, fivebit_t *src, octal_t bit) { *dest=(*src&0x1f&(1<<bit))?1:0; }
void bit_of_a_sixbit( bit_t *dest, sixbit_t *src, octal_t bit) { *dest=(*src&0x3f&(1<<bit))?1:0; }
void bit_of_a_byte( bit_t *dest, byte_t *src, octal_t bit) { *dest=(*src&(1<<bit))?1:0; }
void bit_of_a_word( bit_t *dest, word_t *src, nibble_t bit) { *dest=(*src&(1<<bit))?1:0; }
void bit_of_a_longword( bit_t *dest, longword_t *src, byte_t bit) { *dest=(*src&(1<<bit))?1:0; }
void bit_of_a_mouthful( bit_t *dest, mouthful_t *src, byte_t bit) { *dest=(*src&(1LL<<bit))?1:0; }

/* Assemble bits into larger types */
void bits_to_twobit( twobit_t *dest, bit_t *b0, bit_t *b1) { *dest=( (*b0<<0) | (*b1<<1) ); }
void bits_to_octal( octal_t *dest, bit_t *b0, bit_t *b1, bit_t *b2 ) { *dest=( (*b0<<0) | (*b1<<1) | (*b2<<2) ); }
void bits_to_nibble( nibble_t *dest, bit_t *b0, bit_t *b1, bit_t *b2, bit_t *b3 ) {
	*dest=( (*b0<<0) | (*b1<<1) | (*b2<<2) | (*b3<<3) );
}
void bits_to_fivebit( fivebit_t *dest, bit_t *b0, bit_t *b1, bit_t *b2, bit_t *b3, bit_t *b4 ) {
	*dest=( (*b0<<0) | (*b1<<1) | (*b2<<2) | (*b3<<3) | (*b4<<4) );
}
void bits_to_sixbit( sixbit_t *dest, bit_t *b0, bit_t *b1, bit_t *b2, bit_t *b3, bit_t *b4 , bit_t *b5) {
	*dest=( (*b0<<0) | (*b1<<1) | (*b2<<2) | (*b3<<3) | (*b4<<4) | (*b5<<5) );
}
void bits_to_sevenbit( sevenbit_t *dest, bit_t *b0, bit_t *b1, bit_t *b2, bit_t *b3, bit_t *b4 , bit_t *b5, bit_t *b6) {
	*dest=( (*b0<<0) | (*b1<<1) | (*b2<<2) | (*b3<<3) | (*b4<<4) | (*b5<<5) | (*b6<<6) );
}
void bits_to_byte( byte_t *dest, bit_t *b0, bit_t *b1, bit_t *b2, bit_t *b3, bit_t *b4 , bit_t *b5, bit_t *b6, bit_t *b7) {
	*dest=( (*b0<<0) | (*b1<<1) | (*b2<<2) | (*b3<<3) | (*b4<<4) | (*b5<<5) | (*b6<<6) | (*b7<<7) );
}
/* ...if you need more than a byte, assemble to nibbles or bytes first, then blend? */


void nibbles_to_byte( byte_t *dest,
	nibble_t *n0, nibble_t *n1)
{
	*dest=( (*n0)|(*n1<<4) );
}

void nibbles_to_word( word_t *dest,
	nibble_t *n0, nibble_t *n1, nibble_t *n2, nibble_t *n3)
{
	*dest=( (*n0)|(*n1<<4)|(*n2<<8)|(*n3<<12) );
}

void bytes_to_word( word_t *dest,
	byte_t *b0, nibble_t *b1 )
{
	*dest=( (*b0)|(*b1<<8) );
}



void nibbles_to_longword( longword_t *dest,
	nibble_t *n0, nibble_t *n1, nibble_t *n2, nibble_t *n3,
	nibble_t *n4, nibble_t *n5, nibble_t *n6, nibble_t *n7 )
{
	*dest=( (*n0)|(*n1<<4)|(*n2<<8)|(*n3<<12)|(*n4<<16)|(*n5<<20)|(*n6<<24)|(*n7<<28) );
}

void bytes_to_longword( longword_t *dest,
	byte_t *b0, byte_t *b1, byte_t *b2, byte_t *b3)
{
	*dest=( (*b0)|(*b1<<8)|(*b2<<16)|(*b3<<24) );
}

void words_to_longword( longword_t *dest,
	word_t *w0, word_t *w1 )
{
	*dest=( (*w0)|(*w1<<16) );
}


void nibbles_to_mouthful( mouthful_t *dest,
	nibble_t *n0, nibble_t *n1, nibble_t *n2, nibble_t *n3,
	nibble_t *n4, nibble_t *n5, nibble_t *n6, nibble_t *n7, 
	nibble_t *n8, nibble_t *n9, nibble_t *na, nibble_t *nb,
	nibble_t *nc, nibble_t *nd, nibble_t *ne, nibble_t *nf ) 
{
	*dest=( ((1LL * *n0)<<0) |((1LL * *n1)<<4) |((1LL * *n2)<<8) |((1LL * *n3)<<12)|
		((1LL * *n4)<<16)|((1LL * *n5)<<20)|((1LL * *n6)<<24)|((1LL * *n7)<<28)|
		((1LL * *n8)<<32)|((1LL * *n9)<<36)|((1LL * *na)<<40)|((1LL * *nb)<<44)|
		((1LL * *nc)<<48)|((1LL * *nd)<<52)|((1LL * *ne)<<56)|((1LL * *nf)<<60) );
}

void bytes_to_mouthful( mouthful_t *dest,
	byte_t *b0, byte_t *b1, byte_t *b2, byte_t *b3,
	byte_t *b4, byte_t *b5, byte_t *b6, byte_t *b7 )
{
	deroach("bytes to mouthful: %02x %02x %02x %02x %02x %02x %02x %02x\n", *b7,*b6,*b5,*b4,*b3,*b2,*b1,*b0);
	*dest=( (1LL * *b0)|((1LL * *b1)<<8)|((1LL * *b2)<<16)|((1LL * *b3)<<24)|
		((1LL * *b4)<<32)|((1LL * *b5)<<40)|((1LL * *b6)<<48)|((1LL * *b7)<<56) );
}

void words_to_mouthful( mouthful_t *dest,
	word_t *w0, word_t *w1, word_t *w2, word_t *w3)
{
	*dest=( ((1LL * *w0)<<0) | ((1LL * *w1)<<16) | ((1LL * *w2)<<32) | ((1LL * *w3)<<48) );
}

void longwords_to_mouthful( mouthful_t *dest,
	longword_t *lw0, longword_t *lw1 )
{
	*dest=( ((1LL * *lw0)<<0) | ((1LL * *lw1)<<32) );
}


/* Concatenate an array of bytes into an unsigned 64-bit int */
uint64_t concat_bytes_64(uint8_t num, uint8_t bytes[]){
	uint64_t out=0;
	for(int i=0;i<num;i++) {
		out = out | ( ( 1LL * bytes[i] )<<(i*8));
	}
	return(out);
}


void word_to_bytes( word_t *src, byte_t *b0, byte_t *b1 ) {
	*b0=(*src)&0x00ff;
	*b1=((*src)&0xff00)>>8;
}
void longword_to_bytes ( longword_t *src, byte_t *b0, byte_t *b1, byte_t *b2, byte_t *b3 ) {
	*b0=((*src)&0x000000ff);
	*b1=((*src)&0x0000ff00)>>8;
	*b2=((*src)&0x00ff0000)>>16;
	*b3=((*src)&0xff000000)>>24;
}
void mouthful_to_bytes ( mouthful_t *src,
		byte_t *b0, byte_t *b1, byte_t *b2, byte_t *b3,
		byte_t *b4, byte_t *b5, byte_t *b6, byte_t *b7 )
{
	*b0=((*src)&0x00000000000000ffLL);
	*b1=((*src)&0x000000000000ff00LL)>>8;
	*b2=((*src)&0x0000000000ff0000LL)>>16;
	*b3=((*src)&0x00000000ff000000LL)>>24;
	*b4=((*src)&0x000000ff00000000LL)>>32;
	*b5=((*src)&0x0000ff0000000000LL)>>40;
	*b6=((*src)&0x00ff000000000000LL)>>58;
	*b7=((*src)&0xff00000000000000LL)>>56;
}

#define _bitreverse_(size) \
uint##size##_t bitreverse_##size(uint##size##_t in) { \
	uint##size##_t out=0; \
	for(int i=size-1; i>=0; i--) { \
		out |= in&0x1; \
		if(!i){return(out);} \
		out <<=1; in >>=1; \
	}\
	return(out);\
}
_bitreverse_(64);
_bitreverse_(32);
_bitreverse_(16);
_bitreverse_(8);
#undef _bitreverse_



/* Bitsalad */
/* 0x76543210 */
uint8_t bitsalad_n_byte(uint8_t serving, uint32_t order, uint8_t d) {
	uint8_t pos;
	uint8_t out;
	for(int i=0; i<serving; i++) {
		pos=BITRANGE(order,i*4,4);
		if(serving>pos) {
			if(d&(1<<i)){out |= 1<<pos;}
		} else {
			serving++;
		}
	}
	return(out);
}

/* 0xf3c2d510 */
uint8_t bitsalad_byte_n_word(uint8_t serving, uint32_t order, uint16_t d) {
	return( bitsalad_n_word( serving, 0x00000000ffffffffLL&order, d ) & 0xff );
}

/* 0xfedcba9876543210LL */
uint16_t bitsalad_n_word(uint8_t serving, uint64_t order, uint16_t d) {
	uint8_t pos;
	uint16_t out;
	for(int i=0; i<serving; i++) {
		pos=BITRANGE(order,i*4,4);
		if(serving>pos) {
			if((d&1<<i)){out |= 1<<pos;}
		} else {
			serving++;
		}
	}
	return(out);
}


/* Salad tossers - serve up n bits in byte or word flavor from a bowl of 8 or 16 bits */
/* *a points to source *b points to destination */
void bitsalad_tosser_n_byte(uint8_t serving, uint8_t *a, uint8_t *b, uint32_t salad) {
	*b=bitsalad_n_byte(serving, salad,*a);
}

void bitsalad_tosser_byte_n_word(uint8_t serving, uint16_t *a, uint8_t *b, uint32_t salad) {
	*b=bitsalad_n_word( serving, 0x00000000ffffffffLL&salad, *a ) & 0xff;
}

void bitsalad_tosser_n_word(uint8_t serving, uint16_t *a, uint16_t *b, uint64_t salad) {
	*b=bitsalad_n_word(serving, salad, *a);
}

/* Fixed size versions of above, 8->8, 16->8 or 16->16 bits */
uint8_t bitsalad_byte(uint32_t order, uint8_t d) {
	return( bitsalad_n_byte(8, order, d) );
}
uint8_t bitsalad_byte_word(uint32_t order, uint16_t d) {
	return( bitsalad_byte_n_word(8, order, d) );
}
uint16_t bitsalad_word(uint64_t order, uint16_t d) {
	return( bitsalad_n_word(16, order, d) );
}


void bitsalad_tosser_byte(uint8_t *a, uint8_t *b, uint32_t salad) {
	*b=bitsalad_byte(salad,*a);
}
void bitsalad_tosser_byte_word(uint16_t *a, uint8_t *b, uint32_t salad) {
	*b=bitsalad_byte_word(salad, *a);
}
void bitsalad_tosser_word(uint16_t *a, uint16_t *b, uint64_t salad) {
	*b=bitsalad_word(salad,*a);
}


/* Prepare a serving of n bits from word or byte size bowl into same size or smaller bowl! */

void bitsalad_prep_small(bitsalad_bag_t *bag, uint8_t serving, uint8_t *a, uint8_t *b, uint32_t salad) {
	bag->size=BITSALAD_BAG_SMALL; bag->serving=serving; bag->in.byte=a; bag->out.byte=b; bag->salad.small=salad;
}
void bitsalad_prep_medium(bitsalad_bag_t *bag, uint8_t serving, uint16_t *a, uint8_t *b, uint32_t salad) {
	bag->size=BITSALAD_BAG_MEDIUM; bag->serving=serving; bag->in.word=a; bag->out.byte=b; bag->salad.small=salad;
}
void bitsalad_prep_large(bitsalad_bag_t *bag, uint8_t serving, uint16_t *a, uint16_t *b, uint64_t salad) {
	bag->size=BITSALAD_BAG_LARGE; bag->serving=serving; bag->in.word=a; bag->out.word=b; bag->salad.large=salad;
}

/* Powertools for salad! */
void bitsalad_shooter(bitsalad_bag_t *bag) {
	switch(bag->size) {
		case BITSALAD_BAG_SMALL:
			deroach("Shooting a small bag of bitsalad: ");
			bitsalad_tosser_n_byte(bag->serving, bag->in.byte, bag->out.byte, bag->salad.small);
			deroach("in: 0x%02x  bits:%2x  salad: 0x%08x  out: 0x%02x\n",
					*bag->in.byte, bag->serving, bag->salad.small, *bag->out.byte);
			break;
		case BITSALAD_BAG_MEDIUM:
			deroach("Shooting a medium bag of bitsalad: ");
			bitsalad_tosser_byte_n_word(bag->serving, bag->in.word, bag->out.byte, bag->salad.small);
			deroach("in: 0x%04x  bits:%2x  salad: 0x%08x  out: 0x%02x\n",
					*bag->in.word, bag->serving, bag->salad.small, *bag->out.byte);
			break;
		case BITSALAD_BAG_LARGE:
			deroach("Shooting a large bag of bitsalad: ");
			bitsalad_tosser_n_word(bag->serving, bag->in.word, bag->out.word, bag->salad.large);
			deroach("in: 0x%04x  bits:%2x  salad: 0x%016"PRIx64"  out: 0x%04x\n",
					*bag->in.word, bag->serving, bag->salad.large, *bag->out.word);
			break;
	}
}

#define _bitblend_(size) \
uint##size##_t bitblend_##size(uint8_t bits, char *order, uint8_t *source[]) { \
	uint##size##_t out=0; \
	uint8_t pos; \
	char *c; \
	c=order; \
	for(int i=0;i<bits;i++ ) { \
		while( *c > size ) { ; } \
		pos=c-order; \
		if (*(source[pos>>3]) & (1<<(pos&0x3)) ){ out|= (1<<*c); } \
		c++; \
	} \
	return(out);\
}

_bitblend_(8);
_bitblend_(16);
_bitblend_(32);
_bitblend_(64);

void bitblend(bitblender_t *blend) {
	uint8_t pos;
	char *c;
	c=blend->order;
	for(int i=0;i<blend->bits;i++ ) {
	//	deroach("\nblending bit %i of %u",i,blend->bits);
		while( (uint8_t)*c > blend->bits - 1 ) { /* deroach("\nskipping c=%u",*c);*/ c++; }
		pos=c-blend->order;
		if (*((*blend->sources)[(pos>>3)]) & (1<<(pos&0x3)) ){
	//		deroach("blending bit %i of %u: out=%u, adding found %u at %u: %u\n",
	//				i, blend->bits, *(blend->w),*c,pos, (1<<*c));
			if(blend->bits<9) { *(blend->b) |= (1<<*c); }
			else if(blend->bits<17) { *(blend->w) |= (1<<*c); }
			else if(blend->bits<33) { *(blend->lw) |= (1<<*c); }
			else { *(blend->llw) |= (1LL<<*c); }
		}
		c++;
	}
	return;
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

/* Represent specified number of bits from a byte  as an ascii string of '1's and '0's */
/* Provide an output buffer long enough to hold all bits plus spaces between fields plus NULL */
/* Split sequences of bits into space separated groups of specified size */
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

nibble_t hexchar_to_nibble(char c) {
	if(c >= '0' && c <= '9') { return(c-'0'); }
	if(c >= 'A' && c <= 'F') { return(c-'A'+10); }
	if(c >= 'a' && c <= 'f') { return(c-'a'+10); }
	printf("Passed unknown character '%c' to hexchar_to_nibble\n",c);
	return(0);
}

int ginsumatic_main() {
	char blendtec[8]="\x00\x01\x02\x03\x07\x06\x05\x04";
	uint8_t in=0x55;
	uint8_t *s[]={&in};
	byte_ptr_list_t t={&in};
	uint8_t out;
	uint16_t out2;
	bitblender_t blend = {{&out}, 8,blendtec,&t};
	uint8_t inA=0x05, inB=0x0a, inC=0x0f;
	byte_ptr_list_t seq_output_list={&inA, &inB, &inC};
	char *seq_output_order="\x00\x01\x02\x03\xff\xff\xff\xff\x04\x05\x06\x07\xff\xff\xff\xff\x08\x09\x0a\x0b";
	bitblender_t seq_output_blender={{.w=&out2},11,seq_output_order,&seq_output_list};
	bitblend(&blend);
	printf("blending %02x -> %02x\n", in, out);
	
	bitblend(&seq_output_blender);
	printf("blending %02x %02x %02x -> %04x\n", inA,inB,inC, out2);
	
	
	printf("blending %02x -> %02x\n", in, bitblend_8(8,blendtec,s));
	return(0);
}
