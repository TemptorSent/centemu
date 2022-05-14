#include <stdint.h>
#include "../common/logic-common.h"
#include "../common/clockline.h"

/* 93l422 256x4-bit (1kbit) Static RAM w/ Tri-state outputs */
struct ram_93l422_device_t {
	byte_t *A; /* Eight bit address */
	nibble_t *D; /* Nibble wide Data Read/Write */
	bit_t *WE_; /* (Active LO) Write Enable Write=0, Read=1 */
	bit_t *OE_; /* (Active LO) Output Enable, 1=HiZ */
	bit_t *CS1_; /* (Active LO) Chip Select 1 */
	bit_t *CS2; /* (Active HI) Chip Select 2 */
	nibble_t data[256]; /* Internal data storage of 256 nibbles (as 256 bytes) */
};
typedef struct ram_93l422_device_t ram_93l422_device_t;
	
void ram_93l422(ram_93l422_device_t *dev);
