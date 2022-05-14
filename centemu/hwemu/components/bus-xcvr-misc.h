#include <stdint.h>
#include "../common/logic-common.h"
#include "../common/clockline.h"

/* am2907 Quad Bus Transceivers with Interface Logic D-type driver with open-collector bus driver output */
struct logic_am2907_device_t {
	clock_state_t *clk; /* Driver Clock Pulse - "DRCP" */
	nibble_t *A; /* Driver Register Inputs */
	bit_t *RLE_; /* (Active LO) Receiver Latch Enable; 0=Pass BUS input through latches / 1=Hold latched value */
	nibble_t *BUS_; /* (Inverting) Bus Driver Outputs / Receiver Inputs */
	bit_t *BE_; /* (Active LO) Bus Driver Enable, 1=Bus Drivers HiZ */
	nibble_t *R; /* Receiver Outputs */
	bit_t *OE_; /* (Active LO) Receiver Output Enable, 1=Receiver Outputs HiZ */
	bit_t *ODD; /* Odd Parity Output; Generate parity when BE_=0, check parity when BE_=1 */
	nibble_t d; /* Driver register internal state */
	nibble_t q; /* Receiver register internal state */
};
typedef struct logic_am2907_device_t logic_am2907_device_t;

void logic_am2907(logic_am2907_device_t *dev);


/* ds8833 Series Quad Tri-state Bus Transceivers */
struct logic_ds8833_device_t {
	nibble_t *A;
	nibble_t *BUS;
	nibble_t *BE_;
	nibble_t *Q;
	bit_t *OE_;
};
/* ds8833 Non-inverting Quad Tri-state Bus Transceiver */
typedef struct logic_ds8833_device_t logic_ds8833_device_t;
void logic_ds8833(logic_ds8833_device_t *dev);

/* ds8835 Quad Inverting Tri-state Bus Transceiver */
typedef struct logic_ds8833_device_t logic_ds8835_device_t;
void logic_ds8835(logic_ds8835_device_t *dev);

