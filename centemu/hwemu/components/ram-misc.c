#include <stdint.h>
#include "../common/logic-common.h"
#include "../common/clockline.h"
#include "ram-misc.h"

/* 93l422 256x4-bit (1kbit) Static RAM w/ Tri-state outputs */
void ram_93l422(ram_93l422_device_t *dev) {
	/* Do nothing if not enabled, D=HiZ */
	if( !S_(CS1_) && S_(CS2) ) {
		if( !S_(WE_) ) {
			/* WE_=0, Write Data in D to location in A */
			I_(data[S_(A)])= S_(D)&0xf;
		} else if( !S_(OE_) ) {
			/* Read Data from location A and output to D */
			S_(D)= I_(data[S_(A)])&0xf;
		}
	}
}


