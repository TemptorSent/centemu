/* Does your header make julienne fries? */
#pragma once
#include <inttypes.h>

/* Utility functions to extract ranges of bits, with _R reversing the bit order returned */
#define BITRANGE(d,s,n) ((d>>s) & ((2LL<<(n-1))-1) )
#define BITRANGE_R(d,s,n) (bitreverse_64(BITRANGE(d,s,n)) >>(64-n))

/* Pack nibbles together to make bytes, words, longwords, and even mouthfuls! */
void nibbles_to_byte(uint8_t *dest, nibble_t *n0, nibble_t *n1);

void nibbles_to_word(uint16_t *dest,
	nibble_t *n0, nibble_t *n1, nibble_t *n2, nibble_t *n3);

void nibbles_to_longword( uint32_t *dest,
	nibble_t *n0, nibble_t *n1, nibble_t *n2, nibble_t *n3,
	nibble_t *n4, nibble_t *n5, nibble_t *n6, nibble_t *n7 );

void nibbles_to_mouthful( uint64_t *dest,
	nibble_t *n0, nibble_t *n1, nibble_t *n2, nibble_t *n3,
	nibble_t *n4, nibble_t *n5, nibble_t *n6, nibble_t *n7, 
	nibble_t *n8, nibble_t *n9, nibble_t *na, nibble_t *nb,
	nibble_t *nc, nibble_t *nd, nibble_t *ne, nibble_t *nf );

/* Tosses 8 bits between *in and *out using a 32-bit salad */
void bitsalad_tosser_8(uint8_t *in, uint8_t *out, uint32_t salad);

/* Tosses 16 bits between *in and *out using a 64-bit salad */
/* Note that 64-bit constants need LL specifer! */
void bitsalad_tosser_16(uint16_t *in, uint16_t *out, uint64_t salad);


/* Concatenate an array of bytes into an unsigned 64-bit int */
uint64_t concat_bytes_64(uint8_t num, uint8_t bytes[]);



/* Reverse bits */
uint64_t bitreverse_64(uint64_t in);
uint32_t bitreverse_32(uint32_t in);
uint16_t bitreverse_16(uint16_t in);
uint8_t bitreverse_8(uint8_t in);

/* Rearrange bits */
uint16_t bitsalad_16(uint64_t order, uint16_t d);
uint8_t bitsalad_8(uint32_t order, uint8_t d);

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

