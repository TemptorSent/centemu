#pragma once
#include <stdint.h>
/* 'dev' accessors for values of external signals (pointers) and internal state of logic devices */
#define defS_(sig) dev->sig=sig;
#define S_(sig) (*(dev->sig))
#define oS_(sig,dflt) (dev->sig?*(dev->sig):dflt)

#define I_(internal_state) (dev->internal_state)

/* Output logic for open collector and tristate outputs */
#define OC_OUTPUT(output, ...) output = ( output & __VA_ARGS__ )
#define TRI_OUTPUT(output,cond, ...) if(cond) { output = ( __VA_ARGS__ ); }

/* Definition of bit_t, including HighZ for use with tri-state values */
typedef enum TRI_BIT { LO=0, LOW=0, HI=1, HIGH=1, HiZ=-128, HIZ=-128, HighZ=-128, HIGHZ=-128 } tri_bit_t;

typedef uint64_t mouthful_t;
typedef uint32_t longword_t;
typedef uint16_t word_t;
typedef uint16_t twelvebit_t;
typedef uint16_t tenbit_t;
typedef uint8_t byte_t;
typedef uint8_t sevenbit_t;
typedef uint8_t sixbit_t;
typedef uint8_t fivebit_t;
typedef uint8_t nibble_t;
typedef uint8_t octal_t;
typedef uint8_t twobit_t;
typedef uint8_t bit_t;
