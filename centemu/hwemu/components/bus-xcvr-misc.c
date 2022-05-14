#include <stdint.h>
#include "../common/logic-common.h"
#include "../common/clockline.h"
#include "bus-xcvr-misc.h"

/* am2907 */
void logic_am2907(logic_am2907_device_t *dev) {
	nibble_t p; /* Temp for calculating parity */

	/* Load driver data on rising clock edge */
	if(S_(clk)==CLK_LH) { I_(d)= S_(A); }

	/* If we're driving the bus, output our inverted data from driver register (open collector) */
	if( !S_(BE_) ) { OC_OUTPUT(S_(BUS_), ~I_(d)&0xf ); }

	/* Load receiver latch while RLE_=0 */
	if( !S_(RLE_) ) { I_(q)= S_(BUS_); }

	/* Tristate output of inverted q to R when OE_=0 */
	TRI_OUTPUT( S_(R), !S_(OE_), ~I_(q)&0xf );

	/* Generate parity when driving the bus, otherwise check parity */
	p= ( S_(BE_)?I_(q):~S_(A) )&0xf;
	p= p^(p>>2)?1:0;

	/* Output indicating if we got ODD parity or not */
	S_(ODD)=p;

}


/* ds8833 Non-inverting Quad Tri-state Bus Transceiver */
void logic_ds8833(logic_ds8833_device_t *dev) {
	TRI_OUTPUT( S_(BUS), S_(BE_), S_(A)&0xf );
	TRI_OUTPUT( S_(Q), S_(OE_), S_(BUS)&0xf );
}

/* ds8835 Quad Inverting Tri-state Bus Transceiver */
void logic_ds8835(logic_ds8835_device_t *dev) {
	TRI_OUTPUT( S_(BUS), S_(BE_), ~S_(A)&0xf );
	TRI_OUTPUT( S_(Q), S_(OE_), ~S_(BUS)&0xf );
}

