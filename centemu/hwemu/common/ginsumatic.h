/* Does your header make julienne fries? */
#pragma once
#include <stdint.h>
#include <inttypes.h>
#include "logic-common.h"

#define deroach(...) printf(__VA_ARGS__);
//#define deroach(...) ; //printf(__VA_ARGS__);

/* Utility functions to extract ranges of bits, with _R reversing the bit order returned */
#define BITRANGE(d,s,n) (((d)>>(s)) & ((1LL<<(n))-1LL) )
#define BITRANGE_R(d,s,n) (bitreverse_64(BITRANGE(d,s,n)) >>(64-(n)))

/* Extract a bit from a larger type */
void bit_of_a_twobit( bit_t *dest, twobit_t *src, bit_t bit);
void bit_of_an_octal( bit_t *dest, octal_t *src, twobit_t bit);
void bit_of_a_nibble( bit_t *dest, nibble_t *src, twobit_t bit);
void bit_of_a_fivebit( bit_t *dest, fivebit_t *src, octal_t bit);
void bit_of_a_sixbit( bit_t *dest, sixbit_t *src, octal_t bit);
void bit_of_a_byte( bit_t *dest, byte_t *src, octal_t bit);
void bit_of_a_word( bit_t *dest, word_t *src, nibble_t bit);
void bit_of_a_longword( bit_t *dest, longword_t *src, byte_t bit);
void bit_of_a_mouthful( bit_t *dest, mouthful_t *src, byte_t bit);

/* Assemble bits into larger types */
void bits_to_twobit( twobit_t *dest, bit_t *b0, bit_t *b1);
void bits_to_octal( octal_t *dest, bit_t *b0, bit_t *b1, bit_t *b2 );
void bits_to_nibble( nibble_t *dest, bit_t *b0, bit_t *b1, bit_t *b2, bit_t *b3 );
void bits_to_fivebit( fivebit_t *dest, bit_t *b0, bit_t *b1, bit_t *b2, bit_t *b3, bit_t *b4 );
void bits_to_sixbit( sixbit_t *dest, bit_t *b0, bit_t *b1, bit_t *b2, bit_t *b3, bit_t *b4 , bit_t *b5);
void bits_to_sevenbit( sevenbit_t *dest, bit_t *b0, bit_t *b1, bit_t *b2, bit_t *b3, bit_t *b4 , bit_t *b5, bit_t *b6);
void bits_to_byte( byte_t *dest, bit_t *b0, bit_t *b1, bit_t *b2, bit_t *b3, bit_t *b4 , bit_t *b5, bit_t *b6, bit_t *b7);
/* ...if you need more than a byte, assemble to nibbles or bytes first, then blend? */


/* Pack nibbles, bytes, words, and longwords together */
/* to make bytes, words, longwords, and even mouthfuls! */

void nibbles_to_byte( byte_t *dest,
	nibble_t *n0, nibble_t *n1);

void nibbles_to_word( word_t *dest,
	nibble_t *n0, nibble_t *n1, nibble_t *n2, nibble_t *n3);

void bytes_to_word( word_t *dest,
	byte_t *b0, nibble_t *b1 );


void nibbles_to_longword( longword_t *dest,
	nibble_t *n0, nibble_t *n1, nibble_t *n2, nibble_t *n3,
	nibble_t *n4, nibble_t *n5, nibble_t *n6, nibble_t *n7 );
void bytes_to_longword( longword_t *dest,
	byte_t *b0, byte_t *b1, byte_t *b2, byte_t *b3);
void words_to_longword( longword_t *dest,
	word_t *w0, word_t *w1 );

void nibbles_to_mouthful( mouthful_t *dest,
	nibble_t *n0, nibble_t *n1, nibble_t *n2, nibble_t *n3,
	nibble_t *n4, nibble_t *n5, nibble_t *n6, nibble_t *n7, 
	nibble_t *n8, nibble_t *n9, nibble_t *na, nibble_t *nb,
	nibble_t *nc, nibble_t *nd, nibble_t *ne, nibble_t *nf ); 
void bytes_to_mouthful( mouthful_t *dest,
	byte_t *b0, byte_t *b1, byte_t *b2, byte_t *b3,
	byte_t *b4, byte_t *b5, byte_t *b6, byte_t *b7 );
void words_to_mouthful( mouthful_t *dest,
	word_t *w0, word_t *w1, word_t *w2, word_t *w3);
void longwords_to_mouthful( mouthful_t *dest,
	longword_t *lw0, longword_t *lw1 );


/* Concatenate an array of bytes into an unsigned 64-bit int */
uint64_t concat_bytes_64(uint8_t num, uint8_t bytes[]);

/* Split words, longwords, and mouthfuls into bytes */
void word_to_bytes( word_t *src, byte_t *b0, byte_t *b1 );
void longword_to_bytes ( longword_t *src, byte_t *b0, byte_t *b1, byte_t *b2, byte_t *b3 );
void mouthful_to_bytes ( mouthful_t *src,
		byte_t *b0, byte_t *b1, byte_t *b2, byte_t *b3,
		byte_t *b4, byte_t *b5, byte_t *b6, byte_t *b7 );


/* Reverse bits */
uint64_t bitreverse_64(uint64_t in);
uint32_t bitreverse_32(uint32_t in);
uint16_t bitreverse_16(uint16_t in);
uint8_t bitreverse_8(uint8_t in);

/* Rearrange bits */
uint8_t bitsalad_8(uint32_t order, uint8_t d);
uint16_t bitsalad_16(uint64_t order, uint16_t d);

/* Tosses 8 bits between *in and *out using a 32-bit salad */
void bitsalad_tosser_8(uint8_t *in, uint8_t *out, uint32_t salad);

/* Tosses 16 bits between *in and *out using a 64-bit salad */
/* Note that 64-bit constants need LL specifer! */
void bitsalad_tosser_16(uint16_t *in, uint16_t *out, uint64_t salad);

uint8_t bitsalad_n_byte(uint8_t serving, uint32_t order, uint8_t d);
uint8_t bitsalad_byte_n_word(uint8_t serving, uint32_t order, uint16_t d);
uint16_t bitsalad_n_word(uint8_t serving, uint64_t order, uint16_t d);

void bitsalad_tosser_n_byte(uint8_t serving, uint8_t *a, uint8_t *b, uint32_t salad);
void bitsalad_tosser_byte_n_word(uint8_t serving, uint16_t *a, uint8_t *b, uint32_t salad);
void bitsalad_tosser_n_word(uint8_t serving, uint16_t *a, uint16_t *b, uint64_t salad);

uint8_t bitsalad_byte(uint32_t order, uint8_t d);
uint8_t bitsalad_byte_word(uint32_t order, uint16_t d);
uint16_t bitsalad_word(uint64_t order, uint16_t d);

void bitsalad_tosser_byte(uint8_t *a, uint8_t *b, uint32_t salad);
void bitsalad_tosser_word(uint16_t *a, uint16_t *b, uint64_t salad);
void bitsalad_tosser_byte_word(uint16_t *a, uint8_t *b, uint32_t salad);



/* Rearrange serving bits using predefined input pointer, output pointer and salad */

enum bitsalad_bag_sizes { BITSALAD_BAG_SMALL, BITSALAD_BAG_MEDIUM, BITSALAD_BAG_LARGE };
typedef struct bitsalad_bag_t {
	enum bitsalad_bag_sizes size;
	uint8_t serving;
	union { uint8_t *byte; uint16_t *word; } in;
	union { uint8_t *byte; uint16_t *word; } out;
	union { uint32_t small; uint64_t large; } salad;
} bitsalad_bag_t;

/* Prepare a serving of n bits from word or byte size bowl into same size or smaller bowl! */
void bitsalad_prep_small(bitsalad_bag_t *bag, uint8_t serving, uint8_t *a, uint8_t *b, uint32_t salad);
void bitsalad_prep_medium(bitsalad_bag_t *bag, uint8_t serving, uint16_t *a, uint8_t *b, uint32_t salad);
void bitsalad_prep_large(bitsalad_bag_t *bag, uint8_t serving, uint16_t *a, uint16_t *b, uint64_t salad);

void bitsalad_shooter(bitsalad_bag_t *bag);

/* Blend bits from multiple sources */ 
uint16_t bitblender_16(uint8_t bits, char *order, uint8_t *sources[] );
uint8_t bitblender_8(uint8_t bits, char *order, uint8_t *sources[] );

/* Array of pointers to uint8_ts */
typedef uint8_t *byte_ptr_list_t[];

/* Struct to configure and use a universal bitblender */
typedef struct bitblender_t {
	union { uint8_t *b; uint16_t *w; uint32_t *lw; uint64_t *llw;}; /* Output pointers */
	uint8_t bits; /* Number of bits to blend */
	char *order; /* Character string with positions */
	byte_ptr_list_t *sources; /* Source byte pointers */
} bitblender_t;

/* Will it blend? */
void bitblend(bitblender_t *blend);

/* Pretty-print stringify integers to binary text */
char  *int64_bits_to_binary_string_fields(char *out, uint64_t in, uint8_t bits, char *fieldwidths);
char  *int64_bits_to_binary_string_grouped(char *out, uint64_t in, uint8_t bits, uint8_t grouping);

char  *byte_bits_to_binary_string_grouped(char *out, uint8_t in, uint8_t bits, uint8_t grouping);

/* Convert hex character to nibble */
nibble_t hexchar_to_nibble(char c);
